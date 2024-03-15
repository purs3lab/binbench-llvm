#include "llvm/BCollector/BCollectorAPI.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BCollector/BCollectorTypes.h"
#include "llvm/BCollector/BCollectorUtils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/shuffleInfo.pb.h"
#include <set>

using namespace llvm;

// Sets Predecessor and Successor of basic blocks. 
void BasicBlockCollector::performCollection(const MachineInstr *MI,
                                            MCInst *Inst) {
  std::set<std::string> Preds;
  std::set<std::string> Succs;
  const MachineBasicBlock *MBBa = MI->getParent();
  using IteratorTypeSucc = llvm::MachineBasicBlock::const_succ_iterator;
  for (IteratorTypeSucc Succ = MBBa->succ_begin(); Succ != MBBa->succ_end(); Succ++){
    unsigned SMBBID = (*Succ)->getNumber();
    unsigned SMFID = (*Succ)->getParent()->getFunctionNumber();
    const std::string &SID =
        std::to_string(SMFID) + "_" + std::to_string(SMBBID);
    Succs.insert(SID);
  }
  
  using IteratorTypePred = llvm::MachineBasicBlock::const_pred_iterator;
  for (IteratorTypePred Pred = MBBa->pred_begin(); Pred != MBBa->pred_end(); Pred++) {
    unsigned PMBBID = (*Pred)->getNumber();
    unsigned PMFID = (*Pred)->getParent()->getFunctionNumber();
    const std::string &PID =
        std::to_string(PMFID) + "_" + std::to_string(PMBBID);
    Preds.insert(PID);
  }

  unsigned MBBID = MBBa->getNumber();
  unsigned MFID = MBBa->getParent()->getFunctionNumber();
  unsigned FuncSize = MBBa->getParent()->size();
  const std::string &ID = std::to_string(MFID) + "_" + std::to_string(MBBID);
  std::string FuncName = MBBa->getParent()->getName().str();

  // funcname should be unique
  if (FuncName.length() == 0)
    FuncName = "func_" + std::to_string(MFID);

  Inst->setParentID(ID);
  Inst->setFunctionID(std::to_string(MFID));
  Inst->setFunctionName(FuncName);
  Inst->setFunctionSize(FuncSize);
  Inst->setSuccs(ID, Succs);
  Inst->setPreds(ID, Preds);
}

BCollector::BCollector() {
  utils = new BCollectorUtils();
  JumpTableTargets =
      std::map<std::string,
               std::tuple<unsigned, unsigned, std::list<std::string>>>();
  JumpTableTargets.clear();
}

// Akul FIXME: Move this to BCollector
// Koo: Dump all fixups if necessary
//      In .text, .rodata, .data, .data.rel.ro, .eh_frame, and debugging
//      sections
void BCollector::dumpFixups(FIXUPTYPE &Fixups, const std::string &Kind,
                            bool IsDebug) {
  if (Fixups.size() > 0) {
    DEBUG_WITH_TYPE("binbench", dbgs() << " - Fixups Info (." << Kind
                                       << "): " << Fixups.size() << "\n");
    unsigned Offset, Size, NumJTEntries, JTEntrySize;
    bool IsRel, IsNewSection;
    StringRef FixupParentID, SymbolRefFixupName;

    for (FIXUPTYPE::const_iterator Iter = Fixups.begin(); Iter != Fixups.end();
         ++Iter) {
      std::tie(Offset, Size, IsRel, FixupParentID, SymbolRefFixupName,
               IsNewSection, std::ignore, NumJTEntries, JTEntrySize) = *Iter;
      char IsRelTF = IsRel ? 'T' : 'F';
      if (IsDebug && SymbolRefFixupName.size() > 0) {

        DEBUG_WITH_TYPE("binbench", dbgs() << "\t[" << FixupParentID << "]\t("
                                           << Offset << ", " << Size << ", "
                                           << IsRelTF);
        if (SymbolRefFixupName.size() > 0)
          DEBUG_WITH_TYPE("binbench",
                          dbgs() << ", JT#" << SymbolRefFixupName << ")\n");
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

  // Deal with MFs and MBBs in a ELF code section (.text) only
  for (MCSection &Sec : Layout.getAssembler()) {
    MCSectionELF &ELFSec = reinterpret_cast<MCSectionELF &>(Sec);
    const std::string &SectionName = ELFSec.getSectionName().str();
    if (SectionName.find(".text") == 0) {

      // Per each fragment in a .text section
      processFragment(Sec, Layout, MAI, MOFI, ELFSec);
    }
  }

  // Dump if there is any CFI-generated JT
  // Commented out for debugging?
  // dumpJT(jumpTables, MAI);
}

void BCollector::processFragment(MCSection &Sec, const MCAsmLayout &Layout,
                                 const MCAsmInfo *MAI,
                                 const MCObjectFileInfo *MOFI,
                                 MCSectionELF &ELFSec) {
  const llvm::MCAssembler &Assembler = Layout.getAssembler();
  const llvm::MCContext &Ctx = Assembler.getContext();
  const auto Arch = Ctx.getTargetTriple().getArch();
  // DEBUG_WITH_TYPE("binbench", dbgs() << "Arch: " << arch << "\n");
  unsigned TotalOffset = 0, TotalFixups = 0, TotalAlignSize = 0, PrevMBB = 0;
  int MFID, MBBID, PrevMFID = -1;
  std::string PrevID, CanFallThrough;
  unsigned MBBSize, NumFixups, AlignSize;
  std::set<std::string> CountedMBBs;
  std::string TmpSN = "";
  const std::string &SectionName = ELFSec.getSectionName().str();
  for (MCFragment &MCF : Sec) {
    // processFragment(MCF, Layout, MAI, MOFI, totalOffset, totalFixups)
    // Here MCDataFragment has combined with the following
    // MCRelaxableFragment or MCAlignFragment Corner case: MCDataFragment
    // with no instruction - just skip it
    if (isa<MCDataFragment>(MCF) && MCF.hasInstructions()) {
      if (Arch != 16) {
        TotalOffset = MCF.getOffset(); // XXX: hackish fix for MIPS
      }

      // The offset we get here in incorrect for MIPS so we rely on our
      // own arithmetic

      // Update the MBB offset and MF Size for all collected MBBs in the MF
      for (std::string ID : MCF.getAllMBBs()) {
        if (ID.length() == 0 &&
            MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes > 0) {
          ID = "999_999";
        }

        if (CountedMBBs.find(ID) == CountedMBBs.end() && ID.length() > 0) {
          std::tie(MFID, MBBID) = MAI->getBC()->separateID(ID);

          MBBSize = MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes;
          NumFixups = MAI->getBC()->MachineBasicBlocks[ID].NumFixUps;
          AlignSize = MAI->getBC()->MachineBasicBlocks[ID].Alignments;

          if (TmpSN.length() > 0)
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
            PrevMBB = 0;
            MAI->getBC()->updateSeenFuncs(MFID);
          }
          // Update the MBB offset, MF Size and section name accordingly

          MAI->getBC()->MachineBasicBlocks[ID].Offset = TotalOffset;
          PrevMBB += MBBSize - AlignSize;
          TotalOffset += MBBSize - AlignSize;
          TotalFixups += NumFixups;
          TotalAlignSize += AlignSize;

          CountedMBBs.insert(ID);
          MAI->getBC()->MachineFunctionSizes[MFID] += MBBSize;
          MAI->getBC()->MachineBasicBlocks[ID].SectionName = SectionName;
          CanFallThrough = MAI->getBC()->canMBBFallThrough[ID] ? "*" : "";

          if (MFID > PrevMFID) {
            MAI->getBC()->MachineBasicBlocks[PrevID].BBType =
                MBBINFOTYPE::END; // Type = End of the function
          }

          PrevMFID = MFID;
          PrevID = ID;
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
      if (CountedMBBs.find(ID) == CountedMBBs.end() && ID.length() > 0) {
        std::tie(MFID, MBBID) = MAI->getBC()->separateID(ID);

        MBBSize = MAI->getBC()->MachineBasicBlocks[ID].TotalSizeInBytes;
        NumFixups = MAI->getBC()->MachineBasicBlocks[ID].NumFixUps;
        AlignSize = MAI->getBC()->MachineBasicBlocks[ID].Alignments;
        TmpSN = MAI->getBC()->MachineBasicBlocks[ID].SectionName;

        if (TmpSN.length() > 0)
          continue;

        MAI->getBC()->MBBLayoutOrder.push_back(ID);

        if (!MAI->getBC()->isSeenFuncs(MFID)) {
          PrevMBB = 0;
          MAI->getBC()->updateSeenFuncs(MFID);
        }
        // Update the MBB offset, MF Size and section name accordingly
        MAI->getBC()->MachineBasicBlocks[ID].Offset = TotalOffset;
        PrevMBB += MBBSize - AlignSize;
        TotalOffset += MBBSize - AlignSize;
        TotalFixups += NumFixups;
        TotalAlignSize += AlignSize;
        CountedMBBs.insert(ID);
        MAI->getBC()->MachineFunctionSizes[MFID] += MBBSize;
        MAI->getBC()->MachineBasicBlocks[ID].SectionName = SectionName;

        if (MFID > PrevMFID) {
          // Type = End of the function
          MAI->getBC()->MachineBasicBlocks[ID].BBType = MBBINFOTYPE::END;
        }

        PrevMFID = MFID;
        PrevID = ID;
      }
    }
  }
  // The last ID Type is always the end of the object
  MAI->getBC()->MachineBasicBlocks[PrevID].BBType =
      MBBINFOTYPE::ENDOFOBJECT;
}

void BCollector::dumpJT(const JTTYPEWITHID &JumpTables, const MCAsmInfo *MAI) {
  if (JumpTables.size() > 0) {
    DEBUG_WITH_TYPE("binbench", dbgs() << "\n<Jump Tables Summary>\n");
    unsigned TotalEntries = 0;
    for (JTTYPEWITHID::const_iterator Iter = JumpTables.begin();
         Iter != JumpTables.end(); ++Iter) {
      int JTI, MFID, MFID2, MBBID;
      unsigned EntryKind, EntrySize;
      std::list<std::string> JTEntries;

      std::tie(MFID, JTI) = MAI->getBC()->BCollector::separateID(Iter->first);
      std::tie(EntryKind, EntrySize, JTEntries) = Iter->second;

      DEBUG_WITH_TYPE("binbench", dbgs() << "[JT@Function#" << MFID << "_"
                                         << JTI << "] "
                                         << "(Kind: " << EntryKind << ", "
                                         << JTEntries.size() << " Entries of "
                                         << EntrySize << "B each)\n");

      for (const std::string &JTE : JTEntries) {
        std::tie(MFID2, MBBID) = BCollector::separateID(JTE);
        TotalEntries++;
        if (MFID != MFID2)
          DEBUG_WITH_TYPE(
              "binbench",
              dbgs() << "[CCR-Error] MCAssembler::updateReorderInfoValues - JT "
                        "Entry points to the outside of MF! \n");
        DEBUG_WITH_TYPE("binbench",
                        dbgs()
                            << "\t[" << JTE << "]\t"
                            << BCollectorUtils::hexlify(
                                   MAI->getBC()->MachineBasicBlocks[JTE].Offset)
                            << "\n");
      }
    }

    DEBUG_WITH_TYPE("binbench", dbgs() << "#JTs\t#Entries\n"
                                       << JumpTables.size() << "\t"
                                       << TotalEntries << "\n");
  }
}

// Akul FIXME: Move this to BCollector
void BCollector::setFixups(FIXUPTYPE &Fixups,
                           ShuffleInfo::ReorderInfo_FixupInfo *FixupInfo,
                           const std::string &SecName) {
  unsigned FixupOffset, FixupSize, FixupisRela, NumJTEntries, JTEntrySize;
  StringRef SectionName;
  bool IsNewSection;

  for (FIXUPTYPE::const_iterator F = Fixups.begin(); F != Fixups.end(); ++F) {
    ShuffleInfo::ReorderInfo_FixupInfo_FixupTuple *PFixupTuple =
        getFixupTuple(FixupInfo, SecName);
    std::tie(FixupOffset, FixupSize, FixupisRela, std::ignore, std::ignore,
             IsNewSection, SectionName, NumJTEntries, JTEntrySize) = *F;
    PFixupTuple->set_offset(FixupOffset);
    PFixupTuple->set_deref_sz(FixupSize);
    PFixupTuple->set_is_rela(FixupisRela);
    PFixupTuple->set_section_name(SectionName.data());
    if (IsNewSection)
      PFixupTuple->set_type(
          4); // let linker know if there are multiple .text sections
    else
      PFixupTuple->set_type(
          0); // c2c, c2d, d2c, d2d default=0; should be updated by linker
              //
    PFixupTuple->set_dst_offset(-1); // Should be updated at linking time

    // The following jump table information is fixups in .text for JT entry
    // update only (pic/pie)
    if (NumJTEntries > 0) {
      PFixupTuple->set_num_jt_entries(NumJTEntries);
      PFixupTuple->set_jt_entry_sz(JTEntrySize);
    }
  }
}

void BCollector::serializeReorderInfo(ShuffleInfo::ReorderInfo *RI,
                                      const MCAsmLayout &Layout) {
  // TODO Akul: We can add new info here for the functions
  // Set the binary information for reordering
  ShuffleInfo::ReorderInfo_BinaryInfo *BinaryInfo = RI->mutable_bin();
  BinaryInfo->set_rand_obj_offset(0x0);  // Should be updated at linking time
  BinaryInfo->set_main_addr_offset(0x0); // Should be updated at linking time

  const MCAsmInfo *MAI = Layout.getAssembler().getContext().getAsmInfo();

  // Identify this object file has been compiled from:
  //    obj_type = 0: a general source file (i.e., *.c, *.cc, *.cpp, ...)
  //    obj_type = 1: a source file that contains inline assembly
  //    obj_type = 2: standalone assembly file (i.e., *.s, *.S, ...)
  if (MAI->getBC()->isAssemFile)
    BinaryInfo->set_src_type(2);
  else if (MAI->getBC()->hasInlineAssembly)
    BinaryInfo->set_src_type(1);
  else
    BinaryInfo->set_src_type(0);

  updateReorderInfoValues(Layout);
  // Set the layout of both Machine Functions and Machine Basic Blocks with
  // protobuf definition
  unsigned MBBSize = 0, NumBBs = 0, NumFuncs = 0;
  unsigned ObjSz = 0; 
  int MFID, MBBID, PrevMFID = 0;

  for (std::list<std::string>::const_iterator MBBI =
           MAI->getBC()->MBBLayoutOrder.begin();
       MBBI != MAI->getBC()->MBBLayoutOrder.end(); ++MBBI) {
    ShuffleInfo::ReorderInfo_LayoutInfo *LayoutInfo = RI->add_layout();
    const std::string &ID = MBBI->c_str();
    std::tie(MFID, MBBID) = separateID(ID);

    auto &MBBInfo = MAI->getBC()->MachineBasicBlocks[ID];

    bool MBBFallThrough = MAI->getBC()->canMBBFallThrough[ID];

    // Akul XXX: Add MBB succs, preds, and function calling convention stuff
    // here
    LayoutInfo->set_bb_size(MBBInfo.TotalSizeInBytes);
    LayoutInfo->set_type(MBBInfo.BBType);
    LayoutInfo->set_num_fixups(MBBInfo.NumFixUps);
    LayoutInfo->set_bb_fallthrough(MBBFallThrough);
    LayoutInfo->set_section_name(MBBInfo.SectionName);
    LayoutInfo->set_offset(MBBInfo.Offset);
    LayoutInfo->set_nargs(MBBInfo.NumArgs);
    LayoutInfo->set_bb_id(ID);
    for (auto &Pred : MBBInfo.Predecessors) {
      LayoutInfo->add_preds(Pred);
    }
    for (auto &Succ : MBBInfo.Successors) {
      LayoutInfo->add_succs(Succ);
    }
    LayoutInfo->set_padding_size(MBBInfo.Alignments);

    if (MFID > PrevMFID) {
      NumFuncs++;
      NumBBs = 0;
    }

    ObjSz += MBBSize;
    NumBBs++;
    PrevMFID = MFID;
  }

  BinaryInfo->set_obj_sz(ObjSz);

  // Set the fixup information (.text, .rodata, .data, .data.rel.ro and
  // .init_array)
  const MFCONTAINER &MFs = MAI->getFC()->getMFs();
  for (auto const &F : MFs) {
    ShuffleInfo::ReorderInfo_FunctionInfo *FunctionInfo = RI->add_func();
    FunctionInfo->set_f_id(F.first);
    FunctionInfo->set_bb_num(F.second.TotalSizeInBytes);
    FunctionInfo->set_f_name(F.second.FunctionName);
    FunctionInfo->set_nargs(F.second.NumArgs);
    const auto &ArgSizes = F.second.ArgSizesInBits;
    for (auto const &ArgSize : ArgSizes) {
      FunctionInfo->add_argsizes(ArgSize);
    }
    const auto &ArgTypes = F.second.ArgTypes;
    for (auto const &ArgType : ArgTypes) {
      DEBUG_WITH_TYPE("binbench", dbgs() << "Func Arg Type "<< ArgType << "\n");
      FunctionInfo->add_arg_types(ArgType);
    }
    const auto &LocalVars = F.second.LocalVars;
    for (auto const &LocalVar : LocalVars) {
      FunctionInfo->add_local_var_names(LocalVar.first);
      FunctionInfo->add_local_var_types(std::get<0>(LocalVar.second));
      FunctionInfo->add_local_var_offsets(std::get<1>(LocalVar.second));
      FunctionInfo->add_local_var_sizes(std::get<2>(LocalVar.second));
    }
  }

  const auto &CallGraphInfo = MAI->getFC()->getCG();

  for (auto const &Cg : CallGraphInfo) {
    ShuffleInfo::ReorderInfo_CallGraphInfo *CGInfo = RI->add_func_cg();
    CGInfo->set_f_name(Cg.first);
    DEBUG_WITH_TYPE("binbench", dbgs()
                                    << "Func found in CG " << Cg.first << "\n");
    for (auto const &Successor : Cg.second) {
      CGInfo->add_succs(Successor);
      DEBUG_WITH_TYPE("binbench", dbgs() << "Succs " << Successor << "\n");
    }
  }

  const auto &VTables = MAI->getFC()->getVT();

  for (auto const &V: VTables) {
    ShuffleInfo::ReorderInfo_ClassInfo *ClassInfo = RI->add_class_proto();

    ClassInfo->set_vtable_name(V.first);
    for (auto const &Entry : V.second) {
      ClassInfo->add_ventry(Entry);
    }

  }

  ShuffleInfo::ReorderInfo_FixupInfo *FixupInfo = RI->add_fixup();
  setFixupInfo(FixupInfo, MAI);
}

void BCollector::setFixupInfo(ShuffleInfo::ReorderInfo_FixupInfo *FixupInfo, const MCAsmInfo *MAI) {
  setFixups(MAI->getBC()->FixupsText, FixupInfo, ".text");
  setFixups(MAI->getBC()->FixupsRodata, FixupInfo, ".rodata");
  setFixups(MAI->getBC()->FixupsData, FixupInfo, ".data");
  setFixups(MAI->getBC()->FixupsDataRel, FixupInfo, ".data.rel.ro");
  setFixups(MAI->getBC()->FixupsInitArray, FixupInfo, ".init_array");
}
