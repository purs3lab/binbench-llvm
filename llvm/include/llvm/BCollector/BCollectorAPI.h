// Base class for backend metadata collectors
#ifndef LLVM_BCOLLECTOR_H
#define LLVM_BCOLLECTOR_H

#include "llvm/BCollector/BCollectorUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/DebugInfoMetadata.h"
// #include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/shuffleInfo.pb.h"

#include "llvm/BCollector/BCollectorTypes.h"
#include <iomanip>
#include <string>
#include <algorithm>  // required for std::find


namespace llvm {

/**
 * This class provides a common interface for all backend collectors.
 */
class BCollector {
public:
  BCollectorUtils *utils; ///< Utility class for BCollector

  MBBCONTAINER
      MachineBasicBlocks; ///< Information about all machine basic blocks.

  MFCONTAINER MachineFunctions; ///< Information about all machine functions.

  std::map<std::string, std::list<std::string>>
      vTables; ///< Map of vtables to their entries.
  mutable std::map<std::string, std::string>
      NametoMFID; ///< Map from function name to function ID.
  mutable std::map<std::string, bool>
      canMBBFallThrough; ///< Map from MBB ID to whether it can fall through to
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

  const char *fixupLookupSections[5] = {
      ".text", ".rodata", ".data", ".data.rel.ro", ".init_array",
  }; ///< List of sections that can have fixups.

  /// @brief Dump collected Fixup information. This is used for debugging.
  void dumpFixups(FIXUPTYPE &Fixups, const std::string &kind, bool isDebug);

  /// @brief Updates the ReorderInfo object with the collected metadata.
  /// @param Layout The MCAsmLayout object that contains required metadata.
  void updateReorderInfoValues(const MCAsmLayout &Layout);

  /// @brief get basic block id and machine function id separated
  std::tuple<int, int> separateID(std::string ID) {
    return std::make_tuple(std::stoi(ID.substr(0, ID.find("_"))),
                           std::stoi(ID.substr(ID.find("_") + 1, ID.length())));
  }

  mutable std::map<std::string, std::tuple<unsigned, unsigned, std::list<std::string>>> JumpTableTargets;
  std::string FixupParentID;
  bool isJumpTableRef = false;
  std::string SymbolRefFixupName;


  std::string getFixupParentID() const { return FixupParentID; }
  void setFixupParentID(std::string Value) { FixupParentID = Value; }
  bool getIsJumpTableRef() const { return isJumpTableRef; }
  void setIsJumpTableRef(bool V) { isJumpTableRef = V; }
  std::string getSymbolRefFixupName() const { return SymbolRefFixupName; }
  void setSymbolRefFixupName(std::string FN) { SymbolRefFixupName = FN; }
  /// @brief Set Fixup information for a given section. Users need not use this.
  void setFixups(FIXUPTYPE &Fixups,
                 ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo,
                 const std::string &secName);

  /// @brief get the section id for a given section name.
  int getFixupSectionId(std::string secName) {
    for (size_t i = 0;
         i < sizeof(fixupLookupSections) / sizeof(*fixupLookupSections); ++i)
      if (secName.compare(fixupLookupSections[i]) == 0)
        return i;
    return -1;
  }

  /// @brief Get the FixupTuple object for a given section.
  ShuffleInfo::ReorderInfo_FixupInfo_FixupTuple *
  getFixupTuple(ShuffleInfo::ReorderInfo_FixupInfo *FI, std::string secName) {
    switch (getFixupSectionId(secName)) {
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
  void serializeReorderInfo(ShuffleInfo::ReorderInfo *ri,
                            const MCAsmLayout &Layout);

  /// @brief Add seen function ID to the list.
  void updateSeenFuncs(unsigned funcID) const {
    if (std::find(SeenFunctionIDs.begin(), SeenFunctionIDs.end(), funcID) ==
        SeenFunctionIDs.end())
      SeenFunctionIDs.push_back(funcID);
  }

  /// @brief Check if we have already seen this function ID.
  bool isSeenFuncs(unsigned funcID) const {
    if (std::find(SeenFunctionIDs.begin(), SeenFunctionIDs.end(), funcID) ==
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

  mutable std::string latestParentID;   ///< The latest parent ID.
  mutable std::string latestFunctionID; ///< The latest function ID.
  mutable unsigned nargs; ///< The number of arguments for the latest function.

  mutable bool isAssemFile = false; ///< Is the current file assembly?
  mutable bool hasInlineAssembly =
      false; ///< Does the current function have inline assembly?
  mutable std::string
      prevOpcode; ///< The previous opcode encounted in collection.
  mutable unsigned assemFuncNo =
      0xffffffff; ///< The function number for the current assembly function.
  mutable unsigned assemBBLNo =
      0; ///< The basic block number for the current assembly basic block.
  mutable unsigned specialCntPriorToFunc = 0; ///< Unused and deprecated?

  /// Dump the collected jump table information.
  void dumpJT(JTTYPEWITHID &jumpTables, const MCAsmInfo *MAI);

  /// Process a fragment and collect fixup information.
  void processFragment(MCSection &Sec, const MCAsmLayout &Layout,
                       const MCAsmInfo *MAI, const MCObjectFileInfo *MOFI,
                       MCSectionELF &ELFSec);

  void updateJumpTableTargets(std::string Key, unsigned EntryKind, unsigned EntrySize, \
          std::list<std::string> JTEntries) const {
      JumpTableTargets[Key] = std::make_tuple(EntryKind, EntrySize, JTEntries);
  }

  std::map<std::string, std::tuple<unsigned, unsigned, std::list<std::string>>> 
      getJumpTableTargets() const { 
          return JumpTableTargets; 
      }
  /// @brief Set the latest parent ID.
  void setFunctionid(std::string id) { latestFunctionID = id; }

  /// @brief Set the Fixup info from each section
  void setFixupInfo(ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo,
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
  void updateByteCounter(MBBIDTYPE &id, unsigned emittedBytes,
                         unsigned numFixups, bool isAlign, bool isInline) {

    MachineBasicBlocks[id].TotalSizeInBytes +=
        emittedBytes;                              // Acutal size in MBB
    MachineBasicBlocks[id].NumArgs = nargs;        // Acutal size in MBB
    MachineBasicBlocks[id].NumFixUps += numFixups; // Number of Fixups in MBB

    if (isAlign)
      MachineBasicBlocks[id].Alignments += emittedBytes; // Count NOPs in MBB

    // If inlined, add the bytes in the next MBB instead of current one
    if (isInline)
      MachineBasicBlocks[id].TotalSizeInBytes -= emittedBytes;
  }

  /// @brief  Sets the successors of a basic block.
  /// @param id The ID of the basic block to be updated.
  /// @param succs Successor IDs of the basic block.
  void setSuccs(std::string id, const std::set<std::string> &succs) {
    MachineBasicBlocks[id].Successors = succs;
  }

  /// @brief Sets the predecessors of a basic block.
  /// @param id  The ID of the basic block to be updated.
  /// @param preds  Predecessor IDs of the basic block.
  void setPreds(std::string id, const std::set<std::string> &preds) {
    MachineBasicBlocks[id].Predecessors = preds;
  }
  BasicBlockCollector() {}
  virtual ~BasicBlockCollector() {}
};

class ClassCollector : public BCollector {
public:
  void addVTable(std::string classname, std::list<std::string> vtable_entries) {
    vTables[classname] = vtable_entries;
  }
};

// A lot of the stuff here comes from DWRAF
// -gdwarf flag is absolutely necessary!
/// @brief  Collects the function information.
class FunctionCollector : public BCollector {
public:
  /// @brief  Collects the function information. Includes things like:
  /// ID, function name, arguments (if any)
  void updateFuncDetails(std::string id, std::string funcname, unsigned size) {
    MachineFunctions[id].TotalSizeInBytes = size;
    MachineFunctions[id].FunctionName = funcname;
    MachineFunctions[id].ID = id;
    NametoMFID[funcname] = id;
  }

  auto &getVT() { return vTables; }

  std::string getDICompositeTypeKind(const DICompositeType *CompositeType) {
    if (CompositeType == nullptr) {
      // might want to log an error message here.
      return "Invalid";
    }

    unsigned tag = CompositeType->getTag();

    switch (tag) {
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

  std::string checkDIType(const DIType *TheDIType) {
    std::string type_name = "";
    if (const DIBasicType *BasicType = dyn_cast<DIBasicType>(TheDIType)) {
      // TheDIType is a DIBasicType, and BasicType is of type DIBasicType*
      type_name = BasicType->getName().str();
    } else if (const DICompositeType *CompositeType =
                   dyn_cast<DICompositeType>(TheDIType)) {
      // TheDIType is a DICompositeType, and CompositeType is of type
      // DICompositeType*
      type_name = getDICompositeTypeKind(CompositeType);
    } else {
      type_name = "Pointer";
    }
    return type_name;
  }
  /// @brief Update the argument details of a function
  /// @param funcname Name of the function to be updated
  /// @param numArgs Number of arguments observed
  void updateArgDetails(std::string funcname, int numArgs) {

    // check if name is in the map
    if (NametoMFID.find(funcname) == NametoMFID.end()) {
      DEBUG_WITH_TYPE("binbench",
                      dbgs() << "Function not found: " << funcname << "\n");
      return;
    }
    DEBUG_WITH_TYPE("binbench",
                    dbgs() << "Function has numArgs: " << numArgs << "\n");
    MachineFunctions[NametoMFID[funcname]].NumArgs = numArgs;
  }

  void updateArgTypes(std::string funcname, std::string argType) {

    // check if name is in the map
    if (NametoMFID.find(funcname) == NametoMFID.end()) {
      DEBUG_WITH_TYPE("binbench",
                      dbgs() << "Function not found: " << funcname << "\n");
      return;
    }

    MachineFunctions[NametoMFID[funcname]].ArgTypes.push_back(argType);
  }

  void addLocalVariable(std::string funcname, std::string varname,
                        std::string type, int Offset, unsigned size) {
    MachineFunctions[NametoMFID[funcname]].LocalVars[varname] =
        std::make_tuple(type, Offset, size);
  }

    void addCGSuccessor(std::string caller, std::string callee) {
      const auto &it = CallGraphInfo.find(caller);
      if (it == CallGraphInfo.end()) {
        std::list<std::string> newList{callee};
        CallGraphInfo[caller] = newList;
        } else {
            // Check if callee already exists
            auto &callerList = CallGraphInfo[caller];
            if (std::find(callerList.begin(), callerList.end(), callee) == callerList.end()) {
                // If callee doesn't exist, push it back to the list.
                callerList.push_back(callee);
            }
        }
    }

  void addVTableEntry(std::string classname, std::string entry) {
    const auto &it = vTables.find(classname);
    if (it == vTables.end()) {
        std::list<std::string> newList {entry};
        vTables[classname] = newList;
    } else {
        auto &entries_list = vTables[classname];
        if (std::find(entries_list.begin(), entries_list.end(), entry) == entries_list.end()) {
            entries_list.push_back(entry);
        }
    }
  }


  /// @brief Get collected function metadata object.
  MFCONTAINER &getMFs() { return MachineFunctions; }

  auto &getCG() {return CallGraphInfo;}

  /// @brief To set the number of arguments for a function internally and for
  /// debugging.
  void setNumArgs(std::string funcname, unsigned numArgs) {
    nargs = numArgs;
    DEBUG_WITH_TYPE("binbench", dbgs() << "NumArgs: " << nargs << "\n");
    DEBUG_WITH_TYPE("binbench", dbgs() << "funcname: " << funcname << "\n");
  }

  /// @brief Update the argument sizes of a function
  /// @param funcname Name of the function to be updated
  /// @param argsize The size of the argument in bits
  void addArgSizes(std::string funcname, unsigned argsize) {
    DEBUG_WITH_TYPE("binbench", dbgs() << "For Func " << funcname
                                       << "ArgSize: " << argsize << "\n");
    MachineFunctions[NametoMFID[funcname]].ArgSizesInBits.push_back(argsize);
  }

  FunctionCollector() {}
  virtual ~FunctionCollector() {}
};

} // namespace llvm

#endif // LLVM_BCOLLECTOR_H
