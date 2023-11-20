#include "clang/AST/BingeCollectCXXInfo.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Mangle.h"

using namespace clang;

std::map<std::string, std::set<std::string>> clang::ClassVTSizeCollector::MangledClassNameToVirtualTableSizeInfo;

void ClassVTSizeCollector::collectMangledNameAndVTableSize(ASTContext &Context, const Decl *D) {
    if (const auto *RD = dyn_cast<CXXRecordDecl>(D)) {
        if (RD->hasDefinition() && RD->isDynamicClass()) {
            // Mangle class name
            std::string MangledName;
            llvm::raw_string_ostream MangledNameStream(MangledName);
            clang::DiagnosticsEngine &DE = Context.getDiagnostics();
            clang::ItaniumMangleContext* mangleContext_ = clang::ItaniumMangleContext::create(Context, DE);

            // Use appropriate mangling function
            // For example, mangleCXXVTable for class VTable name
            mangleContext_->mangleCXXVTable(RD, MangledNameStream);
            MangledNameStream.flush();

            // Get vtable size
            const auto &Layout = Context.getASTRecordLayout(RD);
            std::string VTableSize = std::to_string(Layout.getNonVirtualSize().getQuantity());

            // Add information to the map
            MangledClassNameToVirtualTableSizeInfo[MangledName].insert(VTableSize);
        }
    }
}

// Define the VisitCXXRecordDecl method
bool ClassVTSizeCollector::VisitCXXRecordDecl(CXXRecordDecl *D) {
    collectMangledNameAndVTableSize(Context, D);
    return true;  // returning true continues the traversal
}

// Define the HandleTranslationUnit method
void ClassVTSizeConsumer::HandleTranslationUnit(ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}

// Define the CreateASTConsumer method
std::unique_ptr<ASTConsumer> ClassVTSizeFEAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    BingeFrontEndCollector Collector;
    return std::make_unique<ClassVTSizeConsumer>(CI, Collector);
}