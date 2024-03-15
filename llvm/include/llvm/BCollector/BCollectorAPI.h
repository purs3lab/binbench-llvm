// Base class for backend metadata collectors
#ifndef LLVM_BCOLLECTOR_H
#define LLVM_BCOLLECTOR_H

#include "llvm/BCollector/BCollectorUtils.h"
#include "llvm/BCollector/BCollectorTypes.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/shuffleInfo.pb.h"

#include <algorithm>  // required for std::find
#include <string>
#include <iomanip>

namespace llvm {

/**
 * This class provides a common interface for all backend collectors.
 */
class BCollector {
public:
  BCollectorUtils *Utils; ///< Utility class for BCollector

  MBBCONTAINER
      MachineBasicBlocks; ///< Information about all machine basic blocks.

  MFCONTAINER MachineFunctions; ///< Information about all machine functions.

  std::map<std::string, std::list<std::string>>
      VTables; ///< Map of vtables to their entries.
  mutable std::map<std::string, std::string>
      NametoMFID; ///< Map from function name to function ID.
  mutable std::map<std::string, bool>
      CanMbbFallThrough; ///< Map from MBB ID to whether it can fall through to
                         ///< the next MBB.
  mutable std::map<unsigned, unsigned>
      MachineFunctionSizes; ///< Map from Machine Function ID to its size.
  mutable std::list<std::string>
      MBBLayoutOrder; ///< List of MBB IDs in the order they appear in the
                      ///< binary.
  mutable std::map<std::string, std::list<std::string>>
      CallGraphInfo; ///< list of funcs which maps from Function to its
                     ///< successors

  mutable std::vector<unsigned>
      SeenFunctionIDs; ///< List of function IDs that have been seen.

  const char *FixupLookupSections[5] = {
      ".text", ".rodata", ".data", ".data.rel.ro", ".init_array",
  }; ///< List of sections that can have fixups.

  /// @brief Dump collected Fixup information. This is used for debugging.
  void dumpFixups(FIXUPTYPE &Fixups, const std::string &Kind, bool IsDebug);

  /// @brief Updates the ReorderInfo object with the collected metadata.
  /// @param Layout The MCAsmLayout object that contains required metadata.
  void updateReorderInfoValues(const MCAsmLayout &Layout);

  /// @brief get basic block id and machine function id separated
  std::tuple<int, int> separateID(std::string ID) {
    return std::make_tuple(std::stoi(ID.substr(0, ID.find("_"))),
                           std::stoi(ID.substr(ID.find("_") + 1, ID.length())));
  }
  
  //map of Jump table entries and their corresponding targets
  mutable std::map<std::string, std::tuple<unsigned, unsigned, std::list<std::string>>> JumpTableTargets;
  std::string FixupParentID;
  bool IsJumpTableRef = false;
  std::string SymbolRefFixupName;

  const std::string &getFixupParentID() const { return FixupParentID; }
  void setFixupParentID(const std::string &Value) { FixupParentID = Value; }
  bool getIsJumpTableRef() const { return IsJumpTableRef; }
  void setIsJumpTableRef(bool V) { IsJumpTableRef = V; }
  const std::string &getSymbolRefFixupName() const {
    return SymbolRefFixupName;
  }
  void setSymbolRefFixupName(const std::string &FN) { SymbolRefFixupName = FN; }
  /// @brief Set Fixup information for a given section. Users need not use this.
  void setFixups(FIXUPTYPE &Fixups,
                 ShuffleInfo::ReorderInfo_FixupInfo *FixupInfo,
                 const std::string &SecName);

  /// @brief get the section id for a given section name.
  int getFixupSectionId(const std::string &SecName) {
    for (size_t I = 0;
         I < sizeof(FixupLookupSections) / sizeof(*FixupLookupSections); ++I)
      if (SecName.compare(FixupLookupSections[I]) == 0)
        return I;
    return -1;
  }

  /// @brief Get the FixupTuple object for a given section.
  ShuffleInfo::ReorderInfo_FixupInfo_FixupTuple *
  getFixupTuple(ShuffleInfo::ReorderInfo_FixupInfo *FI,
                const std::string &SecName) {
    switch (getFixupSectionId(SecName)) {
    case 0:
      return FI->add_text();
    case 1:
      return FI->add_rodata();
    case 2:
      return FI->add_data();
    case 3:
      return FI->add_datarel();
    case 4:
      return FI->add_initarray();
    default:
      llvm_unreachable("[CCR-Error] ShuffleInfo::getFixupTuple - No such "
                       "section to collect fixups!");
    }
  }

  /// @brief Deprecated
  void getMetadata() { llvm_unreachable("getMetadata() is not implemented"); }

  /// @brief Deprecated
  void setMetadata() { llvm_unreachable("setMetadata() is not implemented"); }

  /// @brief Deprecated
  void printMetadata() {
    llvm_unreachable("printMetadata() is not implemented");
  }

  /// @brief Serialize the collected metadata into a protobuf object.
  /// @param ri The ReorderInfo object to be serialized.
  /// @param Layout Required to update the ReorderInfo object.
  void serializeReorderInfo(ShuffleInfo::ReorderInfo *Ri,
                            const MCAsmLayout &Layout);

  /// @brief Add seen function ID to the list.
  void updateSeenFuncs(unsigned FuncId) const {
    if (std::find(SeenFunctionIDs.begin(), SeenFunctionIDs.end(), FuncId) ==
        SeenFunctionIDs.end())
      SeenFunctionIDs.push_back(FuncId);
  }

  /// @brief Check if we have already seen this function ID.
  bool isSeenFuncs(unsigned FuncId) const {
    if (std::find(SeenFunctionIDs.begin(), SeenFunctionIDs.end(), FuncId) ==
        SeenFunctionIDs.end())
      return false;
    return true;
  }
  /**
   * @name Section Fixup information
   */
  ///@{
  /** Fixup information in each section. */
  mutable FIXUPTYPE FixupsText, FixupsRodata, FixupsData, FixupsDataRel,
      FixupsInitArray;
  ///@}

  mutable std::string LatestParentId;   ///< The latest parent ID.
  mutable std::string LatestFunctionId; ///< The latest function ID.
  mutable unsigned Nargs; ///< The number of arguments for the latest function.

  mutable bool IsAssemFile = false; ///< Is the current file assembly?
  mutable bool HasInlineAssembly =
      false; ///< Does the current function have inline assembly?
  mutable std::string
      PrevOpcode; ///< The previous opcode encounted in collection.
  mutable unsigned AssemFuncNo =
      0xffffffff; ///< The function number for the current assembly function.
  mutable unsigned AssemBblNo =
      0; ///< The basic block number for the current assembly basic block.
  mutable unsigned SpecialCntPriorToFunc = 0; ///< Unused and deprecated?

  /// Dump the collected jump table information.
  void dumpJT(const JTTYPEWITHID &JumpTables, const MCAsmInfo *MAI);

  /// Process a fragment and collect fixup information.
  void processFragment(MCSection &Sec, const MCAsmLayout &Layout,
                       const MCAsmInfo *MAI, const MCObjectFileInfo *MOFI,
                       MCSectionELF &ELFSec);

  void updateJumpTableTargets(const std::string &Key, unsigned EntryKind,
                              unsigned EntrySize,
                              const std::list<std::string> &JTEntries) const {
    JumpTableTargets[Key] = std::make_tuple(EntryKind, EntrySize, JTEntries);
  }

  void incrementJTE(const std::string &Key) const {
    for (auto& Item : FixupsText) {
        if (std::get<4>(Item) == Key) {
            std::get<7>(Item)++; 
            break; 
        }
    }
  }

  const std::tuple<unsigned, unsigned, std::list<std::string>> &
  getJumpTableTargetEntry(const std::string &Key) const {
    return JumpTableTargets[Key];
  }

  const std::map<std::string,
                 std::tuple<unsigned, unsigned, std::list<std::string>>> &
  getJumpTableTargets() const {
    return JumpTableTargets;
  }
  /// @brief Set the latest parent ID.
  void setFunctionid(const std::string &Id) { LatestFunctionId = Id; }

  /// @brief Set the Fixup info from each section
  void setFixupInfo(ShuffleInfo::ReorderInfo_FixupInfo *FixupInfo,
                    const MCAsmInfo *MAI);
  explicit BCollector();
  virtual ~BCollector() {}
};

/// @brief Collects basic block information.
class BasicBlockCollector : public BCollector {
public:
  /// Collects the basic block information. Includes things like:
  /// ID, size, alignment, fixups, successors, predecessors.
  /// @param MI The Machine Intruction from which metadata is to be collected.
  /// @param Inst The MC Layer instruction object into which the collected info
  /// is embedded into.
  virtual void performCollection(const MachineInstr *MI, MCInst *Inst);

  /// Updates the sizes and fixup info of basic blocks.
  /// @param id The ID of the basic block to be updated.
  /// @param emittedBytes The number of bytes that need to be updated.
  /// @param numFixups The number of fixups that need to be updated.
  /// @param isAlign Is the update due to an alignment?
  /// @param isInline Is the update due to an inline assembly? unused.
  void updateByteCounter(MBBIDTYPE &Id, unsigned EmittedBytes,
                         unsigned NumFixups, bool IsAlign, bool IsInline) {

    MachineBasicBlocks[Id].TotalSizeInBytes +=
        EmittedBytes;                              // Acutal size in MBB
    MachineBasicBlocks[Id].NumArgs = Nargs;        // Acutal size in MBB
    MachineBasicBlocks[Id].NumFixUps += NumFixups; // Number of Fixups in MBB

    if (IsAlign)
      MachineBasicBlocks[Id].Alignments += EmittedBytes; // Count NOPs in MBB

    // If inlined, add the bytes in the next MBB instead of current one
    if (IsInline)
      MachineBasicBlocks[Id].TotalSizeInBytes -= EmittedBytes;
  }

  /// @brief  Sets the successors of a basic block.
  /// @param id The ID of the basic block to be updated.
  /// @param succs Successor IDs of the basic block.
  void setSuccs(const std::string &Id, const std::set<std::string> &Succs) {
    MachineBasicBlocks[Id].Successors = Succs;
  }

  /// @brief Sets the predecessors of a basic block.
  /// @param id  The ID of the basic block to be updated.
  /// @param preds  Predecessor IDs of the basic block.
  void setPreds(const std::string &Id, const std::set<std::string> &Preds) {
    MachineBasicBlocks[Id].Predecessors = Preds;
  }
  BasicBlockCollector() {}
  virtual ~BasicBlockCollector() {}
};

// Adds a v table entry for the particular class name. 
class ClassCollector : public BCollector {
public:
  void addVTable(const std::string &Classname,
                 const std::list<std::string> &VtableEntries) {
    VTables[Classname] = VtableEntries;
  }
};

// A lot of the stuff here comes from DWRAF
// -gdwarf flag is absolutely necessary!
/// @brief  Collects the function information.
class FunctionCollector : public BCollector {
public:
  /// @brief  Collects the function information. Includes things like:
  /// ID, function name, arguments (if any)
  void updateFuncDetails(const std::string &Id, const std::string &Funcname,
                         unsigned Size) {
    MachineFunctions[Id].TotalSizeInBytes = Size;
    MachineFunctions[Id].FunctionName = Funcname;
    MachineFunctions[Id].ID = Id;
    NametoMFID[Funcname] = Id;
  }

  const auto &getVT() { return VTables; }

  const std::string
  getDICompositeTypeKind(const DICompositeType *CompositeType) {
    if (CompositeType == nullptr) {
      // might want to log an error message here.
      return "Invalid";
    }

    unsigned Tag = CompositeType->getTag();

    switch (Tag) {
    case dwarf::DW_TAG_array_type:
      return "Array";
    case dwarf::DW_TAG_structure_type:
      return "Structure";
    case dwarf::DW_TAG_union_type:
      return "Union";
    case dwarf::DW_TAG_enumeration_type:
      return "Enum";
    case dwarf::DW_TAG_class_type:
      return "Class";
    default:
      return "Unknown";
    }
  }

  const std::string checkDIType(const DIType *TheDIType) {
    std::string TypeName = "";
    if (const DIBasicType *BasicType = dyn_cast<DIBasicType>(TheDIType)) {
      // TheDIType is a DIBasicType, and BasicType is of type DIBasicType*
      TypeName = BasicType->getName().str();
    } else if (const DICompositeType *CompositeType =
                   dyn_cast<DICompositeType>(TheDIType)) {
      // TheDIType is a DICompositeType, and CompositeType is of type
      // DICompositeType*
      TypeName = getDICompositeTypeKind(CompositeType);
    } else {
      TypeName = "Pointer";
    }
    return TypeName;
  }
  /// @brief Update the argument details of a function
  /// @param funcname Name of the function to be updated
  /// @param numArgs Number of arguments observed
  void updateArgDetails(const std::string &Funcname, int NumArgs) {

    // check if name is in the map
    if (NametoMFID.find(Funcname) == NametoMFID.end()) {
      DEBUG_WITH_TYPE("binbench",
                      dbgs() << "Function not found: " << Funcname << "\n");
      return;
    }
    DEBUG_WITH_TYPE("binbench",
                    dbgs() << "Function has numArgs: " << NumArgs << "\n");
    MachineFunctions[NametoMFID[Funcname]].NumArgs = NumArgs;
  }

  void updateArgTypes(const std::string &Funcname, const std::string &ArgType) {

    // check if name is in the map
    if (NametoMFID.find(Funcname) == NametoMFID.end()) {
      DEBUG_WITH_TYPE("binbench",
                      dbgs() << "Function not found: " << Funcname << "\n");
      return;
    }

    MachineFunctions[NametoMFID[Funcname]].ArgTypes.push_back(ArgType);
  }

  void addLocalVariable(const std::string &Funcname, const std::string &Varname,
                        const std::string &Type, int Offset, unsigned Size) {
    MachineFunctions[NametoMFID[Funcname]].LocalVars[Varname] =
        std::make_tuple(Type, Offset, Size);
  }

  void addCGSuccessor(const std::string &Caller, const std::string &Callee) {
    const auto &It = CallGraphInfo.find(Caller);
    if (It == CallGraphInfo.end()) {
      std::list<std::string> NewList{Callee};
      CallGraphInfo[Caller] = NewList;
    } else {
      // Check if callee already exists
      auto &CallerList = CallGraphInfo[Caller];
      if (std::find(CallerList.begin(), CallerList.end(), Callee) ==
          CallerList.end()) {
        // If callee doesn't exist, push it back to the list.
        CallerList.push_back(Callee);
      }
    }
  }

  void addVTableEntry(const std::string &Classname, const std::string &Entry) {
    const auto &It = VTables.find(Classname);
    if (It == VTables.end()) {
        std::list<std::string> NewList {Entry};
        VTables[Classname] = NewList;
    } else {
        auto &EntriesList = VTables[Classname];
        if (std::find(EntriesList.begin(), EntriesList.end(), Entry) == EntriesList.end()) {
            EntriesList.push_back(Entry);
        }
    }
  }

  /// @brief Get collected function metadata object.
  MFCONTAINER &getMFs() { return MachineFunctions; }

  auto &getCG() {return CallGraphInfo;}

  /// @brief To set the number of arguments for a function internally and for
  /// debugging.
  void setNumArgs(const std::string &Funcname, unsigned NumArgs) {
    Nargs = NumArgs;
    DEBUG_WITH_TYPE("binbench", dbgs() << "NumArgs: " << Nargs << "\n");
    DEBUG_WITH_TYPE("binbench", dbgs() << "funcname: " << Funcname << "\n");
  }

  /// @brief Update the argument sizes of a function
  /// @param funcname Name of the function to be updated
  /// @param argsize The size of the argument in bits
  void addArgSizes(const std::string &Funcname, unsigned Argsize) {
    DEBUG_WITH_TYPE("binbench", dbgs() << "For Func " << Funcname
                                       << "ArgSize: " << Argsize << "\n");
    MachineFunctions[NametoMFID[Funcname]].ArgSizesInBits.push_back(Argsize);
  }

  FunctionCollector() {}
  virtual ~FunctionCollector() {}
};

} // namespace llvm

#endif // LLVM_BCOLLECTOR_H
