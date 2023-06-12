# CMakeList.txt : CMake project for OpenGL Starter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

include(FetchContent)


# Chgange this to 20, see error below
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Enable Hot Reload for MSVC compilers if supported.
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
    set(GLAD_GL_VERSION 4.6 CACHE INTERNAL "")
endif()

#set glad profile
if (NOT DEFINED GLAD_GL_PROFILE)
    #set(GLAD_GL_PROFILE core CACHE INTERNAL "")
    set(GLAD_GL_PROFILE compatibility CACHE INTERNAL "")

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
         execute_process(COMMAND ${CURL} -s -D - -X POST -d "generator=c&api=egl%3Dnone&api=gl%3D${GLAD_GL_VERSION}&profile=gl%3D${GLAD_GL_PROFILE}&api=gles1%3Dnone&profile=gles1%3Dcommon&api=gles2%3Dnone&api=glsc2%3Dnone&api=glx%3Dnone&api=vulkan%3Dnone&api=wgl%3Dnone&extensions=GL_ARB_gl_spirv%2CGL_ARB_spirv_extensions%2CVK_KHR_spirv_1_4%2CGL_AMD_debug_output%2CGL_ARB_debug_output%2CGL_EXT_debug_label%2CGL_EXT_debug_marker%2CGL_KHR_debug&options=LOADER" https://gen.glad.sh/generate OUTPUT_VARIABLE res)
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


add_subdirectory(thirdparty)

set(GRAPHICS_SRC "Graphics/GraphicsCore.h"
"Graphics/GraphicsBase.h"
"Graphics/Renderer/Buffer.h"
"Graphics/Renderer/Buffer.cpp"
"Graphics/Renderer/Camera.h"
"Graphics/Renderer/FrameBuffer.h"
"Graphics/Renderer/FrameBuffer.cpp"
"Graphics/Renderer/GraphicsContext.h"
"Graphics/Renderer/GraphicsContext.cpp"
"Graphics/Renderer/OrthographicCamera.h"
"Graphics/Renderer/OrthographicCamera.cpp"
"Graphics/Renderer/RenderCommand.h"
"Graphics/Renderer/RenderCommand.cpp"
"Graphics/Renderer/Renderer.h"
"Graphics/Renderer/Renderer.cpp"
"Graphics/Renderer/Renderer2D.h"
"Graphics/Renderer/Renderer2D.cpp"
"Graphics/Renderer/RendererAPI.h"
"Graphics/Renderer/RendererAPI.cpp"
"Graphics/Renderer/Shader.h"
"Graphics/Renderer/Shader.cpp"
"Graphics/Renderer/Texture.h"
"Graphics/Renderer/Texture.cpp"
"Graphics/Renderer/UniformBuffer.h"
"Graphics/Renderer/UniformBuffer.cpp"
"Graphics/Renderer/VertexArray.h"
"Graphics/Renderer/VertexArray.cpp"
"Graphics/Platform/OpenGL/OpenGLBuffer.h"
"Graphics/Platform/OpenGL/OpenGLBuffer.cpp"
"Graphics/Platform/OpenGL/OpenGLContext.h"
"Graphics/Platform/OpenGL/OpenGLContext.cpp"
"Graphics/Platform/OpenGL/OpenGLFrameBuffer.h"
"Graphics/Platform/OpenGL/OpenGLFrameBuffer.cpp"
"Graphics/Platform/OpenGL/OpenGLRendererAPI.h"
"Graphics/Platform/OpenGL/OpenGLRendererAPI.cpp"
"Graphics/Platform/OpenGL/OpenGLShader.h"
"Graphics/Platform/OpenGL/OpenGLShader.cpp"
"Graphics/Platform/OpenGL/OpenGLTexture.h"
"Graphics/Platform/OpenGL/OpenGLTexture.cpp"
"Graphics/Platform/OpenGL/OpenGLUniformBuffer.h"
"Graphics/Platform/OpenGL/OpenGLUniformBuffer.cpp"
"Graphics/Platform/OpenGL/OpenGLVertexArray.h"
"Graphics/Platform/OpenGL/OpenGLVertexArray.cpp"
"Graphics/stb_image.h"
)

set(SRC 
    "OpenGL Starter.cpp"
    "OpenGL Starter.h"
    "ImGuiHandler/ImGuiHandler.cpp"
    "ImGuiHandler/ImGuiHandler.h"
    ${GRAPHICS_SRC}
)

include_directories(Graphics)

# Add source to this project's executable.
add_executable (OpenGLStarter ${SRC})
target_link_libraries(OpenGLStarter glad)
target_link_libraries(OpenGLStarter glfw)
target_link_libraries(OpenGLStarter glm)
target_link_libraries(OpenGLStarter imgui)
target_link_libraries(OpenGLStarter spirv-cross-core spirv-cross-glsl spirv-cross-cpp)
target_link_libraries(OpenGLStarter shaderc)

if(IMGUI_DOCKING_BRANCH)
    target_compile_definitions(OpenGLStarter PRIVATE IMGUI_DOCKING_BRANCH_ENABLED)
endif()

message("Final CXX_STANDARD ${CMAKE_CXX_STANDARD}")
file(COPY Resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET OpenGLStarter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
