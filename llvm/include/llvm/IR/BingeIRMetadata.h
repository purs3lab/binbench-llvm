#ifndef BINBENCH_LLVM_BINGEIRMETADATA_H
#define BINBENCH_LLVM_BINGEIRMETADATA_H

#include "llvm/IR/DebugInfo.h"
#include <map>
#include <set>
#include <string>
#include <unordered_map>

namespace llvm {

class BingeIRMetadata{
public:
  static std::map<std::string, std::map<Value*, std::set<Value*>>> BingeIRInfo;
  static std::map<std::string, std::map<Value*, std::string>> BingeIRSrcInfo;
  static std::map<std::map<std::string, std::map<Value*, std::string>>, bool> JsonFileGenerated;
  static std::map<Value*, MDNode*> ValueToMDNodeMap;
  struct StmtInfo {
    std::string collectorKey; // collectorType@FileName@FunctionName
    std::string stmtStr;  // corresponding statement string
  };

  static MDNode* GenBingeMd(Function *F, std::string fileName);
  static const std::map<std::string, std::map<Value*, std::string>>&
  getBingeIRSrcInfo()  ;
  static const std::vector<llvm::Value*> genBingeInterestingInstructions();

  static bool isJsonFileGenerated(const std::map<std::string, std::map<Value*, std::string>> &BingeIRSrcInfo);
  static void generateJsonFile(const std::string &moduleName);
  static void AddValueMDNodeMapping(Value *V, MDNode *MD);
  static void
  AddBingeIRSrcInfo(const std::string &IRTypeStr, Function *CurFn, const std::string fileName, Value *V, MDNode &MD);
};

} // end namespace llvm

#endif // BINBENCH_LLVM_BINGEIRMETADATA_H

