#ifndef LLVM_BCOLLECTORUTILS_H
#define LLVM_BCOLLECTORUTILS_H

#include "llvm/BCollector/BCollectorAPI.h"

#include <string>
#include <iomanip>

namespace llvm {

class BCollectorUtils {

    public:

    template<typename T> static
    std::string hexlify(T i) {
        std::stringbuf buf;
        std::ostream os(&buf);
        os << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
        return buf.str();
    }

    BCollectorUtils() {};
    virtual ~BCollectorUtils() {}
};


} // namespace llvm
#endif // LLVM_BCOLLECTORUTILS_H