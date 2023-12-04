#include "clang/AST/LoopTypeCollector.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

bool LoopTypeCollector::VisitFunctionDecl(FunctionDecl *FD) {
    currentFD = FD;
    return true;  // Continue traversal
}

bool LoopTypeCollector::VisitStmt(Stmt *s) {
    if (currentFD == nullptr)
        return true;

    std::string funcName = currentFD->getNameInfo().getName().getAsString();
    auto &SM = currentFD->getASTContext().getSourceManager();
    std::string fileName = SM.getFilename(currentFD->getBeginLoc()).str();
    std::string loopCollectorKey = "LoopCollector@" + fileName + "@" + funcName;

    std::string stmtType;
    Stmt* StatementOfInterest = s;  // Directly use the statement

    if (isa<ForStmt>(s)) {
        stmtType = "For";
    } else if (isa<WhileStmt>(s)) {
        stmtType = "While";
    } else if (isa<DoStmt>(s)) {
        stmtType = "Do-While";
    }

    if (!stmtType.empty()) {
        Collector.addNodeInfo(loopCollectorKey, StatementOfInterest, stmtType);
    }

    return true; // Continue visiting other statements
}
