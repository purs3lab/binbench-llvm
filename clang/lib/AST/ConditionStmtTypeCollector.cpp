#include "clang/AST/ConditionStmtTypeCollector.h"
#include "clang/Basic/SourceManager.h" // Include this line at the top

using namespace clang;

bool ConditionStmtTypeCollector::TraverseFunctionDecl(FunctionDecl *FD) {
  currentFD = FD;
  return RecursiveASTVisitor<ConditionStmtTypeCollector>::TraverseDecl(FD);
}

bool ConditionStmtTypeCollector::VisitStmt(Stmt *s) {
  std::string funcName = currentFD->getNameInfo().getName().getAsString();

  // Obtain filename (module name)
  auto &SM = currentFD->getASTContext().getSourceManager();
  std::string fileName = SM.getFilename(currentFD->getBeginLoc()).str();

  std::string conditionCollectorKey = "ConditionCollector@" + fileName + "@" + funcName;

  std::set<Stmt *> stmtSet;

  if (isa<IfStmt>(s) || isa<SwitchStmt>(s) || isa<GotoStmt>(s) || isa<ConditionalOperator>(s)) {
    for (Stmt *childStmt : s->children()) {
      stmtSet.insert(childStmt);
    }
  }

  //addNodeInfo(conditionCollectorKey, s, stmtSet);

  std::string stmtType;
  if (isa<IfStmt>(s)) {
    stmtType = "If";
  } else if (isa<SwitchStmt>(s)) {
    stmtType = "Switch";
  } else if (isa<GotoStmt>(s)) {
    stmtType = "Goto";
  } else if (isa<ConditionalOperator>(s)) {
    stmtType = "?:";
  }

  if (!stmtType.empty()) {
    addNodeInfo(conditionCollectorKey, s, stmtType);
  }

  return true;
}
