//
// Created by machiry on 2/10/23.
//

// This file contains all types use BCollector API
// also includes method to update the types.
#ifndef LLVM_BCOLLECTORTYPES_H
#define LLVM_BCOLLECTORTYPES_H

#include "llvm/BCollector/BCollectorUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/shuffleInfo.pb.h"

namespace llvm {

/// Type of Machine Basic Block
typedef enum MachineBasicBlocksInfoType { NOTEND = 0, END = 1,
                                          ENDOFOBJECT = 2 } MBBINFOTYPE;

/// @brief machine basic block ID
typedef std::string MBBIDTYPE;
/// @brief machine function ID
typedef std::string MFIDTYPE;

/// @brief Jump Table Type: <JumpTableID, <JumpTableSize, <JumpTableEntries>>>
typedef std::map<std::string, std::tuple<unsigned, unsigned, std::list<std::string>>> JTTYPEWITHID;

/// @brief Fixup List Type: <offset, size, isRela, parentID, SymbolRefFixupName, isNewSection,
///    secName, numJTEntries, JTEntrySz>
typedef std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string,
                           bool, std::string, unsigned, unsigned>> FIXUPTYPE;


/// Class that contains information about Machine Functions.
class BMachineFunctionInfo {
public:

  unsigned TotalSizeInBytes; ///< Total size of the function in bytes.

  std::string FunctionName; ///< Name of the function.
  std::string ID; ///< Machine Function ID

  int NumArgs; ///< Number of arguments to the function.
  std::vector<unsigned> ArgSizesInBits;  ///< list of the sizes of the arguments in bits.

  BMachineFunctionInfo() {
    TotalSizeInBytes = 0;
    FunctionName = "";
  }
  virtual ~BMachineFunctionInfo() {
  }
};

/// @brief Container for BMachineFunctionInfo.
typedef std::map<MFIDTYPE, BMachineFunctionInfo> MFCONTAINER;

/// Class that contains information about BasicBlocks.
class MachineBasicBlocksInfo {

public:

  unsigned TotalSizeInBytes; ///< Total size of the basic block in bytes.
  unsigned Offset; ///< Offset of the basic block in the object file.
  unsigned NumArgs; ///< Deprecated, to be removed.
  unsigned NumFixUps; ///< Number of fixups in the basic block.
  unsigned Alignments; ///< Alignment of the basic block.

  MBBINFOTYPE BBType; ///< Type can be NOTEND, END or ENDOFOBJECT.

  /// @brief Name of the section in which this basic block is present.
  std::string SectionName;

  std::set<std::string> Predecessors; ///< Set of ids of predecessors of this basic blocks.

  std::set<std::string> Successors; ///< Set of ids of sucessesors of this basic block.

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

/// @brief Container for MachineBasicBlocksInfo.
typedef std::map<MBBIDTYPE, MachineBasicBlocksInfo> MBBCONTAINER;
} // namespace llvm

#endif // LLVM_BCOLLECTORTYPES_H
