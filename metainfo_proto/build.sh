#!/bin/bash

# you will need the protobuf compiler to build these
# configure protobuf with 
#     ./configure "CFLAGS=-fPIC" "CXXFLAGS=-fPIC"

protoc --cpp_out=$PWD shuffleInfo.proto 
protoc --python_out=$PWD shuffleInfo.proto
protoc --python_out=$PWD blocks.proto
clang++ -fPIC -shared shuffleInfo.pb.cc -o shuffleInfo.so `pkg-config --cflags --libs protobuf`
