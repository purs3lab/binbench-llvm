#ifndef LOOPTYPECOLLECTOR_H
#define LOOPTYPECOLLECTOR_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

namespace clang {

class LoopTypeCollector : public RecursiveASTVisitor<LoopTypeCollector> {
public:
  explicit LoopTypeCollector(ASTContext *Context) : Context(Context) {}

  bool TraverseFunctionDecl(FunctionDecl *FD);
  bool VisitStmt(Stmt *s);

private:
  ASTContext *Context;
  FunctionDecl *currentFD = nullptr;

  void addNodeInfo(const std::string &funcName, Stmt *s, const std::string &nodeType);
};

}  // namespace clang

#endif  //
