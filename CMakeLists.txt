cmake_minimum_required(VERSION 3.28.0)
project(bioanalysis VERSION 0.1.0 LANGUAGES C CXX)
FILE(GLOB imgui_sources lib/imgui/*.cpp)
FILE(GLOB implot_sources lib/implot/*.cpp)
add_executable( bioanalysis src/main.cpp src/SerialDataReceiver.cpp ${imgui_sources} ${implot_sources}) # put your cpp files in a src/ directory
target_include_directories( bioanalysis PUBLIC include lib/imgui lib/implot) # put the headers in an include/ directory
target_compile_features( bioanalysis PUBLIC cxx_std_23 )

find_package(glfw3 REQUIRED) 
find_package(OpenGL REQUIRED)
target_link_libraries(bioanalysis PRIVATE glfw)
target_link_libraries(bioanalysis PRIVATE OpenGL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)

target_compile_options(bioanalysis PRIVATE ${GLFW3_CFLAGS}) 

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
