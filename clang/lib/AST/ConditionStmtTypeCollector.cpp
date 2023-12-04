#include "clang/AST/ConditionStmtTypeCollector.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

bool ConditionStmtTypeCollector::VisitFunctionDecl(FunctionDecl *FD) {
    currentFD = FD;
    return true;  // Continue traversal
}

bool ConditionStmtTypeCollector::VisitStmt(Stmt *s) {
    if (currentFD == nullptr)
        return true;

    std::string funcName = currentFD->getNameInfo().getName().getAsString();
    auto &SM = currentFD->getASTContext().getSourceManager();
    std::string fileName = SM.getFilename(currentFD->getBeginLoc()).str();
    std::string conditionCollectorKey = "ConditionCollector@" + fileName + "@" + funcName;

    Stmt* StatementOfInterest = nullptr;
    std::string stmtType;

    if (auto *ifStmt = dyn_cast<IfStmt>(s)) {
        stmtType = "If";
        StatementOfInterest = ifStmt->getCond();
        Collector.addNodeInfo(conditionCollectorKey, StatementOfInterest, stmtType);
        if (ifStmt->getElse()) {
            // Handle the else part
            stmtType = "Else";
            StatementOfInterest = ifStmt->getElse();
            Collector.addNodeInfo(conditionCollectorKey, StatementOfInterest, stmtType);
        }
    } else if (auto *switchStmt = dyn_cast<SwitchStmt>(s)) {
        stmtType = "Switch";
        StatementOfInterest = switchStmt->getCond();
        Collector.addNodeInfo(conditionCollectorKey, StatementOfInterest, stmtType);
    } else if (auto *gotoStmt = dyn_cast<GotoStmt>(s)) {
        stmtType = "Goto";
        StatementOfInterest = gotoStmt;
        Collector.addNodeInfo(conditionCollectorKey, StatementOfInterest, stmtType);
    } else if (auto *condOp = dyn_cast<ConditionalOperator>(s)) {
        stmtType = "?:";
        StatementOfInterest = condOp->getCond();
        Collector.addNodeInfo(conditionCollectorKey, StatementOfInterest, stmtType);
    }

    return true; // Continue visiting other statements
}