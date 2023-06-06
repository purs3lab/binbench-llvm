#include "clang/AST/BingeFrontEndCollector.h"
#include "llvm/IR/BingeIRMetadata.h"

using namespace clang;
std::map<std::string, std::map<std::string, std::map<clang::Stmt*, std::set<clang::Stmt*>>>> clang::BingeFrontEndCollector::BingeFrontEndInstrInfo;
std::map<std::string, std::map<clang::Stmt*, std::string>> clang::BingeFrontEndCollector::BingeFrontEndSrcInfo;
std::unordered_map<const clang::Stmt*, clang::BingeFrontEndCollector::StmtInfo> clang::BingeFrontEndCollector::StmtInfoMap;
std::unordered_map<llvm::Value*, clang::BingeFrontEndCollector::StmtInfo> clang::BingeFrontEndCollector::valueToStmtInfoMap;

const std::unordered_map<llvm::Value*, clang::BingeFrontEndCollector::StmtInfo>& BingeFrontEndCollector::getValueToStmtInfoMap() {
    return valueToStmtInfoMap;
}

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

bool BingeFrontEndCollector::isStmtCollectedAsBingeSrcInfo(const Stmt* S) {
    for (auto &kv : StmtInfoMap) {
        if (areStructurallyEqual(S, kv.first)) {
            return true;
        }
    }
    return false;
}

bool BingeFrontEndCollector::areStructurallyEqual(const Stmt* S1, const Stmt* S2) {
    // Check if both statements are null, or one is null
    if (!S1 || !S2) {
        return S1 == S2;
    }

    // Check if both statements have same class (e.g., IfStmt, WhileStmt etc.)
    if (S1->getStmtClass() != S2->getStmtClass()) {
        return false;
    }

    // Check children statements
    Stmt::const_child_iterator it1 = S1->child_begin(), it2 = S2->child_begin();
    for ( ; it1 != S1->child_end() && it2 != S2->child_end(); ++it1, ++it2) {
        if (!areStructurallyEqual(*it1, *it2)) {
            return false;
        }
    }

    // Check if both statements have the same number of children
    return it1 == S1->child_end() && it2 == S2->child_end();
}
