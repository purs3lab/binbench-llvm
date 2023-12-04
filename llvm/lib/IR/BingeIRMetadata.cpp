#include "llvm/IR/BingeIRMetadata.h"
#include "clang/AST/BingeCollectCXXInfo.h"
#include "llvm/IR/Function.h" // Add this at the top of the file
#include <iostream>
#include <sstream> // Add this at the top of the file
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

using namespace llvm;
std::map<std::string, std::map<Value*, std::set<Value*>>> BingeIRMetadata::BingeIRInfo;
std::map<std::string, std::map<Value*, std::string>> BingeIRMetadata::BingeIRSrcInfo;
std::map<std::map<std::string, std::map<Value*, std::string>>, bool> BingeIRMetadata::JsonFileGenerated;
std::map<Value*, MDNode*> BingeIRMetadata::ValueToMDNodeMap;

void BingeIRMetadata::AddValueMDNodeMapping(Value *V, MDNode *MD) {
    if (V && MD) {
        ValueToMDNodeMap[V] = MD;
    }
}

static std::string serializeValue(Value *val) {
    std::stringstream ss;
    if (!val) {
        ss << "null";
    } else if (BingeIRMetadata::ValueToMDNodeMap.find(val) != BingeIRMetadata::ValueToMDNodeMap.end()) {
        MDNode *MD = BingeIRMetadata::ValueToMDNodeMap[val];

        // Assuming the MDNode contains filename, line, and column in that order
        if (MD->getNumOperands() >= 3) {
            MDString *FilenameMD = dyn_cast<MDString>(MD->getOperand(0));
            ConstantInt *LineMD = dyn_cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(MD->getOperand(1))->getValue());
            ConstantInt *ColMD = dyn_cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(MD->getOperand(2))->getValue());

            if (FilenameMD && LineMD && ColMD) {
                ss << "File: " << FilenameMD->getString().str()
                   << ", Line: " << LineMD->getValue().getZExtValue()
                   << ", Column: " << ColMD->getValue().getZExtValue();
            } else {
                ss << "Incomplete debug information in MDNode";
            }
        } else {
            ss << "Insufficient operands in MDNode for debug information";
        }
    } else {
        ss << "Value has no associated debug information";
    }
    return ss.str();
}

void BingeIRMetadata::generateJsonFile(const std::string &filename) {
    json j;
    // First, handle the BingeIRSrcInfo
    for (const auto &entry : BingeIRSrcInfo) {
        json moduleJson;
        for (const auto &valPair : entry.second) {
            std::string valueStr = serializeValue(valPair.first);
            json valueJson;
            valueJson["Details"] = valueStr;
            valueJson["IRType"] = valPair.second;
            moduleJson.push_back(valueJson);
        }
        j["BingeIRSrcInfo"][entry.first] = moduleJson;
    }

    // Now, handle the MangledClassNameToVirtualTableSizeInfo independently
    json mangledClassJson;
    for (const auto &entry : clang::ClassVTSizeCollector::MangledClassNameToVirtualTableSizeInfo) {
        json classJson;
        for (const auto &size : entry.second) {
            classJson.push_back(size);
        }
        mangledClassJson[entry.first] = classJson;
    }
    j["MangledClassNameToVTableSizeInfo"] = mangledClassJson;

    // Write JSON to file
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4); // 4 spaces for pretty printing
        file.close();
        std::cout << "JSON file created: " << filename << std::endl;
        JsonFileGenerated[BingeIRSrcInfo] = true; // Set the flag to true after successful file generation
    } else {
        std::cerr << "Unable to open file for writing JSON data" << std::endl;
    }
}

bool BingeIRMetadata::isJsonFileGenerated(const std::map<std::string, std::map<Value*, std::string>> &BingeIRSrcInfo) {
    return JsonFileGenerated[BingeIRSrcInfo];
}

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
void BingeIRMetadata::AddBingeIRSrcInfo(const std::string &IRTypeStr, Function *CurFn,
                                        const std::string fileName, Value *V, llvm::MDNode &MD) {
    if (!CurFn) return; // add error handling

    std::string funcName = CurFn->getName().str();

    // Generate the key for the BingeIRSrcInfo map
    std::string key;
    if (IRTypeStr == "Branch" || IRTypeStr == "Switch")
        key = "ConditionCollector@" + fileName + "@" + funcName;
    else if (IRTypeStr == "For" || IRTypeStr == "While" || IRTypeStr == "DoWhile")
        key = "LoopCollector@" + fileName + "@" + funcName;
    else
        key = "BingeIRSrcInfo@" + fileName + "@" + funcName;
    // Insert the value with the given IR type string into the map
    BingeIRSrcInfo[key][V] = IRTypeStr;
    // Add the mapping between Value and MDNode
    AddValueMDNodeMapping(V, &MD);
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