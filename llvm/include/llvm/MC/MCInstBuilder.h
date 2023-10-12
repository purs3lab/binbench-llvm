//===-- llvm/MC/MCInstBuilder.h - Simplify creation of MCInsts --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the MCInstBuilder class for convenient creation of
// MCInsts.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCINSTBUILDER_H
#define LLVM_MC_MCINSTBUILDER_H

#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

#include <string>

namespace llvm {

class MCInstBuilder {
  MCInst Inst;

public:
  /// Create a new MCInstBuilder for an MCInst with a specific opcode.
  MCInstBuilder(unsigned Opcode) {
    Inst.setOpcode(Opcode);
  }

  /// Add a new register operand.
  MCInstBuilder &addReg(unsigned Reg) {
    Inst.addOperand(MCOperand::createReg(Reg));
    return *this;
  }

  /// Add a new integer immediate operand.
  MCInstBuilder &addImm(int64_t Val) {
    Inst.addOperand(MCOperand::createImm(Val));
    return *this;
  }

  /// Add a new single floating point immediate operand.
  MCInstBuilder &addSFPImm(uint32_t Val) {
    Inst.addOperand(MCOperand::createSFPImm(Val));
    return *this;
  }

  /// Add a new floating point immediate operand.
  MCInstBuilder &addDFPImm(uint64_t Val) {
    Inst.addOperand(MCOperand::createDFPImm(Val));
    return *this;
  }

  /// Add a new MCExpr operand.
  MCInstBuilder &addExpr(const MCExpr *Val) {
    Inst.addOperand(MCOperand::createExpr(Val));
    return *this;
  }

  /// Add a new MCInst operand.
  MCInstBuilder &addInst(const MCInst *Val) {
    Inst.addOperand(MCOperand::createInst(Val));
    return *this;
  }

  MCInstBuilder &setMetaData(const MachineInstr *MI) {

    std::set<std::string> Preds;
    std::set<std::string> Succs;
    // LLVM_DEBUG(dbgs() << "Successors: ");
    auto succs = MI->getParent()->successors();
    for (auto succ = succs.begin(); succ != succs.end(); succ++) {
      // LLVM_DEBUG(dbgs() << (*succ)->getNumber() << "\n");
      unsigned SMBBID = (*succ)->getNumber();
      unsigned SMFID = (*succ)->getParent()->getFunctionNumber();
      std::string SID = std::to_string(SMFID) + "_" + std::to_string(SMBBID);
      Succs.insert(SID);
    }
    // LLVM_DEBUG(dbgs() << "\n");
    // LLVM_DEBUG(dbgs() << "Predecessors: ");
    auto preds = MI->getParent()->predecessors();
    for (auto pred = preds.begin(); pred != preds.end(); pred++) {
      // LLVM_DEBUG(dbgs() << (*pred)->getNumber() << "\n");
      unsigned PMBBID = (*pred)->getNumber();
      unsigned PMFID = (*pred)->getParent()->getFunctionNumber();
      std::string PID = std::to_string(PMFID) + "_" + std::to_string(PMBBID);
      Preds.insert(PID);
    }
    // LLVM_DEBUG(dbgs() << "\n");
    const MachineBasicBlock *MBBa = MI->getParent();
    unsigned MBBID = MBBa->getNumber();
    unsigned MFID = MBBa->getParent()->getFunctionNumber();
    unsigned funcsize = MBBa->getParent()->size();
    std::string FunctionName = MBBa->getParent()->getName().str();
    std::string ID = std::to_string(MFID) + "_" + std::to_string(MBBID);

    this->Inst.setParentID(ID);
    this->Inst.setFunctionID(std::to_string(MFID));
    this->Inst.setFunctionName(FunctionName);
    this->Inst.setFunctionSize(funcsize);
    this->Inst.setSuccs(ID, Succs);
    this->Inst.setPreds(ID, Preds);

    return *this;
  }

  /// Add an operand.
  MCInstBuilder &addOperand(const MCOperand &Op) {
    Inst.addOperand(Op);
    return *this;
  }

  operator MCInst&() {
    return Inst;
  }
};

} // end namespace llvm

#endif
