cmake_minimum_required(VERSION 3.0.0)
project(bioanalysis VERSION 0.1.0 LANGUAGES C CXX)

add_executable( bioanalysis src/main.cpp src/SerialDataReceiver.cpp ) # put your cpp files in a src/ directory
target_include_directories( bioanalysis PUBLIC include ) # put the headers in an include/ directory
target_compile_features( bioanalysis PUBLIC cxx_std_23 )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
