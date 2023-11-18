#include "llvm/IR/BingeIRMetadata.h"
#include "llvm/IR/Function.h" // Add this at the top of the file
#include <iostream>
#include <sstream> // Add this at the top of the file

using namespace llvm;
std::map<std::string, std::map<Value*, std::set<Value*>>> BingeIRMetadata::BingeIRInfo;
std::map<std::string, std::map<Value*, std::string>> BingeIRMetadata::BingeIRSrcInfo;

MDNode* BingeIRMetadata::GenBingeMd(Function *F, std::string fileName) {
    std::string funcName = F->getName().str();

    // Look up the function in the BingeIRSrcInfo map.
    auto iter = BingeIRSrcInfo.find("ConditionCollector@" + fileName + "@" + funcName);
    if (iter == BingeIRSrcInfo.end()) {
        // If the function isn't found in the map, return null.
        return nullptr;
    }

    // Fetch the map of Values to string.
    std::map<Value*, std::string> &valueMap = iter->second;

    for (auto &BB : *F) {  // For each basic block in the function
        for (auto &I : BB) {  // For each instruction in the basic block
            if (valueMap.find(&I) != valueMap.end()) {
                // Get the string representing the location.
                std::string InstructionType = valueMap[&I];

                // Create a meaningful debug string for this instruction
                std::stringstream debugInfo;
                debugInfo << "Function: " << funcName
                          << ", FileName: " << fileName
                          << ", InstructionType: " << InstructionType
                          << ", BasicBlock: " << BB.getName().str()
                          << ", Operands: " << I.getNumOperands()
                          << ", InstructionID: ";

                // Create a std::string for raw_string_ostream
                std::string instrString;

                // Create a raw_string_ostream to capture the output of the print method
                llvm::raw_string_ostream rso(instrString);

                // Print the instruction to the raw string stream
                I.print(rso);

                // At this point, the instruction has been printed to 'instrString'
                debugInfo << rso.str();

                unsigned MyKindID = F->getContext().getMDKindID("BingeIRSrcInfo");
                MDString *MyData = MDString::get(F->getContext(), debugInfo.str());
                F->setMetadata(MyKindID, MDNode::get(F->getContext(), MyData));
            }
        }
    }

    // Return the last used metadata
    MDString *MD = MDString::get(F->getContext(), funcName);
    return MDNode::get(F->getContext(), MD);
}
void BingeIRMetadata::AddBingeIRSrcInfo(const std::string &IRTypeStr, Function *CurFn, const std::string fileName, Value *V) {
    if (!CurFn) return; // add error handling

    std::string funcName = CurFn->getName().str();

    // Generate the key for the BingeIRSrcInfo map
    std::string key = "ConditionCollector@" + fileName + "@" + funcName;

    // Insert the value with the given IR type string into the map
    BingeIRSrcInfo[key][V] = IRTypeStr;
}

const std::map<std::string, std::map<Value*, std::string>>&
BingeIRMetadata::getBingeIRSrcInfo() {
    return BingeIRSrcInfo;
}

const std::vector<llvm::Value *> BingeIRMetadata::genBingeInterestingInstructions() {
    std::vector<llvm::Value *> BingeInterestingInstructions;
    for (const auto &srcInfo : BingeIRMetadata::BingeIRSrcInfo) {
        // srcInfo is a pair whose second element is the inner map
        for (const auto &valuePair : srcInfo.second) {
            // valuePair is a pair whose first element is the Value* key
            BingeInterestingInstructions.push_back(valuePair.first);
        }
    }
    return BingeInterestingInstructions;
}