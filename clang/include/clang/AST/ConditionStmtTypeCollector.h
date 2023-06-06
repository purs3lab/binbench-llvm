#ifndef LLVM_CONDITIONSTMTTYPECOLLECTOR_H
#define LLVM_CONDITIONSTMTTYPECOLLECTOR_H

#include "BingeFrontEndCollector.h"
#include "RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h" // Include CompilerInstance
#include "clang/AST/ASTContext.h" // Include ASTContext

namespace clang {

    class ConditionStmtTypeCollector : public RecursiveASTVisitor<ConditionStmtTypeCollector> {
    public:
        ConditionStmtTypeCollector(ASTContext &Context, BingeFrontEndCollector &Collector)
                : Context(Context), Collector(Collector) {}

        bool VisitStmt(Stmt *s);
        bool VisitFunctionDecl(FunctionDecl *FD);
    private:
        ASTContext &Context;
        BingeFrontEndCollector &Collector;
        FunctionDecl *currentFD = nullptr;
    };

    class ConditionStmtTypeConsumer : public ASTConsumer {
    private:
        BingeFrontEndCollector Collector;
        ConditionStmtTypeCollector Visitor;

    public:
        ConditionStmtTypeConsumer(CompilerInstance &CI, BingeFrontEndCollector &Collector)
                : Collector(Collector), Visitor(CI.getASTContext(), Collector) {}

        void HandleTranslationUnit(ASTContext &Context) override {
            Visitor.TraverseDecl(Context.getTranslationUnitDecl());
        }
    };

    class ConditionStmtTypeFEAction : public ASTFrontendAction {
    public:
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
            BingeFrontEndCollector Collector;
            return std::make_unique<ConditionStmtTypeConsumer>(CI, Collector);
        }
    };

}  // namespace clang

#endif // LLVM_CONDITIONSTMTTYPECOLLECTOR_H