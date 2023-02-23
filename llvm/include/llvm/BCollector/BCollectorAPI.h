// Base class for backend metadata collectors
#ifndef LLVM_BCOLLECTOR_H
#define LLVM_BCOLLECTOR_H

#include "llvm/BCollector/BCollectorUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
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

namespace llvm {

class BCollector {
public:
  BCollectorUtils *utils;

  // Information about all machine basic blocks.
  MBBCONTAINER MachineBasicBlocks;

  // mutable std::map<std::string, std::tuple<unsigned, std::string>>
  // MachineFunctions;

  MFCONTAINER MachineFunctions;
  mutable std::map<std::string, bool> canMBBFallThrough;
  mutable std::map<unsigned, unsigned> MachineFunctionSizes;
  mutable std::list<std::string> MBBLayoutOrder;

  mutable std::vector<unsigned> SeenFunctionIDs;

  // virtual void performCollection() = 0;

  const char *fixupLookupSections[5] = {
      ".text", ".rodata", ".data", ".data.rel.ro", ".init_array",
  };

  void dumpFixups(
      std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string,
                           bool, std::string, unsigned, unsigned>>
          Fixups,
      std::string kind, bool isDebug);

  void updateReorderInfoValues(const MCAsmLayout &Layout);
  std::tuple<int, int> separateID(std::string ID) {
    return std::make_tuple(std::stoi(ID.substr(0, ID.find("_"))),
                           std::stoi(ID.substr(ID.find("_") + 1, ID.length())));
  }

  void setFixups(
      std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string,
                           bool, std::string, unsigned, unsigned>>
          Fixups,
      ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo, std::string secName);
  int getFixupSectionId(std::string secName) {
    for (size_t i = 0;
         i < sizeof(fixupLookupSections) / sizeof(*fixupLookupSections); ++i)
      if (secName.compare(fixupLookupSections[i]) == 0)
        return i;
    return -1;
  }

  // Akul FIXME: Move this to BCollector
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

  // TODO: Overload these in derived classes?
  void getMetadata() { llvm_unreachable("getMetadata() is not implemented"); }

  void setMetadata() { llvm_unreachable("setMetadata() is not implemented"); }

  void printMetadata() {
    llvm_unreachable("printMetadata() is not implemented");
  }

  void serializeReorderInfo(ShuffleInfo::ReorderInfo *ri,
                            const MCAsmLayout &Layout);

  void updateSeenFuncs(unsigned funcID) const {
    if (std::find(SeenFunctionIDs.begin(), SeenFunctionIDs.end(), funcID) ==
        SeenFunctionIDs.end())
      SeenFunctionIDs.push_back(funcID);
  }

  bool isSeenFuncs(unsigned funcID) const {
    if (std::find(SeenFunctionIDs.begin(), SeenFunctionIDs.end(), funcID) ==
        SeenFunctionIDs.end())
      return false;
    return true;
  }

  // (b) Fixups (list)
  //    * <offset, size, isRela, parentID, SymbolRefFixupName, isNewSection,
  //    secName, numJTEntries, JTEntrySz>
  //    - The last two elements are jump table information for FixupsText only,
  //      which allows for updating the jump table entries (relative values)
  //      with pic/pie-enabled.
  mutable std::list<
      std::tuple<unsigned, unsigned, bool, std::string, std::string, bool,
                 std::string, unsigned, unsigned>>
      FixupsText, FixupsRodata, FixupsData, FixupsDataRel, FixupsInitArray;
  //    - FixupsEhframe, FixupsExceptTable; (Not needed any more as a randomizer
  //    directly handles them later on)
  //    - Keep track of the latest ID when parent ID is unavailable
  mutable std::string latestParentID;
  mutable std::string latestFunctionID;
  mutable unsigned nargs;

  // (c) Others
  //     The following method helps full-assembly file (*.s) identify functions
  //     and basic blocks that inherently lacks their boundaries because neither
  //     MF nor MBB has been constructed.
  mutable bool isAssemFile = false;
  mutable bool hasInlineAssembly = false;
  mutable std::string prevOpcode;
  mutable unsigned assemFuncNo = 0xffffffff;
  mutable unsigned assemBBLNo = 0;
  mutable unsigned specialCntPriorToFunc = 0;

  void updateFuncDetails(std::string id, std::string funcname, unsigned size) {
    MachineFunctions[id].TotalSizeInBytes = size;
    MachineFunctions[id].FunctionName = funcname;
  }

  MFCONTAINER &getMFs() { return MachineFunctions; }

  void dumpJT(JTTYPEWITHID &jumpTables, const MCAsmInfo *MAI);

  void processFragment(MCSection &Sec, const MCAsmLayout &Layout,
                       const MCAsmInfo *MAI, const MCObjectFileInfo *MOFI,
                       MCSectionELF &ELFSec);
  void setFunctionid(std::string id) { latestFunctionID = id; }
  void setFixupInfo(ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo,
                    const MCAsmInfo *MAI);
  explicit BCollector();
  virtual ~BCollector() {}
};

class BasicBlockCollector : public BCollector {
public:
  virtual void performCollection(const MachineInstr *MI, MCInst *Inst);

  // BBlockCollector
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
  void setSuccs(std::string id, const std::set<std::string> &succs) {
    MachineBasicBlocks[id].Successors = succs;
  }

  void setPreds(std::string id, const std::set<std::string> &preds) {
    MachineBasicBlocks[id].Predecessors = preds;
  }
  BasicBlockCollector() {}
  virtual ~BasicBlockCollector() {}
};

} // namespace llvm

#endif // LLVM_BCOLLECTOR_H