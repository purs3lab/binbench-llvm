#include "clang/AST/BingeFrontEndCollector.h"

using namespace clang;

void BingeFrontEndCollector::addNodeInfo(const std::string &Type, Stmt *p, const std::string &I) {
  BingeFrontEndSrcInfo[Type][p] = I;
  StmtInfo info = {Type, I};
  StmtInfoMap[p] = info;
}

void BingeFrontEndCollector::addNodeInfo(const std::string &Type, Stmt *p, const std::set<Stmt *> &ISet) {
    BingeFrontEndInstrInfo[Type][p->getStmtClassName()].insert(std::make_pair(p, ISet));
}  // Here's your new setter method

void BingeFrontEndCollector::addValueStmtInfo(llvm::Value* val, const Stmt* stmt) {
    StmtInfo stmtInfo;
    // Populate stmtInfo using stmt
    // Assuming StmtInfoMap has already been populated with stmt info
    if (StmtInfoMap.find(stmt) != StmtInfoMap.end()) {
      stmtInfo = StmtInfoMap[stmt];
    }
    // else handle the case when stmt is not found in StmtInfoMap
    valueToStmtInfoMap[val] = stmtInfo;
}
