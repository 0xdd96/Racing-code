#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <time.h>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InlineAsm.h"

#include "full_trace.h"

using namespace llvm;
using namespace std;

raw_fd_ostream *logstream, *trace_id_steam, *inst_id_steam, *bb_id_steam;

namespace {

static void getDebugLoc(Instruction *I, std::string &Filename, unsigned &Line,
                        unsigned &Column) {
#ifdef LLVM_OLD_DEBUG_API
  DebugLoc Loc = I->getDebugLoc();
  if (!Loc.isUnknown()) {
    DILocation cDILoc(Loc.getAsMDNode(M.getContext()));
    DILocation oDILoc = cDILoc.getOrigLocation();

    Line = oDILoc.getLineNumber();
    Filename = oDILoc.getFilename().str();
    Column = oDILoc.getColumnNumber();

    if (filename.empty()) {
      Line = cDILoc.getLineNumber();
      Filename = cDILoc.getFilename().str();
      Column = cDILoc.getColumnNumber();
    }
  }
#else
  if (DILocation *Loc = I->getDebugLoc()) {
    Line = Loc->getLine();
    Filename = Loc->getFilename().str();
    Column = Loc->getColumn();

    if (Filename.empty()) {
      DILocation *oDILoc = Loc->getInlinedAt();
      if (oDILoc) {
        Line = oDILoc->getLine();
        Filename = oDILoc->getFilename().str();
        Column = oDILoc->getColumn();
      }
    }
  }
#endif /* LLVM_OLD_DEBUG_API */

  size_t lastSlash = Filename.rfind('/');
  if (lastSlash != std::string::npos) {
    size_t secondLastSlash = Filename.rfind('/', lastSlash - 1);
    if (secondLastSlash != std::string::npos)
      Filename = Filename.substr(secondLastSlash + 1);
  }
}

} // end of anonymous namespace

Tracer::Tracer() : ModulePass(ID) {}

bool Tracer::doInitialization(Module &M) {
  std::error_code ErrInfo;
  //filename = M.getSourceFileName();
  //filename = filename.substr(filename.find_last_of("/\\") + 1);

  char *trace_id = alloc_printf("trace-id.log");
  trace_id_steam = new raw_fd_ostream(base_dir + "/" + std::string(trace_id),
                                      ErrInfo, sys::fs::OpenFlags::F_Append);
  if (trace_id == nullptr) {
    SAYF("failed to open %s\n", trace_id);
  }

  std::ifstream inst_id_file(base_dir + "/" + "inst_id");
  if (!inst_id_file.is_open()) {
    outs() << "failed to open inst_id file\n";
    inst_id = 0;
  } else {
    inst_id = 0;
    std::string inst_id_str;
    while (std::getline(inst_id_file, inst_id_str)) {
      inst_id = atoi(inst_id_str.c_str());
    }
    inst_id_file.close();
  }
  outs() << "initial inst_id: " << inst_id << "\n";

  std::ifstream bb_id_file(base_dir + "/" + "bb_id");
  if (!bb_id_file.is_open()) {
    outs() << "failed to open bb_id file\n";
    bb_id = 1;
    bb_startIndex = 0;
  } else {
    bb_id = 1;
    bb_startIndex = 0;
    std::string bb_id_str;
    while (std::getline(bb_id_file, bb_id_str)) {
      size_t pos = bb_id_str.find(',');
      if (pos != std::string::npos) {
        bb_id = atoi(bb_id_str.substr(0, pos).c_str());
        bb_startIndex =  atoi(bb_id_str.substr(pos+1).c_str());
      }
    }
    bb_id_file.close();
  }
  outs() << "initial bb_id: " << bb_id << "\n";
  outs() << "initial bb_index: " << bb_startIndex << "\n";

  auto &llvm_context = M.getContext();
  auto I1Ty = Type::getInt1Ty(llvm_context);
  auto I16Ty = Type::getInt16Ty(llvm_context);
  auto I32Ty = Type::getInt32Ty(llvm_context);
  auto I64Ty = Type::getInt64Ty(llvm_context);
  auto I8PtrTy = Type::getInt8PtrTy(llvm_context);
  auto VoidTy = Type::getVoidTy(llvm_context);
  auto DoubleTy = Type::getDoubleTy(llvm_context);

  // Add external trace_value function declarations.
  TL_trace_value = M.getOrInsertFunction("trace_value", I64Ty, I64Ty);
  TL_trace_register = M.getOrInsertFunction("trace_register", I64Ty, I64Ty);
  TL_trace_edge = M.getOrInsertFunction("trace_edge", I32Ty, I32Ty, I16Ty);
  return false;
}

void Tracer::dofinish() {
  std::error_code ErrInfo;
  char *inst_id_file = alloc_printf("inst_id");
  outs() << base_dir + "/" + std::string(inst_id_file) << "\n";
  inst_id_steam = new raw_fd_ostream(
      base_dir + "/" + std::string(inst_id_file), ErrInfo,
      sys::fs::OpenFlags::F_Append | sys::fs::OpenFlags::F_RW);
  outs() << "final inst_id: " << inst_id << "\n";
  *inst_id_steam << inst_id << '\n';
  inst_id_steam->close();

  char * bb_id_file = alloc_printf("bb_id");
  outs() << base_dir + "/" + std::string(bb_id_file) << "\n";
  bb_id_steam = new raw_fd_ostream(
      base_dir + "/" + std::string(bb_id_file), ErrInfo,
      sys::fs::OpenFlags::F_Append | sys::fs::OpenFlags::F_RW);
  outs() << "final bb_id: " << bb_id<<"," << bb_startIndex << "\n";
  *bb_id_steam << bb_id<<"," << bb_startIndex << '\n';
  bb_id_steam->close();

  /*
  for (const auto sourceCodeInfoEntry : inst_id_map) {
    const std::string sourceCodeInfo = sourceCodeInfoEntry.first;
    for (const auto instructionInfoEntry : sourceCodeInfoEntry.second) {
      const std::string instructionInfo = instructionInfoEntry.first;
      int instructionID = instructionInfoEntry.second;
      *trace_id_steam << sourceCodeInfo << "," << instructionInfo << "," << instructionID << std::endl;
    }
  }
  trace_id_steam->close();
  */
  for (const auto& sourceCodeInfoEntry : inst_id_map) {
    const std::string& sourceCodeInfo = sourceCodeInfoEntry.first;
    for (const auto& instructionEntry : sourceCodeInfoEntry.second) {
      const std::string& instruction = instructionEntry.first;
      int instructionID = instructionEntry.second;
      *trace_id_steam << sourceCodeInfo << " ###### " << instruction << " ###### " << instructionID << "\n";
    }
  }
  trace_id_steam->close();
}

bool Tracer::runOnFunction(Function &F, std::vector<std::string> &target) {
  bool func_modified = false;
  std::map<BasicBlock*, u16> recorded_bb;
  recorded_bb.clear();

  for (auto bb_it = F.begin(); bb_it != F.end(); ++bb_it) {
    BasicBlock &bb = *bb_it;
    bool trace_bb = false;
    for (BasicBlock::iterator itr = bb.begin(); itr != bb.end(); ++itr) {
      getDebugLoc(cast<Instruction>(itr), filename, line, column);
      if (filename.empty() || line == 0)
        continue;
      std::size_t found = filename.find_last_of("/\\");
      if (found != std::string::npos)
        filename = filename.substr(found + 1);
      auto findline = find(begin(target), end(target),
                           filename + ':' + std::to_string(line));
      if (findline != std::end(target)) {
        trace_bb = true;
        target.erase(findline);
        break;
      }
    }
    if (trace_bb) {
      func_modified = runOnBasicBlock(bb, target);

      u16 cnt_SuccBB = 0;
      Instruction *LastInst = bb.getTerminator();
      while (LastInst && !isa<CallInst>(LastInst)) {
        LastInst = LastInst->getPrevNode();
      }
      
      if (LastInst && isa<CallInst>(LastInst)) {
        CallInst *Call = dyn_cast<CallInst>(LastInst);
        Function *called_func = Call->getCalledFunction();
        if (!called_func){
          cnt_SuccBB = 16;
        }
      }

      for (succ_iterator SI = succ_begin(&bb), E = succ_end(&bb); SI != E; ++SI) {
        BasicBlock *SuccBB = *SI;
        if (recorded_bb.find(SuccBB) == recorded_bb.end())
          recorded_bb[SuccBB] = 0;
        cnt_SuccBB+=1;
      }
      recorded_bb[&bb] = cnt_SuccBB;
    }
  }

  for(auto it=recorded_bb.begin(); it!=recorded_bb.end(); it++)	{
    BasicBlock::iterator IP = (*(it->first)).getFirstInsertionPt();
    IRBuilder<> IRB(&(*IP));
    u16 cnt_SuccBB =it->second;

    Value *v_bb_id = ConstantInt::get(IRB.getInt32Ty(), bb_id);
    Value *v_bb_startIndex = ConstantInt::get(IRB.getInt32Ty(), bb_startIndex);
    Value *v_cnt_succ = ConstantInt::get(IRB.getInt16Ty(), cnt_SuccBB);
    Value *args[] = {v_bb_id,v_bb_startIndex,v_cnt_succ};
    IRB.CreateCall(TL_trace_edge, args);

    bb_startIndex += cnt_SuccBB;
    bb_id+=1;
  }

  return func_modified;
}

bool Tracer::runOnBasicBlock(BasicBlock &BB, std::vector<std::string> &target) {
  // We have to get the first insertion point before we insert any
  // instrumentation!
  BasicBlock::iterator insertp = BB.getFirstInsertionPt();
  BasicBlock::iterator itr = BB.begin();

  if (isa<PHINode>(itr)) {
    getDebugLoc(cast<Instruction>(itr), filename, line, column);
    handlePhiNodes(&BB);
  }

  // From this point onwards, nodes cannot be PHI nodes.
  BasicBlock::iterator nextitr;
  for (BasicBlock::iterator itr = insertp; itr != BB.end(); itr = nextitr) {

    nextitr = itr;
    nextitr++;

    getDebugLoc(cast<Instruction>(itr), filename, line, column);

    // Invoke instructions are used to call functions that may throw exceptions.
    // They are the only the terminator instruction that can also return a
    // value. 
    if (isa<InvokeInst>(*itr))
    {
      continue;
    }

    Instruction *currInst = cast<Instruction>(itr);
    if (ZExtInst *Izext = dyn_cast<ZExtInst>(currInst))
      continue;
    
    if (auto bitcast_I = dyn_cast<BitCastInst>(currInst))
      continue;
    
    if (CallInst *I = dyn_cast<CallInst>(currInst)) {
      Function *called_func = I->getCalledFunction();
      if (!called_func || called_func->isIntrinsic()) {
        continue;
      }

      handleCallInstruction(currInst);
    }else {
      handleNonPhiNonCallInstruction(currInst);
    }

    if (!currInst->getType()->isVoidTy()) {
      Instruction *nextInst = cast<Instruction>(nextitr);
      handleInstructionResult(currInst, nextInst);
    }
    
    if (auto *opInst = dyn_cast<BinaryOperator>(currInst)) {
      // Insert inline assembly to read EFLAGS register
      //*trace_id_steam << "\ntrace_register\n" << register_inst_id << "\n";
      //currInst->print(*trace_id_steam);
      std::string inst_info; 
      raw_string_ostream stream(inst_info);
      currInst->print(stream);
      uint32_t id = getInstructionID(inst_info, filename, line);

      LLVMContext &Context = currInst->getFunction()->getContext();
      IRBuilder<> IRB(currInst);
      IRB.SetInsertPoint(currInst->getParent(), ++currInst->getIterator());
      InlineAsm *Asm =
          InlineAsm::get(FunctionType::get(Type::getInt64Ty(Context), {}, false),
                        "pushfq\n\t"
                        "pop $0",           // inline asm
                        "=r",               // constraints
                        false,              // Has side effects
                        false,              // Is align stack
                        InlineAsm::AD_Intel // Intel assembly syntax
          );
      ArrayRef<Value *> Args = None;
      CallInst *v_result = IRB.CreateCall(Asm, Args);

      Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), id);//inst_id-1);
      Value *args[] = {v_result, v_inst_id};
      IRB.CreateCall(TL_trace_register, args);
    }
  }

  return true;
}

uint32_t Tracer::getInstructionID(std::string instructionInfo, std::string Filename, unsigned Line) {
  std::string sourceCodeInfo = Filename + ':' + std::to_string(Line);
  uint32_t id = 0;

  if (inst_id_map.find(sourceCodeInfo) != inst_id_map.end() && inst_id_map[sourceCodeInfo].find(instructionInfo) != inst_id_map[sourceCodeInfo].end()) {
    id = inst_id_map[sourceCodeInfo][instructionInfo];
    hasBeenInstrumented = true;
  } else {
    id = inst_id;
    inst_id = (inst_id + 1) % INST_SIZE;
    inst_id_map[sourceCodeInfo][instructionInfo] = id;
    hasBeenInstrumented = false;
  }
  return id;
}

void Tracer::handleCallInstruction(Instruction *inst) {
  CallInst *CI = dyn_cast<CallInst>(inst);
  Function *fun = CI->getCalledFunction();
  int call_id = 0;
  Value *value, *curr_operand;
  for (auto arg_it = fun->arg_begin(); arg_it != fun->arg_end();
       ++arg_it, ++call_id) {
    curr_operand = inst->getOperand(call_id);
    if (Instruction *I = dyn_cast<Instruction>(curr_operand)) {
      value = curr_operand;
    } else {
      if (curr_operand->getType()->isLabelTy()) {
        continue;
      } else if (curr_operand->getValueID() == Value::FunctionVal) {
        continue;
      } else {
        value = curr_operand;
      }
    }
    if (isa<Constant>(*value))
      continue;

    //outs() << "handleCallInstruction: inst_id is " << inst_id << "\n";
    //*trace_id_steam << "\nhandleCallInstruction: inst_id is " << inst_id;
    

    IRBuilder<> IRB(inst);
    Value *v_value;
    if (value->getType()->isMetadataTy())
      return;
    if (value->getType()->isPointerTy())
      v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
    else if (value->getType()->isFloatingPointTy())
      v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
    else
      v_value = IRB.CreateZExtOrTrunc(value, IRB.getInt64Ty());

    //Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), inst_id);

    //*trace_id_steam << "\n" << inst_id << "\n";
    //value->print(*trace_id_steam);
    std::string inst_info; 
    raw_string_ostream stream(inst_info);
    value->print(stream);
    uint32_t id = getInstructionID(inst_info, filename, line);
    if(!hasBeenInstrumented){
      Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), id);
    
      Value *args[] = {v_value, v_inst_id};
      IRB.CreateCall(TL_trace_value, args);
      //inst_id = (inst_id + 1) % INST_SIZE;
    }
  }
}

void Tracer::handleNonPhiNonCallInstruction(Instruction *inst) {
  int num_of_operands = inst->getNumOperands();
  if (num_of_operands > 0) {
    for (int i = num_of_operands - 1; i >= 0; i--) {
      Value *curr_operand = inst->getOperand(i);
      Value *value;

      if (Instruction *I = dyn_cast<Instruction>(curr_operand)) {
        value = curr_operand;
      } else {
        if (curr_operand->getType()->isVectorTy()) {
          value = curr_operand;
        } else if (curr_operand->getType()->isLabelTy()) {
          continue;
        } else if (curr_operand->getValueID() == Value::FunctionVal) {
          continue;
        } else {
          value = curr_operand;
        }
      }
      if (isa<Constant>(*value))
        continue;

      //outs() << "handleNonPhiNonCallInstruction: inst_id is " << inst_id << "\n";
      //*trace_id_steam << "\nhandleNonPhiNonCallInstruction: inst_id is " << inst_id;

      IRBuilder<> IRB(inst);
      Value *v_value;
      if (value->getType()->isMetadataTy())
        return;
      if (value->getType()->isPointerTy())
        v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
      else if (value->getType()->isFloatingPointTy())
        v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
      else
        v_value = IRB.CreateZExtOrTrunc(value, IRB.getInt64Ty());

      //Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), inst_id);
      
      //*trace_id_steam << "\n" << inst_id << "\n";
      //value->print(*trace_id_steam);
      std::string inst_info; 
      raw_string_ostream stream(inst_info);
      value->print(stream);
      uint32_t id = getInstructionID(inst_info, filename, line);
      if(!hasBeenInstrumented){
        Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), id);

        Value *args[] = {v_value, v_inst_id};
        IRB.CreateCall(TL_trace_value, args);
        //inst_id = (inst_id + 1) % INST_SIZE;
      }
    }
  }
}

// Handle all phi nodes at the beginning of a basic block.

void Tracer::handlePhiNodes(BasicBlock *BB) {
  BasicBlock::iterator insertp = BB->getFirstInsertionPt();
  Instruction *insertPointInst = cast<Instruction>(insertp);

  Value *v_value;
  Value *curr_operand = nullptr;
  for (BasicBlock::iterator itr = BB->begin(); isa<PHINode>(itr); itr++) {
    Instruction *currInst = cast<Instruction>(itr);
    if (ZExtInst *Izext = dyn_cast<ZExtInst>(currInst))
      continue;
    
    if (auto bitcast_I = dyn_cast<BitCastInst>(currInst))
      continue;
    
    // Print each operand.
    int num_of_operands = currInst->getNumOperands();
    if (num_of_operands > 0) {
      for (int i = num_of_operands - 1; i >= 0; i--) {
        curr_operand = currInst->getOperand(i);

        if (Instruction *I = dyn_cast<Instruction>(curr_operand)) {
          //
        } else {
          Value *value = curr_operand;
          if (isa<Constant>(*value))
            continue;

          //outs() << "handlePhiNodes-operands: inst_id is " << inst_id << "\n";
          //*trace_id_steam << "\nhandlePhiNodes-operands: inst_id is " << inst_id;

          IRBuilder<> IRB(insertPointInst);

          if (value->getType()->isMetadataTy())
            return;
          if (value->getType()->isPointerTy())
            v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
          else if (value->getType()->isFloatingPointTy())
            v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
          else
            v_value = IRB.CreateZExtOrTrunc(value, IRB.getInt64Ty());

          //Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), inst_id);
          
          //*trace_id_steam << "\n" << inst_id << "\n";
          //value->print(*trace_id_steam);

          std::string inst_info; 
          raw_string_ostream stream(inst_info);
          value->print(stream);
          uint32_t id = getInstructionID(inst_info, filename, line);
          if(!hasBeenInstrumented){
            Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), id);

            Value *args[] = {v_value, v_inst_id};
            IRB.CreateCall(TL_trace_value, args);
            //inst_id = (inst_id + 1) % INST_SIZE;
          }
        }
      }
    }

    // Print result line.

    if (!currInst->getType()->isVoidTy()) {
      if (currInst->isTerminator()) {
        assert(false && "It is terminator...\n");
      } else {
        Value *value = currInst;
        if (isa<Constant>(*value))
          continue;
        //outs() << "handlePhiNodes-result: inst_id is " << inst_id << "\n";
        //*trace_id_steam << "\nhandlePhiNodes-result: inst_id is " << inst_id;

        IRBuilder<> IRB(insertPointInst);

        if (value->getType()->isMetadataTy())
          return;
        if (value->getType()->isPointerTy())
          v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
        else if (value->getType()->isFloatingPointTy())
          v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
        else
          v_value = IRB.CreateZExtOrTrunc(value, IRB.getInt64Ty());

        //Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), inst_id);
        
        //*trace_id_steam << "\n" << inst_id << "\n";
        //value->print(*trace_id_steam);

        std::string inst_info; 
        raw_string_ostream stream(inst_info);
        value->print(stream);
        uint32_t id = getInstructionID(inst_info, filename, line);
        if(!hasBeenInstrumented){
          Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), id);

          Value *args[] = {v_value, v_inst_id};
          IRB.CreateCall(TL_trace_value, args);
          //inst_id = (inst_id + 1) % INST_SIZE;
        }
      }
    }
  }
}

void Tracer::handleInstructionResult(Instruction *inst,
                                     Instruction *next_inst) {
  if (inst->isTerminator()) {
    assert(false);
  } else {
    Value *value = inst;

    if (isa<Constant>(*value))
      return;
    /*
    int num_of_operands = next_inst->getNumOperands();
    for (int i = 0; i < num_of_operands; i++) {
      Value *next_operand = next_inst->getOperand(i);
      if (next_operand == value)
        return;
    }
    */

    //outs() << "handleInstructionResult: inst_id is " << inst_id << "\n";
    //*trace_id_steam << "\nhandleInstructionResult: inst_id is " << inst_id;

    IRBuilder<> IRB(next_inst);
    Value *v_value;
    if (value->getType()->isMetadataTy())
      return;
    if (value->getType()->isPointerTy())
      v_value = IRB.CreatePtrToInt(value, IRB.getInt64Ty());
    else if (value->getType()->isFloatingPointTy())
      v_value = IRB.CreateFPExt(value, IRB.getDoubleTy());
    else
      v_value = IRB.CreateZExtOrTrunc(value, IRB.getInt64Ty());

    //Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), inst_id);

    //*trace_id_steam << "\n" << inst_id << "\n";
    //value->print(*trace_id_steam);

    std::string inst_info; 
    raw_string_ostream stream(inst_info);
    value->print(stream);
    uint32_t id = getInstructionID(inst_info, filename, line);
    if(!hasBeenInstrumented){
      Value *v_inst_id = ConstantInt::get(IRB.getInt64Ty(), id);

      Value *args[] = {v_value, v_inst_id};
      IRB.CreateCall(TL_trace_value, args);
      //inst_id = (inst_id + 1) % INST_SIZE;
    }
  }
}

char Tracer::ID = 0;
