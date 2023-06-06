#include "clang/AST/ConditionStmtTypeCollector.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

bool ConditionStmtTypeCollector::VisitFunctionDecl(FunctionDecl *FD) {
    currentFD = FD;
    // Continue traversal
    return true;
}


bool ConditionStmtTypeCollector::VisitStmt(Stmt *s) {
    if (currentFD == NULL)
        return true;

    std::string funcName = currentFD->getNameInfo().getName().getAsString();

    // Obtain filename (module name)
    auto &SM = currentFD->getASTContext().getSourceManager();
    std::string fileName = SM.getFilename(currentFD->getBeginLoc()).str();

    std::string conditionCollectorKey = "ConditionCollector@" + fileName + "@" + funcName;
    Stmt* StatementOfInterest = NULL;
    std::string stmtType;
    if (isa<IfStmt>(s)) {
        stmtType = "If";
        StatementOfInterest = dyn_cast<IfStmt>(s)->getCond();
    } else if (isa<SwitchStmt>(s)) {
        stmtType = "Switch";
        StatementOfInterest = dyn_cast<SwitchStmt>(s)->getCond();
    } else if (isa<GotoStmt>(s)) {
        stmtType = "Goto";
        StatementOfInterest = dyn_cast<GotoStmt>(s);
    } else if (isa<ConditionalOperator>(s)) {
        stmtType = "?:";
        StatementOfInterest = dyn_cast<ConditionalOperator>(s)->getCond();
    }

    if (!stmtType.empty()) {
        Collector.addNodeInfo(conditionCollectorKey, StatementOfInterest, stmtType);
    }

    return true;
}