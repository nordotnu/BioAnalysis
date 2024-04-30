cmake_minimum_required(VERSION 3.28.0)
project(bioanalysis VERSION 0.1.0 LANGUAGES C CXX)
FILE(GLOB imgui_sources lib/imgui/*.cpp)
FILE(GLOB implot_sources lib/implot/*.cpp)
FILE(GLOB libsvm_sources lib/libsvm/*.cpp)
FILE(GLOB sources src/*.cpp)

add_executable( bioanalysis ${sources} ${imgui_sources} ${implot_sources} ${libsvm_sources}) # put your cpp files in a src/ directory
target_include_directories( bioanalysis PUBLIC include lib/imgui lib/implot lib/libsvm) # put the headers in an include/ directory
target_compile_features( bioanalysis PUBLIC cxx_std_23 )

find_package(Threads REQUIRED)
find_package(glfw3 REQUIRED) 
find_package(OpenGL REQUIRED)
target_link_libraries(bioanalysis PRIVATE Threads::Threads)
target_link_libraries(bioanalysis PRIVATE glfw)
target_link_libraries(bioanalysis PRIVATE OpenGL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)

target_compile_options(bioanalysis PRIVATE ${GLFW3_CFLAGS}) 

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
