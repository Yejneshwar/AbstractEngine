# CMakeList.txt : CMake project for OpenGL Starter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("OpenGL Starter" VERSION 1.0)

#if tags not set set them
if (NOT DEFINED GLFW_TAG)
	set(GLFW_TAG "" CACHE INTERNAL "")
endif()
if (NOT DEFINED GLM_TAG)
    set(GLM_TAG "" CACHE INTERNAL "")
endif()
if (NOT DEFINED IMGUI_TAG)
	set(IMGUI_TAG "" CACHE INTERNAL "")
endif()
if (NOT DEFINED IMGUI_DOCKING_BRANCH)
    set(IMGUI_DOCKING_BRANCH OFF CACHE BOOL "")
endif()

#set glad version
if (NOT DEFINED GLAD_GL_VERSION)
    set(GLAD_GL_VERSION 4.5 CACHE INTERNAL "")
endif()

#set glad profile
if (NOT DEFINED GLAD_GL_PROFILE)
    set(GLAD_GL_PROFILE core CACHE INTERNAL "")
endif()

#Print all vars
message(STATUS "GLFW_TAG : ${GLFW_TAG}")
message(STATUS "GLM_TAG : ${GLM_TAG}")
message(STATUS "IMGUI_TAG : ${IMGUI_TAG}")
message(STATUS "IMGUI_DOCKING_BRANCH : ${IMGUI_DOCKING_BRANCH}")
message(STATUS "GLAD_GL_VERSION : ${GLAD_GL_VERSION}")
message(STATUS "GLAD_GL_PROFILE : ${GLAD_GL_PROFILE}")
message(STATUS "glad_SOURCE_DIR : ${glad_SOURCE_DIR}")
message(STATUS "glad_INSTALLED_VERSION : ${glad_INSTALLED_VERSION}")




 if ("${glad_INSTALLED_VERSION}" STREQUAL "${GLAD_GL_VERSION}-${GLAD_GL_PROFILE}")
     message(STATUS "Avoiding repeated download of glad gl ${GLAD_GL_VERSION}/${GLAD_GL_PROFILE}")
     message(STATUS "GLAD Source directory at ${glad_SOURCE_DIR}")
     set(glad_SOURCE_DIR ${glad_LAST_SOURCE_DIR})
 else ()
     find_program(CURL NAMES curl curl.exe)
     if (NOT CURL)
         message(STATUS "Could NOT find curl!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
         set(glad_SOURCE_DIR glad)
     else ()
         execute_process(COMMAND ${CURL} -s -D - -X POST -d generator=c&api=egl%3Dnone&api=gl%3D${GLAD_GL_VERSION}&profile=gl%3D${GLAD_GL_PROFILE}&api=gles1%3Dnone&profile=gles1%3Dcommon&api=gles2%3Dnone&api=glsc2%3Dnone&api=glx%3Dnone&api=vulkan%3Dnone&api=wgl%3Dnone&options=LOADER https://gen.glad.sh/generate OUTPUT_VARIABLE res)
         string(REGEX MATCH "Location: ([A-Za-z0-9_\\:/\\.]+)" location "${res}")
         set(location "${CMAKE_MATCH_1}")
         message("Glad Location : ${loaction}")
         string(REGEX REPLACE "/$" "" location "${location}")
         string(APPEND location "/glad.zip")
         message(STATUS "Downloading glad extension for gl ${GLAD_GL_VERSION}/${GLAD_GL_PROFILE} from ${location} ")
         FetchContent_Declare(
                 glad
                 URL ${location}
                 DOWNLOAD_EXTRACT_TIMESTAMP true
         )
         FetchContent_MakeAvailable(glad)
         set(glad_INSTALLED_VERSION ${GLAD_GL_VERSION}-${GLAD_GL_PROFILE} CACHE INTERNAL "")
         set(glad_LAST_SOURCE_DIR ${glad_SOURCE_DIR} CACHE INTERNAL "")
     endif ()
 endif ()

add_library(
       glad
       ${glad_SOURCE_DIR}/src/gl.c
       ${glad_SOURCE_DIR}/include/glad/gl.h
       ${glad_SOURCE_DIR}/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC ${glad_SOURCE_DIR}/include)


FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG ${GLFW_TAG}
)
list(APPEND components glfw)



FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG ${GLM_TAG}
)
list(APPEND components glm)

if(IMGUI_DOCKING_BRANCH)
    set(IMGUI_BRANCH "docking")
else()
    set(IMGUI_BRANCH ${IMGUI_TAG})
endif()

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG ${IMGUI_BRANCH}
)
list(APPEND components imgui)



FetchContent_MakeAvailable(${components})

file(GLOB imgui_SRC ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)
add_library(
        imgui
        ${imgui_SRC}
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
)
target_link_libraries(imgui PUBLIC glfw glad)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GLAD2")
target_include_directories(
        imgui
        PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)


# Add source to this project's executable.
add_executable (OpenGLStarter "OpenGL Starter.cpp" "OpenGL Starter.h" "ImGuiHandler/ImGuiHandler.cpp" "ImGuiHandler/ImGuiHandler.h")
target_link_libraries(OpenGLStarter glad)
target_link_libraries(OpenGLStarter glfw)
target_link_libraries(OpenGLStarter glm)
target_link_libraries(OpenGLStarter imgui)

if(IMGUI_DOCKING_BRANCH)
    target_compile_definitions(OpenGLStarter PRIVATE IMGUI_DOCKING_BRANCH_ENABLED)
endif()



if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET OpenGLStarter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
