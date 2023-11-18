//
// Created by arun on 6/18/23.
//

#ifndef BINBENCH_LLVM_BINGECOLLECTCXXINFO_H
#define BINBENCH_LLVM_BINGECOLLECTCXXINFO_H
#include "BingeFrontEndCollector.h"
#include "RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h" // Include CompilerInstance
#include "clang/AST/ASTContext.h" // Include ASTContext

namespace clang {

    class ClassVTSizeCollector : public RecursiveASTVisitor<ClassVTSizeCollector> {
    public:
        ClassVTSizeCollector(ASTContext &Context, BingeFrontEndCollector &Collector)
                : Context(Context){}
        void collectMangledNameAndVTableSize(ASTContext &Context, const Decl *D);
        bool VisitCXXRecordDecl(CXXRecordDecl *D)   ;
        static std::map<std::string, std::set<std::string>> MangledClassNameToVirtualTableSizeInfo;
    private:
        ASTContext &Context;
    };

    class ClassVTSizeConsumer : public ASTConsumer {
    private:
        BingeFrontEndCollector Collector;
        ClassVTSizeCollector Visitor;

    public:
        ClassVTSizeConsumer(CompilerInstance &CI, BingeFrontEndCollector &Collector)
                : Collector(Collector), Visitor(CI.getASTContext(), Collector) {}
        void HandleTranslationUnit(ASTContext &Context);
    };

    class ClassVTSizeFEAction : public ASTFrontendAction {
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file);
    };

}  // namespace clang

#endif //BINBENCH_LLVM_BINGECOLLECTCXXINFO_H