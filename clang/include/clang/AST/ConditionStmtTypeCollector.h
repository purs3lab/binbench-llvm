//
// Created by arun on 5/21/23.
//

#ifndef LLVM_CONDITIONSTMTTYPECOLLECTOR_H
#define LLVM_CONDITIONSTMTTYPECOLLECTOR_H

#include "clang/AST/ASTContext.h"  // Include ASTContext
#include "BingeFrontEndCollector.h"

namespace clang {

class ConditionStmtTypeCollector : public BingeFrontEndCollector {
public:
  ConditionStmtTypeCollector(ASTContext &Context) : Context(Context) {} // Add a constructor that takes an ASTContext

  bool VisitStmt(Stmt *s) override;
  bool TraverseFunctionDecl(FunctionDecl *FD);
private:
  ASTContext &Context;  // Store a reference to the ASTContext
  FunctionDecl *currentFD = nullptr; // Current FunctionDecl
};

}  // namespace clang

#endif // LLVM_CONDITIONSTMTTYPECOLLECTOR_H
