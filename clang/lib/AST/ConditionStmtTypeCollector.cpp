#include "clang/AST/ConditionStmtTypeCollector.h"
using namespace clang;

bool ConditionStmtTypeCollector::TraverseFunctionDecl(FunctionDecl *FD) {
  currentFD = FD;
  return true;
}

bool ConditionStmtTypeCollector::VisitStmt(Stmt *s) {
  std::string funcName = currentFD->getNameInfo().getName().getAsString();
    if (isa<IfStmt>(s)) {
      addNodeInfo(funcName, s, "If");
    } else if (isa<SwitchStmt>(s)) {
      addNodeInfo(funcName, s, "Switch");
    } else if (isa<GotoStmt>(s)) {
      addNodeInfo(funcName, s, "Goto");
    } else if (isa<ConditionalOperator>(s)) {
      addNodeInfo(funcName, s, "?:");
    }
  return true;
}