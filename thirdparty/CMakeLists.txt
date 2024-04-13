include(ExternalProject)

set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
set(SHADERC_SKIP_INSTALL ON)


# Set minimum cxx_standard required for spirv-tools to compile
set(CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested." FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

# Add the FetchContent module
include(FetchContent)

set(SPIRV_TOOLS_TAG "sdk-1.3.261.1")
set(SPIRV_HEADERS_TAG "sdk-1.3.261.1")
set(SPIRV_CROSS_TAG "sdk-1.3.261.1")
set(GLSLANG_TAG "sdk-1.3.261.1")
set(SHADERC_TAG "v2023.2")
#set(BUILD_SHARED_LIBS 1)

# Declare the SPIRV-Tools library and its source URL
FetchContent_Declare(
    spirv-tools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
    GIT_TAG        ${SPIRV_TOOLS_TAG}
)





#message("spirv-tools : ${SPIRV_TOOLS}")
#set_property(TARGET SPIRV-Tools-shared PROPERTY CXX_STANDARD 17 FORCE)
#
#set_property(TARGET SPIRV-Tools-static PROPERTY CXX_STANDARD 17 FORCE)
#set_property(TARGET SPIRV-Tools-shared PROPERTY CXX_STANDARD 17)
#set_target_properties(SPIRV-Tools-static PROPERTIES
#    CXX_STANDARD 17
#    CXX_STANDARD_REQUIRED ON
#  )
#set_target_properties(SPIRV-Tools-shared PROPERTIES
#    CXX_STANDARD 17
#    CXX_STANDARD_REQUIRED ON
#  )




# NOTE : THIS IS TEMPORARY UNTIL I FIGURE OUT WHAT IS HAPPENING WITH SPIRV-TOOLS small_vector.h
#       FAILED: _deps/spirv-tools-build/source/opt/CMakeFiles/SPIRV-Tools-opt.dir/set_spec_constant_default_value_pass.cpp.obj 
#         C:\PROGRA~1\MICROS~3\2022\COMMUN~1\VC\Tools\MSVC\1435~1.322\bin\Hostx64\x64\cl.exe  /nologo /TP -DSPIRV_COLOR_TERMINAL -DSPIRV_WINDOWS -I"C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src" -I"C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\include" -I"C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-headers-src\include" -I"C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-build" /DWIN32 /D_WINDOWS /W3 /GR /EHsc -DIMGUI_IMPL_OPENGL_LOADER_GLAD2 /MDd /Zi /Ob0 /Od /RTC1   /MP4 -DSPIRV_CHECK_CONTEXT -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /wd4800 /wd4819 /WX /EHs -std:c++20 /showIncludes /Fo_deps\spirv-tools-build\source\opt\CMakeFiles\SPIRV-Tools-opt.dir\set_spec_constant_default_value_pass.cpp.obj /Fd_deps\spirv-tools-build\source\opt\CMakeFiles\SPIRV-Tools-opt.dir\SPIRV-Tools-opt.pdb /FS -c "C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source\opt\set_spec_constant_default_value_pass.cpp"
#       C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source\util\small_vector.h(186): error C2220: the following warning is treated as an error
#         C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source/util/small_vector.h(185): note: while compiling class template member function 'bool spvtools::utils::operator ==(const std::vector<uint32_t,std::allocator<uint32_t>> &,const spvtools::utils::SmallVector<uint32_t,2> &)'
#         C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source/util/small_vector.h(191): note: see reference to function template instantiation 'bool spvtools::utils::operator ==(const std::vector<uint32_t,std::allocator<uint32_t>> &,const spvtools::utils::SmallVector<uint32_t,2> &)' being compiled
#         C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source/opt/instruction.h(93): note: see reference to class template instantiation 'spvtools::utils::SmallVector<uint32_t,2>' being compiled
#       C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source\util\small_vector.h(186): warning C5232: in C++20 this comparison calls 'bool spvtools::utils::operator ==(const std::vector<uint32_t,std::allocator<uint32_t>> &,const spvtools::utils::SmallVector<uint32_t,2> &)' recursively
#         C:\Users\yejne\Desktop\Tests\OpenGL Starter\out\build\x64-Debug\_deps\spirv-tools-src\source/util/small_vector.h(186): note: to simplify migration, consider the temporary use of /Wv:18 flag with the version of the compiler with which you used to build without warnings

## Declare the SPIRV-Headers library and its source URL

#FetchContent_Declare(
#    spirv-tools 
#    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
#    GIT_TAG        v2023.2
#)
#
#FetchContent_GetProperties(spirv-tools)
#
#if(NOT spirv-tools_POPULATED)
#   FetchContent_Populate(spirv-tools)
#
#
#
#   # Disable unneeded modules to speed up build
#
#
#   # Make sure to build the necessary sub-projects for SPIRV-Tools
#   add_subdirectory(${spirv-tools_SOURCE_DIR} ${spirv-tools_BINARY_DIR})
#
#   # Ensure SPIRV-Tools library is available to linker and has proper permissions
#   set_property(TARGET SPIRV-Tools-static PROPERTY IMPORTED_LOCATION ${spirv-tools_BINARY_DIR}/source/${CMAKE_SHARED_LIBRARY_PREFIX}SPIRV${CMAKE_SHARED_LIBRARY_SUFFIX})
#
#endif()

FetchContent_Declare(
    spirv-headers
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
    GIT_TAG        ${SPIRV_HEADERS_TAG}
)

# Build and add the SPIRV-Headers library to the project
FetchContent_MakeAvailable(spirv-headers)
FetchContent_MakeAvailable(spirv-tools)


# Declare the spirv-cross library and its source URL
FetchContent_Declare(
    spirv-cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG        ${SPIRV_CROSS_TAG}
)

# Build and add the spirv-cross library to the project
FetchContent_MakeAvailable(spirv-cross)



set(spirv-cross_SRC_DIR ${spirv-cross_SOURCE_DIR} CACHE INTERNAL "SPIRV-Cross source directory")

message("spirv includes :  ${spirv-cross_SOURCE_DIR}/include")

#
## Declare the glslang library and its source URL
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG        ${GLSLANG_TAG}
)

# Declare the shaderc library and its source URL
FetchContent_Declare(
    shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG        ${SHADERC_TAG}
)

FetchContent_MakeAvailable(glslang shaderc)

message("Build type is : ${CMAKE_BUILD_TYPE}")
message("Current Flags :
        CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}
        CMAKE_CXX_FLAGS_DEBUG : ${CMAKE_CXX_FLAGS_DEBUG}
        CMAKE_CXX_FLAGS_RELEASE : ${CMAKE_CXX_FLAGS_RELEASE}
        CMAKE_C_FLAGS : ${CMAKE_C_FLAGS}
        CMAKE_C_FLAGS_DEBUG : ${CMAKE_C_FLAGS_DEBUG}
        CMAKE_C_FLAGS_RELEASE : ${CMAKE_C_FLAGS_RELEASE}")


message("SPIRV-Tools tag : ${SPIRV_TOOLS_TAG}")
message("SPIRV-Headers tag : ${SPIRV_HEADERS_TAG}")
message("SPIRV-Cross tag : ${SPIRV_CROSS_TAG}")
message("GLSLang tag : ${GLSLANG_TAG}")
message("ShaderC tag : ${SHADERC_TAG}")

message("BUILD type : ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug build for shaderc and glslang")
    target_compile_options(glslang PRIVATE /MDd)
    target_compile_options(shaderc_util PRIVATE /MDd)
    target_compile_options(shaderc PRIVATE /MDd)
    set_target_properties(shaderc PROPERTIES LINK_FLAGS "/FORCE:MULTIPLE")
else()
    message("Release build for shaderc and glslang")
    target_compile_options(glslang PRIVATE /MD)
    target_compile_options(shaderc_util PRIVATE /MD)
    target_compile_options(shaderc PRIVATE /MD)
endif()





message("spirv includes :  ${shaderc_SOURCE_DIR}/include")