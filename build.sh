#/bin/bash

set -e

TOOLCHAIN_ROOT=`pwd`
PROTODEF_DIR="$TOOLCHAIN_ROOT/metainfo_proto"
LLVM_B_DIR="$TOOLCHAIN_ROOT/build"
LLVM_DIR="$TOOLCHAIN_ROOT"
PROTO="shuffleInfo.proto"
SHUFFLEINFO="shuffleInfo.so"
CC_HDR="shuffleInfo.pb.h"
C_HDR="shuffleInfo.pb-c.*"
PROTO_C="shuffleInfo.pb.cc"
PROTO_PY="shuffleInfo_pb2.py"

REBUILD_PROTOBUF=0

while getopts 'p' flag; do
  case "${flag}" in
    p) REBUILD_PROTOBUF=1 ;;
  esac
done

cd $PROTODEF_DIR
if [ $REBUILD_PROTOBUF ]
then
    echo "Building protobuf"
    # Building Protobuf
    wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protobuf-all-3.19.4.tar.gz
    tar xvf protobuf-all-3.19.4.tar.gz
    rm protobuf-all-3.19.4.tar.gz*
    cd protobuf-3.19.4
    cd cmake
    [ ! -d "./build" ] && mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC" -Dprotobuf_BUILD_TESTS=OFF ../
    make -j$(nproc)
    sudo make install
    cd ../
else
    echo "Found protobuf! reinstall using commands in build.sh if you encounter version issues"
fi

if [ ! -f $PROTODEF_DIR/$CC_HDR ] || [ $REBUILD_PROTOBUF ]
then
    cd $PROTODEF_DIR

    echo "Building protobuf definition"
    echo $(pwd)
    protoc --cpp_out=$PWD shuffleInfo.proto 
    protoc --python_out=$PWD shuffleInfo.proto
    clang++ -fPIC -shared shuffleInfo.pb.cc -o shuffleInfo.so `pkg-config --cflags --libs protobuf`
    cd $TOOLCHAIN_ROOT

    LIB1="/usr/lib"
    LIB2="/usr/local/lib"

    USER=`whoami`
    chmod 755 $PROTODEF_DIR/$SHUFFLEINFO
    chown $USER:$USER $PROTODEF_DIR/$SHUFFLEINFO $PROTODEF_DIR/$PROTO_C $PROTODEF_DIR/$PROTO_PY

    sudo cp $PROTODEF_DIR/$SHUFFLEINFO $LIB1/$SHUFFLEINFO
    sudo cp $PROTODEF_DIR/$SHUFFLEINFO $LIB2/$SHUFFLEINFO
    sudo cp $PROTODEF_DIR/$SHUFFLEINFO $LIB2/lib$SHUFFLEINFO
else
    echo "building protobuf definitions, skipped, use -p to rebuild"
fi


[ ! -d "./build" ] && mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXE_LINKER_FLAGS_DEBUG="-I/usr/local/include -L/usr/local/lib -lprotobuf -lpthread" -DLLVM_ENABLE_RTTI=ON -DLLVM_TARGETS_TO_BUILD=all -DLLVM_ENABLE_PROJECTS="clang" -DCMAKE_INSTALL_TYPE=Debug ../llvm
cp $PROTODEF_DIR/$CC_HDR $LLVM_DIR/build/include/llvm/Support/$CC_HDR
MODIFIED_LINK1="$LLVM_DIR/build/lib/MC/CMakeFiles/LLVMMC.dir/link.txt"
MODIFIED_LINK2="$LLVM_DIR/build/tools/lto/CMakeFiles/LTO.dir/link.txt"
MODIFIED_LINK3="$LLVM_DIR/build/tools/clang/tools/libclang/CMakeFiles/libclang.dir/link.txt"
MODIFIED_LINK4="$LLVM_DIR/build/tools/clang/tools/c-index-test/CMakeFiles/c-index-test.dir/link.txt"

# Adding /usr/lib/shuffleInfo.so
sed -i '/LLVMMC.dir/s/$/\ \/usr\/lib\/shuffleInfo\.so/' $MODIFIED_LINK1

# Adding -I/usr/local/include -L/usr/local/lib -lprotobuf
sed -i 's/$/\-I\/usr\/local\/include\ \-L\/usr\/local\/lib\ \-lprotobuf/' $MODIFIED_LINK2 
sed -i 's/$/\-I\/usr\/local\/include\ \-L\/usr\/local\/lib\ \-lprotobuf/' $MODIFIED_LINK3 
sed -i 's/$/\-I\/usr\/local\/include\ \-L\/usr\/local\/lib\ \-lprotobuf/' $MODIFIED_LINK4 
make -j$(nproc)
cd ../../

cd $TOOLCHAIN_ROOT

echo "Ready to roll!"
