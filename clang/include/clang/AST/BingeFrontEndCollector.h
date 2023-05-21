//
// Created by arun on 5/21/23.
//

#ifndef LLVM_BINGEFRONTENDCOLLECTOR_H
#define LLVM_BINGEFRONTENDCOLLECTOR_H
#include <map>
#include <set>
#include <string>
#include <unordered_map>

#include "clang/AST/AST.h"
#include "llvm/IR/Value.h"

namespace clang {

class BingeFrontEndCollector {
public:
  virtual ~BingeFrontEndCollector() {}
  static bool isStmtCollectedAsBingeSrcInfo(const Stmt* S);
  static void addValueStmtInfo(llvm::Value* val, const Stmt* stmt);
protected:
  void addNodeInfo(const std::string &Type, Stmt *p, const std::string &I);
  void addNodeInfo(const std::string &Type, Stmt *p, const std::set<Stmt *> &ISet);

  struct StmtInfo {
    std::string collectorKey; // collectorType@FileName@FunctionName
    std::string stmtStr;  // corresponding statement string
  };

  static std::map<std::string, std::map<std::string, std::map<Stmt*, std::set<Stmt*>>>> BingeFrontEndInstrInfo;
  static std::map<std::string, std::map<Stmt*, std::string>> BingeFrontEndSrcInfo;
  static std::unordered_map<const Stmt*, StmtInfo> StmtInfoMap; // for constant time lookup
  static std::unordered_map<llvm::Value*, StmtInfo> valueToStmtInfoMap;
};

}  // namespace clang

#endif // LLVM_BINGEFRONTENDCOLLECTOR_H
