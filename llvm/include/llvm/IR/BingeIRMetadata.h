#ifndef BINBENCH_LLVM_BINGEIRMETADATA_H
#define BINBENCH_LLVM_BINGEIRMETADATA_H

#include "llvm/IR/DebugInfo.h"
#include <map>
#include <set>
#include <string>
#include <unordered_map>

namespace llvm {

    class BingeIRMetadata{
    public:
        static std::map<std::string, std::map<Value*, std::set<Value*>>> BingeIRInfo;
        static std::map<std::string, std::map<Value*, std::string>> BingeIRSrcInfo;

        struct StmtInfo {
            std::string collectorKey; // collectorType@FileName@FunctionName
            std::string stmtStr;  // corresponding statement string
        };

        static MDNode* GenBingeMd(Function *F, std::string fileName);
        static void AddBingeIRSrcInfo(const std::string &IRTypeStr, Function *CurFn,
                                      const std::string fileName, Value *V);
        static const std::map<std::string, std::map<Value*, std::string>>&
        getBingeIRSrcInfo()  ;
        static const std::vector<llvm::Value*> genBingeInterestingInstructions();
    };

} // end namespace llvm

#endif // BINBENCH_LLVM_BINGEIRMETADATA_H