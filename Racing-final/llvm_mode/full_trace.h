#include <fstream>
#include <map>
#include <set>
#include <string>

#include "../config.h"
#include "../debug.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Pass.h"

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

extern char s_phi[];

using namespace llvm;

class Tracer : public ModulePass {
public:
  Tracer();
  ~Tracer(){}

  virtual bool doInitialization(Module &M);
  void dofinish();
  bool runOnFunction(Function &F,std::vector<std::string> &target);
  bool runOnBasicBlock(BasicBlock &BB,std::vector<std::string> &target);
  uint32_t getInstructionID(std::string instructionInfo, std::string Filename, unsigned Line);

  // Instrumentation functions for different types of nodes.
  void handlePhiNodes(BasicBlock *BB);
  void handleInstructionResult(Instruction *inst, Instruction *next_inst);
  void handleCallInstruction(Instruction *inst, Instruction *prevInst);
  void handleNonPhiNonCallInstruction(Instruction *inst, Instruction *prevInst);

  // References to the logging functions.
  Value *TL_trace_value;
  Value *TL_trace_register;
  Value *TL_trace_edge;
  uint32_t inst_id;
  uint32_t bb_id;
  uint32_t bb_startIndex;
  std::string base_dir;

  std::string filename;
  unsigned line, column;
  bool hasBeenInstrumented;
  std::map<std::string, std::map<std::string, int>> inst_id_map;
  struct BBInfo {
    std::string firstInst;
    std::string lastInst;
  };
  std::map<uint32_t, struct BBInfo> bb_id_map;
  static char ID;
  
};
