#ifndef BINBENCH_LLVM_BINGEIRMETADATA_H
#define BINBENCH_LLVM_BINGEIRMETADATA_H

#include "llvm/IR/DebugInfo.h"
#include <map>
#include <set>
#include <string>

namespace llvm {

class BingeIRMetadata : public DebugInfo {
public:
  std::map<std::string, std::map<Value*, std::set<Value*>>> BingeIRInfo;
  std::map<std::string, std::map<Value*, std::string>> BingeIRSrcInfo;

  void addIRInfo(std::string &infoType, Value *src, std::set<Value*> &dst);
  MDNode* GenBingeMd(Function *F) override;
private:
  std::string decodeFileName(const std::string &key);
  std::string decodeFuncName(const std::string &key);
};

} // end namespace llvm

#endif // BINBENCH_LLVM_BINGEIRMETADATA_H
