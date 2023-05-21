#include "clang/AST/LoopTypeCollector.h"
using namespace clang;

bool LoopTypeCollector::TraverseFunctionDecl(FunctionDecl *FD) {
  currentFD = FD;
  return RecursiveASTVisitor<LoopTypeCollector>::TraverseDecl(FD);
}

bool LoopTypeCollector::VisitStmt(Stmt *s) {
  std::string funcName = currentFD->getNameInfo().getName().getAsString();
  if (isa<ForStmt>(s)) {
    addNodeInfo(funcName, s, "For");
  } else if (isa<WhileStmt>(s)) {
    addNodeInfo(funcName, s, "While");
  } else if (isa<DoStmt>(s)) {
    addNodeInfo(funcName, s, "Do-While");
  }
  return true;
}
