#include "clang/AST/LoopTypeCollector.h"
#include "clang/Basic/SourceManager.h" // Include this line at the top

using namespace clang;

bool LoopTypeCollector::TraverseFunctionDecl(FunctionDecl *FD) {
  currentFD = FD;
  return RecursiveASTVisitor<LoopTypeCollector>::TraverseDecl(FD);
}

bool LoopTypeCollector::VisitStmt(Stmt *s) {
  std::string funcName = currentFD->getNameInfo().getName().getAsString();

  // Obtain filename (module name)
  auto &SM = currentFD->getASTContext().getSourceManager();
  std::string fileName = SM.getFilename(currentFD->getBeginLoc()).str();

  std::string loopCollectorKey = "LoopCollector@" + fileName + "@" + funcName;

  std::string stmtType;
  if (isa<ForStmt>(s)) {
    stmtType = "For";
  } else if (isa<WhileStmt>(s)) {
    stmtType = "While";
  } else if (isa<DoStmt>(s)) {
    stmtType = "Do-While";
  }

  if (!stmtType.empty()) {
    addNodeInfo(loopCollectorKey, s, stmtType);
  }

  return true;
}
