//
// Created by arun on 5/21/23.
//

#ifndef LLVM_BINGEFRONTENDCOLLECTOR_H
#define LLVM_BINGEFRONTENDCOLLECTOR_H
#include <map>
#include <set>
#include <string>

#include "clang/AST/AST.h"

namespace clang {

class BingeFrontEndCollector {
public:
  virtual ~BingeFrontEndCollector() {}

protected:
  void addNodeInfo(const std::string &Type, Stmt *p, const std::string &I);
  void addNodeInfo(const std::string &Type, Stmt *p, const std::set<Stmt *> &ISet);

  static std::map<std::string, std::map<std::string, std::map<Stmt*, std::set<Stmt*>>>> BingeFrontEndInstrInfo;
  static std::map<std::string, std::map<Stmt*, std::string>> BingeFrontEndSrcInfo;
};

}  // namespace clang

#endif // LLVM_BINGEFRONTENDCOLLECTOR_H
