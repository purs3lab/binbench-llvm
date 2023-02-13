//
// Created by machiry on 2/10/23.
//

// This file contains all types use BCollector API
// also includes method to update the types.
#ifndef LLVM_BCOLLECTORTYPES_H
#define LLVM_BCOLLECTORTYPES_H

namespace llvm {
// Type of Machine Basic Block
typedef enum MachineBasicBlocksInfoType { NOTEND = 0, END = 1,
                                          ENDOFOBJECT = 2 } MBBINFOTYPE;

typedef std::string MBBIDTYPE;
typedef std::string MFIDTYPE;

// Class that contains information about Machine Functions
class BMachineFunctionInfo {
public:
  unsigned TotalSizeInBytes;

  std::string FunctionName;

  BMachineFunctionInfo() {
    TotalSizeInBytes = 0;
    FunctionName = "";
  }
  virtual ~BMachineFunctionInfo() {
  }
};

typedef std::map<MFIDTYPE, BMachineFunctionInfo> MFCONTAINER;

// Class that contains information about BasicBlocks

class MachineBasicBlocksInfo {
public:
  unsigned TotalSizeInBytes;
  unsigned Offset;
  unsigned NumArgs;
  unsigned NumFixUps;
  unsigned Alignments;

  // what is the type of this basic block.
  MBBINFOTYPE BBType;

  std::string SectionName;

  // Set of ids of predecessors of this basic blocks.
  std::set<std::string> Predecessors;

  // Set of ids of sucessesors of this basic block.
  std::set<std::string> Successors;

  MachineBasicBlocksInfo() {
    TotalSizeInBytes = Offset = NumArgs = NumFixUps = 0;
    Alignments = 0;
    BBType = MBBINFOTYPE::NOTEND;
    SectionName = "";
    Predecessors.clear();
    Successors.clear();
  }
  virtual ~MachineBasicBlocksInfo() {
    Predecessors.clear();
    Successors.clear();
  }
};

typedef std::map<MBBIDTYPE, MachineBasicBlocksInfo> MBBCONTAINER;
} // namespace llvm

#endif // LLVM_BCOLLECTORTYPES_H
