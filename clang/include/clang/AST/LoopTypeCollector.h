#ifndef LOOPTYPECOLLECTOR_H
#define LOOPTYPECOLLECTOR_H

#include "BingeFrontEndCollector.h"
#include "RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTContext.h"

namespace clang {

    class LoopTypeCollector : public RecursiveASTVisitor<LoopTypeCollector> {
    public:
        LoopTypeCollector(ASTContext &Context, BingeFrontEndCollector &Collector)
                : Context(Context), Collector(Collector) {}

        bool VisitStmt(Stmt *s);
        bool VisitFunctionDecl(FunctionDecl *FD);

    private:
        ASTContext &Context;
        BingeFrontEndCollector &Collector;
        FunctionDecl *currentFD = nullptr;
    };

    class LoopTypeConsumer : public ASTConsumer {
    private:
        BingeFrontEndCollector Collector;
        LoopTypeCollector Visitor;

    public:
        LoopTypeConsumer(CompilerInstance &CI, BingeFrontEndCollector &Collector)
                : Collector(Collector), Visitor(CI.getASTContext(), Collector) {}

        void HandleTranslationUnit(ASTContext &Context) override {
            Visitor.TraverseDecl(Context.getTranslationUnitDecl());
        }
    };

    class LoopTypeFEAction : public ASTFrontendAction {
    public:
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
            BingeFrontEndCollector Collector;
            return std::make_unique<LoopTypeConsumer>(CI, Collector);
        }
    };

}  // namespace clang


#endif  // LOOPTYPECOLLECTOR_H