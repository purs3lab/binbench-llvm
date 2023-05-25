#include "llvm/IR/BingeIRMetadata.h"

using namespace llvm;

void BingeIRMetadata::addIRInfo(std::string &infoType, Value *src, std::set<Value*> &dst) {
  BingeIRInfo[infoType][src] = dst;
}

MDNode* BingeIRMetadata::GenBingeMd(Function *F) {
  std::string fileName = decodeFileName(F->getName().str());
  std::string funcName = decodeFuncName(F->getName().str());

  auto iter = BingeIRSrcInfo.find("ConditionCollector@" + fileName + "@" + funcName);
  if (iter != BingeIRSrcInfo.end()) {
    // Fetch the map of Values to string.
    std::map<Value*, std::string> &valueMap = iter->second;

    // Go through each value and fetch its location.
    for (auto &valuePair : valueMap) {
      // Get the value.
      Value* val = valuePair.first;

      // Get the string representing the location.
      std::string locStr = valuePair.second;

      // Assuming locStr is in the format "filename:line:col".
      std::istringstream iss(locStr);
      std::string file, lineStr, colStr;
      std::getline(iss, file, ':');
      std::getline(iss, lineStr, ':');
      std::getline(iss, colStr, ':');
      unsigned line = std::stoi(lineStr);
      unsigned col = std::stoi(colStr);

      // Create DIFile.
      DIFile *diFile = DIFile::get(F->getContext(), file, ".");

      // Create DISubprogram.
      DISubprogram *SP = DISubprogram::get(F->getContext(), funcName, funcName, diFile, line, nullptr, 0, nullptr, nullptr);

      // Create DILocation.
      DILocation *Loc = DILocation::get(F->getContext(), line, col, SP);

      // Set the debug info for the value.
      val->setMetadata("dbg", Loc);
    }

    // Get the metadata string for this function.
    MDString *MD = MDString::get(F->getContext(), funcName);
    return MDNode::get(F->getContext(), MD);
  }

  return nullptr;
}
