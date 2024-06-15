/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.
*/

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"
#include "full_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#if defined(LLVM34)
#include "llvm/DebugInfo.h"
#else
#include "llvm/IR/DebugInfo.h"
#endif

#if defined(LLVM34) || defined(LLVM35) || defined(LLVM36)
#define LLVM_OLD_DEBUG_API
#endif

/* User-facing macro to sprintf() to a dynamically allocated buffer. */

#define alloc_printf(_str...)                                                  \
  ({                                                                           \
    char *_tmp;                                                                \
    s32 _len = snprintf(NULL, 0, _str);                                        \
    if (_len < 0)                                                              \
      FATAL("Whoa, snprintf() fails?!");                                       \
    _tmp = (char *)malloc(_len + 1);                                           \
    snprintf((char *)_tmp, _len + 1, _str);                                    \
    _tmp;                                                                      \
  })

using namespace llvm;

namespace {

class AFLCoverage : public Tracer {

public:
  //static char ID;
  std::vector<std::string> target;
  AFLCoverage();

  bool runOnModule(Module &M) override;

  StringRef getPassName() const override {
    return "American Fuzzy Lop Instrumentation";
  }
};

} // namespace

//char AFLCoverage::ID = 0;

cl::opt<std::string> PoCtrace("poc_trace", cl::desc("trace of poc."),
                              cl::ZeroOrMore);
cl::opt<bool> KeepIntermediate("keep_intermediate",
                               cl::desc("keep intermediate IR files."),
                               cl::ZeroOrMore);

AFLCoverage::AFLCoverage() {
  std::cout << "PoCtrace: " << PoCtrace << std::endl;
  std::cout << "KeepIntermediate: " << KeepIntermediate << std::endl;
  base_dir = PoCtrace.substr(0, PoCtrace.find_last_of('/'));

  std::ifstream pt(PoCtrace);
  std::string inst_info;

  while (std::getline(pt, inst_info)) {
    target.push_back(inst_info);
  }

  pt.close();
}

void split(const std::string &s, std::vector<std::string> &sv,
           std::string delimiter) {
  size_t pos = 0;
  std::string temp;
  std::string str = s;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    temp = str.substr(0, pos);
    sv.push_back(temp);
    str = str.substr(pos + delimiter.length());
  }
  if (int(str.length()) > 0) {
    sv.push_back(str);
  }
  return;
}

bool AFLCoverage::runOnModule(Module &M) {

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {
    // SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST
    //           " by <lszekeres@google.com>\n");
  } else
    be_quiet = 1;

  /* Decide instrumentation ratio */
  char *inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {
    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");
  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

  GlobalVariable *AFLMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

  GlobalVariable *AFLPrevLoc = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc", 0,
      GlobalVariable::GeneralDynamicTLSModel, 0, false);

  doInitialization(M);

  if (KeepIntermediate) {
    int status = mkdir("./.tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == 0) {
      WARNF("failed to create .tmp dir.");
    } else {
      std::error_code ErrInfo;
      std::string filename = M.getSourceFileName();
      filename = filename.substr(filename.find_last_of("/\\") + 1);
      char *modified_file = alloc_printf("%s/.afl-%s-%u-origin.ll", "./.tmp",
                                        filename.c_str(), (u32)time(NULL));
      raw_ostream *out = new raw_fd_ostream(std::string(modified_file), ErrInfo,
                                            sys::fs::OpenFlags::F_RW);
      if (out == nullptr) {
        WARNF("failed to open %s\n", modified_file);
      } else {
        SAYF("write to %s\n", modified_file);
        M.print(*out, 0);
        out->flush();
        delete out;
      }
    }
  }

  /* Instrument all the things! */
  int inst_blocks = 0;
  for (auto &F : M) {
    runOnFunction(F, target);

    for (auto &BB : F) {
      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      if (AFL_R(100) >= inst_ratio)
        continue;

      /* Make up cur_loc */
      unsigned int cur_loc = AFL_R(MAP_SIZE);
      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      /* Load prev_loc */

      LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
      PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      /* Load SHM pointer */

      LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
      MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *MapPtrIdx =
          IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

      /* Update bitmap */

      LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
      Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
      Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
      IRB.CreateStore(Incr, MapPtrIdx)
          ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      /* Set prev_loc to cur_loc >> 1 */

      StoreInst *Store =
          IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
      Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

      inst_blocks++;
    }
  }

  dofinish();

  if (KeepIntermediate) {
    int status = mkdir("./.tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == 0) {
      WARNF("failed to create .tmp dir.");
    } else {
      std::error_code ErrInfo;
      std::string filename = M.getSourceFileName();
      filename = filename.substr(filename.find_last_of("/\\") + 1);
      char *modified_file = alloc_printf("%s/.afl-%s-%u.ll", "./.tmp",
                                        filename.c_str(), (u32)time(NULL));
      raw_ostream *out = new raw_fd_ostream(std::string(modified_file), ErrInfo,
                                            sys::fs::OpenFlags::F_RW);
      if (out == nullptr) {
        WARNF("failed to open %s\n", modified_file);
      } else {
        SAYF("write to %s\n", modified_file);
        M.print(*out, 0);
        out->flush();
        delete out;
      }
    }
  }

  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks)
      WARNF("No instrumentation targets found.");
    else
      OKF("Instrumented %u locations (%s mode, ratio %u%%).", inst_blocks,
          getenv("AFL_HARDEN")
              ? "hardened"
              : ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN"))
                     ? "ASAN/MSAN"
                     : "non-hardened"),
          inst_ratio);
  }

  return true;
}

static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());
}

static RegisterStandardPasses
    RegisterAFLPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                    registerAFLPass);

static RegisterStandardPasses
    RegisterAFLPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                     registerAFLPass);
