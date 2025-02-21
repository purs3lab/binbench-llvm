#ifndef LLVM_BCOLLECTORUTILS_H
#define LLVM_BCOLLECTORUTILS_H

#include "llvm/BCollector/BCollectorAPI.h"

#include <iomanip>
#include <string>

namespace llvm {

/// @brief Utility class for BCollector
class BCollectorUtils {

    public:

    /// @brief Hexlify for dubugging 
    template<typename T> static
    std::string hexlify(T I) {
        std::stringbuf Buf;
        std::ostream Os(&Buf);
        Os << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << I;
        return Buf.str();
    }

    BCollectorUtils() {};
    virtual ~BCollectorUtils() {}
};


} // namespace llvm
#endif // LLVM_BCOLLECTORUTILS_H
