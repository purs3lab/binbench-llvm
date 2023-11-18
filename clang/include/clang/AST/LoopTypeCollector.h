#ifndef LOOPTYPECOLLECTOR_H
#define LOOPTYPECOLLECTOR_H

#include "BingeFrontEndCollector.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

namespace clang {

    class LoopTypeCollector : public BingeFrontEndCollector,
                              public RecursiveASTVisitor<LoopTypeCollector> {
    public:
        explicit LoopTypeCollector(ASTContext *Context) : Context(Context) {}

        bool TraverseFunctionDecl(FunctionDecl *FD);
        bool VisitStmt(Stmt *s);

    private:
        ASTContext *Context;
        FunctionDecl *currentFD = nullptr;
    };

}  // namespace clang

#endif  // LOOPTYPECOLLECTOR_H