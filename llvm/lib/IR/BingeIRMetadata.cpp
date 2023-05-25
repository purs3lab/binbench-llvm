#include "llvm/IR/BingeIRMetadata.h"
#include "llvm/IR/Function.h"  // Add this at the top of the file
#include <sstream>  // Add this at the top of the file

using namespace llvm;

void BingeIRMetadata::addIRInfo(std::string &infoType, Value *src, std::set<Value*> &dst) {
  BingeIRInfo[infoType][src] = dst;
}

MDNode* BingeIRMetadata::GenBingeMd(Function *F) {
  std::string fileName = decodeFileName(F->getName().str());
  std::string funcName = decodeFuncName(F->getName().str());

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
        std::string locStr = valueMap[&I];

        // Assuming locStr is in the format "filename:line:col".
        std::istringstream iss(locStr);
        std::string lineStr;
        std::getline(iss, lineStr, ':');
        unsigned line = std::stoi(lineStr);

        // Create a metadata string for this instruction
        MDString *MD = MDString::get(F->getContext(), funcName + ":" + std::to_string(line));
        I.setMetadata("dbg", MDNode::get(F->getContext(), MD));  // Set the debug info
      }
    }
  }

  // Return the last used metadata
  MDString *MD = MDString::get(F->getContext(), funcName);
  return MDNode::get(F->getContext(), MD);
}
