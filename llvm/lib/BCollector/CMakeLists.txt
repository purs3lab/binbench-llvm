add_llvm_component_library(LLVMBC 
  BCollectorAPI.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/BCollector

  LINK_COMPONENTS
  )