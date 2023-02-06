// Base class for backend metadata collectors

#include "llvm/MC/MCInst.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/Support/shuffleInfo.pb.h"

#include <iomanip>

using namespace llvm;

static const char* fixupLookupSections[] = 
{
    ".text",
    ".rodata",
    ".data",
    ".data.rel.ro",
    ".init_array",
};

class BCollector {
public:
  mutable std::map<
      std::string,
      std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
                  std::string, std::vector<std::string>,
                  std::vector<std::string>>>
      MachineBasicBlocks;

  mutable std::map<std::string, std::tuple<unsigned, std::string>> MachineFunctions;
  mutable std::map<std::string, bool> canMBBFallThrough;
  mutable std::map<unsigned, unsigned> MachineFunctionSizes;
  mutable std::list<std::string> MBBLayoutOrder;

  mutable std::vector<unsigned> SeenFunctionIDs;

  static void dumpFixups(std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string, bool, std::string, unsigned, unsigned>> \
                  Fixups, std::string kind, bool isDebug);

  static void updateReorderInfoValues(const MCAsmLayout &Layout);
  static std::tuple<int, int> separateID(std::string ID) {
    return std::make_tuple(std::stoi(ID.substr(0, ID.find("_"))), \
                          std::stoi(ID.substr(ID.find("_") + 1, ID.length())));
  }

  bool updateMetadata(MCAsmInfo *MAI) const {
    bool success = false;
    // collect all the bookkeeping information and send it to the BCollector
    // where should this function be called? 
    return success;
  }

  template<typename T> static
  std::string hexlify(T i) {
      std::stringbuf buf;
      std::ostream os(&buf);
      os << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
      return buf.str();
  }

  static void setFixups(std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string, bool, std::string, unsigned, unsigned>> Fixups,
                ShuffleInfo::ReorderInfo_FixupInfo* fixupInfo, std::string secName); 
  // Akul FIXME: Move this to BCollector
  // Koo: These sections contain the fixups that we want to handle
  // Akul FIXME: Move this to BCollector
  // Koo: Helper functions for serializeReorderInfo()
  static int getFixupSectionId(std::string secName) {
      for (size_t i = 0; i < sizeof(fixupLookupSections)/sizeof(*fixupLookupSections); ++i)
          if (secName.compare(fixupLookupSections[i]) == 0)
              return i;
      return -1;
  }

// Akul FIXME: Move this to BCollector
  static ShuffleInfo::ReorderInfo_FixupInfo_FixupTuple* getFixupTuple(ShuffleInfo::ReorderInfo_FixupInfo* FI, std::string secName) {
    switch (getFixupSectionId(secName)) {
      case 0: return FI->add_text();
      case 1: return FI->add_rodata();
      case 2: return FI->add_data();
      case 3: return FI->add_datarel();
      case 4: return FI->add_initarray();
      default: llvm_unreachable("[CCR-Error] ShuffleInfo::getFixupTuple - No such section to collect fixups!");
    }
  }

  void getMetadata() {
    llvm_unreachable("getMetadata() is not implemented");
  }

  void setMetadata() { llvm_unreachable("setMetadata() is not implemented"); }

  void printMetadata() {
    llvm_unreachable("printMetadata() is not implemented");
  }

  static void serializeReorderInfo(ShuffleInfo::ReorderInfo* ri, const MCAsmLayout &Layout);
  BCollector() {}
  ~BCollector() {}
};

class BBlockCollector : public BCollector {
private:
   

public:
  static void setMetadata(const MachineInstr *MI, MCInst *Inst);
  void getMetadata(); 
  BBlockCollector() {}
  ~BBlockCollector() {}
};