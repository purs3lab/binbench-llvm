#include "clang/AST/BingeFrontEndCollector.h"

using namespace clang;

void BingeFrontEndCollector::addNodeInfo(const std::string &Type, Stmt *p, const std::string &I) {
    BingeFrontEndSrcInfo[Type][p] = I;
}

void BingeFrontEndCollector::addNodeInfo(const std::string &Type, Stmt *p, const std::set<Stmt *> &ISet) {
    BingeFrontEndInstrInfo[Type][p->getStmtClassName()].insert(std::make_pair(p, ISet));
}