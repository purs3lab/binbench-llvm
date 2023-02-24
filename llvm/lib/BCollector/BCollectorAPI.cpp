

#include "llvm/BCollector/BCollectorAPI.h"

#include "llvm/BCollector/BCollectorTypes.h"
#include "llvm/BCollector/BCollectorUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/shuffleInfo.pb.h"
#include <set>

using namespace llvm;

void BasicBlockCollector::performCollection(const MachineInstr *MI,
                                            MCInst *Inst) {
  std::set<std::string> Preds;
  std::set<std::string> Succs;
  auto succs = MI->getParent()->successors();
  for (auto succ = succs.begin(); succ != succs.end(); succ++) {
    unsigned SMBBID = (*succ)->getNumber();
    unsigned SMFID = (*succ)->getParent()->getFunctionNumber();
    std::string SID = std::to_string(SMFID) + "_" + std::to_string(SMBBID);
    Succs.insert(SID);
  }
  auto preds = MI->getParent()->predecessors();
  for (auto pred = preds.begin(); pred != preds.end(); pred++) {
    unsigned PMBBID = (*pred)->getNumber();
    unsigned PMFID = (*pred)->getParent()->getFunctionNumber();
    std::string PID = std::to_string(PMFID) + "_" + std::to_string(PMBBID);
    Preds.insert(PID);
  }
  const MachineBasicBlock *MBBa = MI->getParent();
  unsigned MBBID = MBBa->getNumber();
  unsigned MFID = MBBa->getParent()->getFunctionNumber();
  unsigned funcsize = MBBa->getParent()->size();
  std::string ID = std::to_string(MFID) + "_" + std::to_string(MBBID);

  Inst->setParentID(ID);
  Inst->setFunctionID(std::to_string(MFID));
  Inst->setFunctionName(ID);
  Inst->setFunctionSize(funcsize);
  Inst->setSuccs(ID, Succs);
  Inst->setPreds(ID, Preds);
}

BCollector::BCollector() { utils = new BCollectorUtils(); }

// Akul FIXME: Move this to BCollector
// Koo: Dump all fixups if necessary
//      In .text, .rodata, .data, .data.rel.ro, .eh_frame, and debugging
//      sections
void BCollector::dumpFixups(
    std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string,
                         bool, std::string, unsigned, unsigned>>
        Fixups,
    std::string kind, bool isDebug) {
  if (Fixups.size() > 0) {
    DEBUG_WITH_TYPE("binbench", dbgs() << " - Fixups Info (." << kind
                                       << "): " << Fixups.size() << "\n");
    unsigned offset, size, numJTEntries, JTEntrySize;
    bool isRel, isNewSection;
    std::string FixupParentID, SymbolRefFixupName, sectionName;

    for (auto it = Fixups.begin(); it != Fixups.end(); ++it) {
      std::tie(offset, size, isRel, FixupParentID, SymbolRefFixupName,
               isNewSection, sectionName, numJTEntries, JTEntrySize) = *it;
      char isRelTF = isRel ? 'T' : 'F';
      if (isDebug && SymbolRefFixupName.length() > 0) {
        errs() << "\t[" << FixupParentID << "]\t(" << offset << ", " << size
               << ", " << isRelTF;
        if (SymbolRefFixupName.length() > 0)
          errs() << ", JT#" << SymbolRefFixupName;
        errs() << ")\n";
      }
    }
  }
}
// Akul FIXME: Move this to BCollector
// Koo: Final value updates for the entire layout of both MFs and MBBs
void BCollector::updateReorderInfoValues(const MCAsmLayout &Layout) {
  const MCAsmInfo *MAI = Layout.getAssembler().getContext().getAsmInfo();
  const MCObjectFileInfo *MOFI =
      Layout.getAssembler().getContext().getObjectFileInfo();
  JTTYPEWITHID jumpTables = MOFI->getJumpTableTargets();

  // Deal with MFs and MBBs in a ELF code section (.text) only
  for (MCSection &Sec : Layout.getAssembler()) {
    MCSectionELF &ELFSec = reinterpret_cast<MCSectionELF &>(Sec);
    // MCSection &ELFSec = Sec;

    std::string tmpSN, sectionName = ELFSec.getSectionName().str();
    if (sectionName.find(".text") == 0) {

      // Per each fragment in a .text section
      processFragment(Sec, Layout, MAI, MOFI, ELFSec);      

    }
  }

  // Dump if there is any CFI-generated JT
  dumpJT(jumpTables, MAI);
}

void BCollector::processFragment(MCSection &Sec, const MCAsmLayout &Layout,
                     const MCAsmInfo *MAI, const MCObjectFileInfo *MOFI, MCSectionELF &ELFSec) {
  unsigned totalOffset = 0, totalFixups = 0, totalAlignSize = 0, prevMBB = 0;
  int MFID, MBBID, prevMFID = -1;
  std::string prevID, canFallThrough;
  unsigned MBBSize, numFixups, alignSize;
  std::set<std::string> countedMBBs;
  std::set<std::string> preds;
  std::set<std::string> succs;
  std::string tmpSN, sectionName = ELFSec.getSectionName().str();
  for (MCFragment &MCF : Sec) {
    // processFragment(MCF, Layout, MAI, MOFI, totalOffset, totalFixups)
    // Here MCDataFragment has combined with the following
    // MCRelaxableFragment or MCAlignFragment Corner case: MCDataFragment
    // with no instruction - just skip it
    if (isa<MCDataFragment>(MCF) && MCF.hasInstructions()) {
      totalOffset = MCF.getOffset();

      // Update the MBB offset and MF Size for all collected MBBs in the MF
      for (std::string ID : MCF.getAllMBBs()) {
        if (ID.length() == 0 &&
            MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes > 0) {
          ID = "999_999";
        }

        if (countedMBBs.find(ID) == countedMBBs.end() && ID.length() > 0) {
          std::tie(MFID, MBBID) = MAI->getBC()->separateID(ID);

          MBBSize = MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes;
          numFixups = MAI->getBC()->MachineBasicBlocks[ID].NumFixUps;
          alignSize = MAI->getBC()->MachineBasicBlocks[ID].Alignments;
          // MBBType = MAI->getBC()->MachineBasicBlocks[ID].BBType;

          if (tmpSN.length() > 0)
            continue;
          MAI->getBC()->MBBLayoutOrder.push_back(ID);

          // Handle a corner case: see handleDirectEmitDirectives() in
          // AsmParser.cpp
          if (MAI->getBC()->specialCntPriorToFunc > 0) {
            MAI->getBC()->updateByteCounter(
                ID, MAI->getBC()->specialCntPriorToFunc, /*numFixups=*/0,
                /*isAlign=*/false, /*isInline=*/false);
            MBBSize += MAI->getBC()->specialCntPriorToFunc;
            MAI->getBC()->specialCntPriorToFunc = 0;
          }
          if (!MAI->isSeenFuncs(MFID)) {
            prevMBB = 0;
            MAI->getBC()->updateSeenFuncs(MFID);
          }
          // Update the MBB offset, MF Size and section name accordingly

          MAI->getBC()->MachineBasicBlocks[ID].Offset = totalOffset;
          prevMBB += MBBSize - alignSize;
          totalOffset += MBBSize - alignSize;
          totalFixups += numFixups;
          totalAlignSize += alignSize;
          countedMBBs.insert(ID);
          MAI->getBC()->MachineFunctionSizes[MFID] += MBBSize;
          MAI->getBC()->MachineBasicBlocks[ID].SectionName = sectionName;
          canFallThrough = MAI->getBC()->canMBBFallThrough[ID] ? "*" : "";

          if (MFID > prevMFID) {
            MAI->getBC()->MachineBasicBlocks[prevID].BBType =
                MBBINFOTYPE::END; // Type = End of the function
          }

          prevMFID = MFID;
          prevID = ID;
        }
      }
    }

    // Check out MCRelaxableFragments, which have not combined with any
    // MCDataFragment It happens when there are consecutive
    // MCRelaxableFragment (i.e., switch/case)
    if (isa<MCRelaxableFragment>(MCF) && MCF.hasInstructions()) {
      MCRelaxableFragment &MCRF = static_cast<MCRelaxableFragment &>(MCF);
      std::string ID = MCRF.getInst().getParentID();

      if (ID.length() == 0 &&
          MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes > 0)
        ID = "999_999";
      // If yet the ID has not been showed up along with getAllMBBs(),
      // it would be an independent RF that does not belong to any DF
      if (countedMBBs.find(ID) == countedMBBs.end() && ID.length() > 0) {
        std::tie(MFID, MBBID) = MAI->getBC()->separateID(ID);

        MBBSize = MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes;
        numFixups = MAI->getBC()->MachineBasicBlocks[ID].NumFixUps;
        alignSize = MAI->getBC()->MachineBasicBlocks[ID].Alignments;
        tmpSN = MAI->getBC()->MachineBasicBlocks[ID].SectionName;

        if (tmpSN.length() > 0)
          continue;

        MAI->getBC()->MBBLayoutOrder.push_back(ID);

        if (!MAI->getBC()->isSeenFuncs(MFID)) {
          prevMBB = 0;
          MAI->getBC()->updateSeenFuncs(MFID);
        }
        // Update the MBB offset, MF Size and section name accordingly
        // std::get<1>(MAI->MachineBasicBlocks[ID]) += (fragOff + prevMBB);
        MAI->getBC()->MachineBasicBlocks[ID].Offset = totalOffset;
        prevMBB += MBBSize - alignSize;
        totalOffset += MBBSize - alignSize;
        totalFixups += numFixups;
        totalAlignSize += alignSize;
        countedMBBs.insert(ID);
        MAI->getBC()->MachineFunctionSizes[MFID] += MBBSize;
        MAI->getBC()->MachineBasicBlocks[ID].SectionName = sectionName;

        if (MFID > prevMFID) {
          // Type = End of the function
          MAI->getBC()->MachineBasicBlocks[ID].BBType = MBBINFOTYPE::END;
        }

        prevMFID = MFID;
        prevID = ID;
      }
    }
  }
  // The last ID Type is always the end of the object
  MAI->getBC()->MachineBasicBlocks[prevID].BBType =
      MBBINFOTYPE::ENDOFOBJECT;
}

void BCollector::dumpJT(JTTYPEWITHID &jumpTables, const MCAsmInfo *MAI) {
  if (jumpTables.size() > 0) {
    DEBUG_WITH_TYPE("binbench", dbgs() << "\n<Jump Tables Summary>\n");
    unsigned totalEntries = 0;
    for (auto it = jumpTables.begin(); it != jumpTables.end(); ++it) {
      int JTI, MFID, MFID2, MBBID;
      unsigned entryKind, entrySize;
      std::list<std::string> JTEntries;

      std::tie(MFID, JTI) = MAI->getBC()->BCollector::separateID(it->first);
      std::tie(entryKind, entrySize, JTEntries) = it->second;

      DEBUG_WITH_TYPE("binbench", dbgs() << "[JT@Function#" << MFID << "_"
                                         << JTI << "] "
                                         << "(Kind: " << entryKind << ", "
                                         << JTEntries.size() << " Entries of "
                                         << entrySize << "B each)\n");

      for (std::string JTE : JTEntries) {
        std::tie(MFID2, MBBID) = BCollector::separateID(JTE);
        totalEntries++;
        if (MFID != MFID2)
          errs() << "[CCR-Error] MCAssembler::updateReorderInfoValues - JT "
                    "Entry points to the outside of MF! \n";
        DEBUG_WITH_TYPE("binbench",
                        dbgs()
                            << "\t[" << JTE << "]\t"
                            << BCollectorUtils::hexlify(
                                   MAI->getBC()->MachineBasicBlocks[JTE].Offset)
                            << "\n");
      }
    }

    DEBUG_WITH_TYPE("binbench", dbgs() << "#JTs\t#Entries\n"
                                       << jumpTables.size() << "\t"
                                       << totalEntries << "\n");
  }
}


// Akul FIXME: Move this to BCollector
void BCollector::setFixups(
    std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string,
                         bool, std::string, unsigned, unsigned>>
        Fixups,
    ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo, std::string secName) {
  unsigned FixupOffset, FixupSize, FixupisRela, numJTEntries, JTEntrySize;
  std::string sectionName, FixupParentID, SymbolRefFixupName;
  bool isNewSection;

  for (auto F = Fixups.begin(); F != Fixups.end(); ++F) {
    ShuffleInfo::ReorderInfo_FixupInfo_FixupTuple *pFixupTuple =
        getFixupTuple(fixupInfo, secName);
    std::tie(FixupOffset, FixupSize, FixupisRela, FixupParentID,
             SymbolRefFixupName, isNewSection, sectionName, numJTEntries,
             JTEntrySize) = *F;
    pFixupTuple->set_offset(FixupOffset);
    pFixupTuple->set_deref_sz(FixupSize);
    pFixupTuple->set_is_rela(FixupisRela);
    pFixupTuple->set_section_name(sectionName);
    if (isNewSection)
      pFixupTuple->set_type(
          4); // let linker know if there are multiple .text sections
    else
      pFixupTuple->set_type(
          0); // c2c, c2d, d2c, d2d default=0; should be updated by linker

    // The following jump table information is fixups in .text for JT entry
    // update only (pic/pie)
    if (numJTEntries > 0) {
      pFixupTuple->set_num_jt_entries(numJTEntries);
      pFixupTuple->set_jt_entry_sz(JTEntrySize);
    }
  }
}

void BCollector::serializeReorderInfo(ShuffleInfo::ReorderInfo *ri,
                                      const MCAsmLayout &Layout) {
  // TODO Akul: We can add new info here for the functions
  // Set the binary information for reordering
  ShuffleInfo::ReorderInfo_BinaryInfo *binaryInfo = ri->mutable_bin();
  binaryInfo->set_rand_obj_offset(0x0);  // Should be updated at linking time
  binaryInfo->set_main_addr_offset(0x0); // Should be updated at linking time

  const MCAsmInfo *MAI = Layout.getAssembler().getContext().getAsmInfo();

  // Identify this object file has been compiled from:
  //    obj_type = 0: a general source file (i.e., *.c, *.cc, *.cpp, ...)
  //    obj_type = 1: a source file that contains inline assembly
  //    obj_type = 2: standalone assembly file (i.e., *.s, *.S, ...)
  if (MAI->getBC()->isAssemFile)
    binaryInfo->set_src_type(2);
  else if (MAI->getBC()->hasInlineAssembly)
    binaryInfo->set_src_type(1);
  else
    binaryInfo->set_src_type(0);

  updateReorderInfoValues(Layout);
  // Set the layout of both Machine Functions and Machine Basic Blocks with
  // protobuf definition
  std::string sectionName;
  unsigned MBBSize;
  unsigned objSz = 0, numFuncs = 0, numBBs = 0;
  int MFID, MBBID, prevMFID = 0;
  std::vector<std::string> preds;
  std::vector<std::string> succs;

  for (auto MBBI = MAI->getBC()->MBBLayoutOrder.begin();
       MBBI != MAI->getBC()->MBBLayoutOrder.end(); ++MBBI) {
    ShuffleInfo::ReorderInfo_LayoutInfo *layoutInfo = ri->add_layout();
    std::string ID = *MBBI;
    std::tie(MFID, MBBID) = separateID(ID);

    auto &MBBInfo = MAI->getBC()->MachineBasicBlocks[ID];

    bool MBBFallThrough = MAI->getBC()->canMBBFallThrough[ID];

    // Akul XXX: Add MBB succs, preds, and function calling convention stuff
    // here
    layoutInfo->set_bb_size(MBBInfo.TotalSizeInBytes);
    layoutInfo->set_type(MBBInfo.BBType);
    layoutInfo->set_num_fixups(MBBInfo.NumFixUps);
    layoutInfo->set_bb_fallthrough(MBBFallThrough);
    layoutInfo->set_section_name(MBBInfo.SectionName);
    layoutInfo->set_offset(MBBInfo.Offset);
    layoutInfo->set_nargs(MBBInfo.NumArgs);
    layoutInfo->set_bb_id(ID);
    for (auto &pred : MBBInfo.Predecessors) {
      layoutInfo->add_preds(pred);
    }
    for (auto succ : MBBInfo.Successors) {
      layoutInfo->add_succs(succ);
    }
    layoutInfo->set_padding_size(MBBInfo.Alignments);

    if (MFID > prevMFID) {
      numFuncs++;
      numBBs = 0;
    }

    objSz += MBBSize;
    numBBs++;
    prevMFID = MFID;
  }

  binaryInfo->set_obj_sz(objSz);

  // Set the fixup information (.text, .rodata, .data, .data.rel.ro and
  // .init_array)
  MFCONTAINER &MFs = MAI->getFC()->getMFs();
  for (auto const &x : MFs) {
    ShuffleInfo::ReorderInfo_FunctionInfo *FunctionInfo = ri->add_func();
    FunctionInfo->set_f_id(x.first);
    FunctionInfo->set_bb_num(x.second.TotalSizeInBytes);
    FunctionInfo->set_f_name(x.second.FunctionName);

  }

  ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo = ri->add_fixup();
  setFixupInfo(fixupInfo, MAI);
}

void BCollector::setFixupInfo(ShuffleInfo::ReorderInfo_FixupInfo *fixupInfo, const MCAsmInfo *MAI) {
  setFixups(MAI->getBC()->FixupsText, fixupInfo, ".text");
  setFixups(MAI->getBC()->FixupsRodata, fixupInfo, ".rodata");
  setFixups(MAI->getBC()->FixupsData, fixupInfo, ".data");
  setFixups(MAI->getBC()->FixupsDataRel, fixupInfo, ".data.rel.ro");
  setFixups(MAI->getBC()->FixupsInitArray, fixupInfo, ".init_array");
}
