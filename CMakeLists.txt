# CMakeList.txt : CMake project for OpenGL Starter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

if(NOT DEFINED GUI_PROJECT_NAME)
   set(GUI_PROJECT_NAME "TestGui")
   #message( FATAL_ERROR "Set the project name with GUI_PROJECT_NAME." )
endif()
if(NOT DEFINED GUI_BUILD_TESTING)
   set(GUI_BUILD_TESTING OFF CACHE BOOL "")
endif()

include(FetchContent)
if(APPLE)
    include("cmake/IOS-helpers.cmake")
    if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	    set(CMAKE_TOOLCHAIN_FILE "cmake/ios.toolchain.cmake" CACHE STRING "")
    endif()
    if (NOT DEFINED PLATFORM)
	    set(PLATFORM "OS64COMBINED" CACHE STRING "")
    endif()
    if (NOT DEFINED DEPLOYMENT_TARGET)
	    set(DEPLOYMENT_TARGET "18.0" CACHE STRING "")
    endif()
    message(STATUS "PLATFORM: ${PLATFORM}")
    message(STATUS "Deployment Target ${DEPLOYMENT_TARGET}")
endif()

# Chgange this to 20, see error below
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (${GUI_PROJECT_NAME} VERSION 1.0)

message("Cmake System Name ${CMAKE_SYSTEM_NAME} - ${CMAKE_SYSTEM_VERSION} - ${CMAKE_OSX_DEPLOYMENT_TARGET}")
if(IOS)
    message("IOS Build")
    add_compile_definitions(BUILDING_METAL)
elseif(ANDROID)
	if(${ANDROID_ABI} STREQUAL "x86_64")
		message("-- Android Build for x86_64")
	elseif(${ANDROID_ABI} STREQUAL "x86")
		message("-- Android Build for x86")
	elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
		message("-- Android Build for armeabi-v7a")
	elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
		message("-- Android Build for arm64-v8a")
	else()
		message(FATAL_ERROR "-- Android undefined build")
	endif()
elseif(APPLE)
   message("Apple Build")
else()
	message(FATAL_ERROR "-- Undefined build")
endif()

message("Project Name : ${GUI_PROJECT_NAME}")
message("Building Testing : ${GUI_BUILD_TESTING}")

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
    set(IMGUI_DOCKING_BRANCH ON CACHE BOOL "")
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

if(NOT APPLE)
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
         set(GLAD_WEBSITE "https://gen.glad.sh")
         execute_process(COMMAND ${CURL} -s -D - -X POST -d "generator=c&api=egl%3Dnone&api=gl%3D${GLAD_GL_VERSION}&profile=gl%3D${GLAD_GL_PROFILE}&api=gles1%3Dnone&profile=gles1%3Dcommon&api=gles2%3Dnone&api=glsc2%3Dnone&api=glx%3Dnone&api=vulkan%3Dnone&api=wgl%3Dnone&extensions=GL_ARB_gl_spirv%2CGL_ARB_spirv_extensions%2CVK_KHR_spirv_1_4%2CGL_AMD_debug_output%2CGL_ARB_debug_output%2CGL_EXT_debug_label%2CGL_EXT_debug_marker%2CGL_KHR_debug&options=LOADER" ${GLAD_WEBSITE}/generate OUTPUT_VARIABLE res)
         string(REGEX MATCH "Location: ([A-Za-z0-9_\\:/\\.]+)" location "${res}")
         set(location "${GLAD_WEBSITE}${CMAKE_MATCH_1}")
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
endif() #if APPLE


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


if(IOS)
    file(GLOB imgui_SRC ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)
    add_library(
            imgui
            ${imgui_SRC}
            ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
            ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.h
    )
    target_link_libraries(imgui PUBLIC "-framework Foundation")
    target_link_libraries(imgui PUBLIC "-framework UIKit")
    target_link_libraries(imgui PUBLIC "-framework GameController")
    target_link_libraries(imgui PUBLIC "-framework QuartzCore")
    target_link_libraries(imgui PUBLIC "-framework Metal")
    target_link_libraries(imgui PUBLIC "-framework MetalKit")
    target_include_directories(
            imgui
            PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )
elseif(APPLE)
    file(GLOB imgui_SRC ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)
    add_library(
            imgui
            ${imgui_SRC}
            ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
            ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.h
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
    )
    target_link_libraries(imgui PUBLIC "-framework Foundation")
    target_link_libraries(imgui PUBLIC "-framework UIKit")
    target_link_libraries(imgui PUBLIC "-framework QuartzCore")
    target_link_libraries(imgui PUBLIC "-framework Metal")
    target_link_libraries(imgui PUBLIC "-framework MetalKit")
    target_link_libraries(imgui PUBLIC glfw)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_IMPL_OPENGL_LOADER_GLAD2")
    target_include_directories(
            imgui
            PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )
else()
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
endif()

SET(SPIRV_SKIP_EXECUTABLES 1)
SET(ENABLE_GLSLANG_BINARIES 0)
add_subdirectory(thirdparty)

set(PLATFORM_OPENGL "Graphics/Platform/OpenGL/OpenGLBuffer.h"
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
"Graphics/Platform/OpenGL/OpenGLVertexArray.cpp")

set(PLATFORM_METAL
"Graphics/Platform/Metal/MetalBuffer.h"
"Graphics/Platform/Metal/MetalBuffer.cpp"
"Graphics/Platform/Metal/MetalContext.h"
"Graphics/Platform/Metal/MetalContext.mm"
"Graphics/Platform/Metal/MetalFrameBuffer.h"
"Graphics/Platform/Metal/MetalFrameBuffer.cpp"
"Graphics/Platform/Metal/MetalRendererAPI.h"
"Graphics/Platform/Metal/MetalRendererAPI.cpp"
"Graphics/Platform/Metal/MetalShader.h"
"Graphics/Platform/Metal/MetalShader.cpp"
"Graphics/Platform/Metal/MetalComputeShader.h"
"Graphics/Platform/Metal/MetalComputeShader.cpp"
"Graphics/Platform/Metal/MetalTexture.h"
"Graphics/Platform/Metal/MetalTexture.cpp"
"Graphics/Platform/Metal/MetalUniformBuffer.h"
"Graphics/Platform/Metal/MetalUniformBuffer.cpp"
"Graphics/Platform/Metal/MetalVertexArray.h"
"Graphics/Platform/Metal/MetalVertexArray.cpp"
)

set(GRAPHICS_SRC "Graphics/GraphicsCore.h"
"Graphics/GraphicsBase.h"
"Graphics/Renderer/BatchRenderer.h"
"Graphics/Renderer/BatchRenderer.cpp"
"Graphics/Renderer/Buffer.h"
"Graphics/Renderer/Buffer.cpp"
"Graphics/Renderer/Camera.h"
"Graphics/Renderer/2DCamera.h"
"Graphics/Renderer/2DCamera.cpp"
"Graphics/Renderer/3DCamera.h"
"Graphics/Renderer/3DCamera.cpp"
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
"Graphics/Renderer/ComputeShader.h"
"Graphics/Renderer/ComputeShader.cpp"
"Graphics/Renderer/Texture.h"
"Graphics/Renderer/Texture.cpp"
"Graphics/Renderer/UniformBuffer.h"
"Graphics/Renderer/UniformBuffer.cpp"
"Graphics/Renderer/VertexArray.h"
"Graphics/Renderer/VertexArray.cpp"
"Graphics/stb_image.h"
"Graphics/stb_image_impl.cpp"
)

if(APPLE)
list(APPEND GRAPHICS_SRC ${PLATFORM_METAL})
else()
list(APPEND GRAPHICS_SRC ${PLATFORM_OPENGL})
endif()

set(PLATFORM_WINDOW_SRC
#"Application/Window/Platform/WindowsWindow.cpp"
#"Application/Window/Platform/WindowsWindow.h"
"Application/Window/Platform/MacOSWindow.mm"
"Application/Window/Platform/MacOSWindow.h"
"Application/Window/Platform/IOSWindow.mm"
"Application/Window/Platform/IOSWindow.h"
"Application/Window/Window.cpp"
"Application/Window/Window.h"
"Application/Window/Platform/InputManager/InputManager.cpp"
"Application/Window/Platform/InputManager/InputManager.h"
)

set(APPLICATION_SRC 
"Application/AbstractApplication.h"
"Application/AbstractApplication.cpp"
"Application/Core/Base.h"
"Application/Core/Layer.h"
"Application/Core/LayerStack.h"
"Application/Core/LayerStack.cpp"
"Application/Events/Codes/KeyCodes.h"
"Application/Events/Codes/MouseCodes.h"
"Application/Events/EventTypes/ApplicationEvent.h"
"Application/Events/EventTypes/KeyEvent.h"
"Application/Events/EventTypes/MouseEvent.h"
"Application/Events/Input.cpp"
"Application/Events/Input.h"
"Application/Events/Event.h"
${PLATFORM_WINDOW_SRC}
)

set(IMGUI_HANDLER_SRC
"Application/ImGuiHandler/ImGuiHandler.h"
)

set(UTILS_SRC
"Utils/DataTypes.h"
)

if(APPLE)
list(APPEND IMGUI_HANDLER_SRC
"Application/ImGuiHandler/ImGuiHandlerMetal.mm"
)
list(APPEND UTILS_SRC
"Utils/IOS/FileUtils.h"
"Utils/IOS/FileUtils.mm"
)
else()
list(APPEND IMGUI_HANDLER_SRC
"Application/ImGuiHandler/ImGuiHandlerOpenGL.cpp"
)
endif()

set(SRC
    ${IMGUI_HANDLER_SRC}
    ${GRAPHICS_SRC}
    ${APPLICATION_SRC}
    ${UTILS_SRC}
)

FetchContent_Declare(LoggingLibrary                   #name of repository
 GIT_REPOSITORY https://github.com/cubeItDone/LoggingLibrary.git
 GIT_TAG        master)

FetchContent_MakeAvailable(LoggingLibrary)

include_directories(Graphics)
include_directories(Application)
include_directories(Utils)

# Add source to this project's executable.
add_executable (${PROJECT_NAME} ${SRC})
if(GUI_BUILD_TESTING)
    if(APPLE)
        target_sources(${PROJECT_NAME} PRIVATE "MainIOS.mm" "Starter.cpp" "Starter.h" "MetalTestLayer.mm" "MetalTestLayer.h")
    else()
        target_sources(${PROJECT_NAME} PRIVATE "MainWindows.cpp" "Starter.cpp" "Starter.h")
    endif()
endif()
if(APPLE)

SET(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
message(STATUS "Resources path for APPLE ${RESOURCES_PATH}")

resource(RESOURCE_FILES ${RESOURCES_PATH} "Resources/Resource" *)

target_sources(${PROJECT_NAME} PRIVATE ${RESOURCE_FILES})

set_xcode_property(${PROJECT_NAME} PRODUCT_BUNDLE_IDENTIFIER "com.abstract.testgui" All)
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    MACOSX_BUNDLE_LONG_VERSION_STRING "1.0"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    MACOSX_BUNDLE_GUI_IDENTIFIER "\$\{PRODUCT_BUNDLE_IDENTIFIER\}"
)
target_link_libraries(${PROJECT_NAME} "-framework Foundation")
target_link_libraries(${PROJECT_NAME} "-framework QuartzCore")
target_link_libraries(${PROJECT_NAME} "-framework Metal")
target_link_libraries(${PROJECT_NAME} "-framework MetalKit")
target_include_directories(${PROJECT_NAME} PRIVATE
        "Graphics/lib"
        )
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

target_link_libraries(${PROJECT_NAME} spirv-cross-msl)

codesign(${PROJECT_NAME})

else()
file(COPY Resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} glad)
target_link_libraries(${PROJECT_NAME} glfw)
endif()
target_link_libraries(${PROJECT_NAME} spirv-cross-core spirv-cross-glsl spirv-cross-cpp)
target_link_libraries(${PROJECT_NAME} shaderc)
target_link_libraries(${PROJECT_NAME} glm)
target_link_libraries(${PROJECT_NAME} imgui)
target_link_libraries(${PROJECT_NAME} LoggingLibrary)


if(IMGUI_DOCKING_BRANCH)
    target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_DOCKING_BRANCH_ENABLED)
endif()

message(STATUS "${PROJECT_NAME} uses STB_IMAGE_IMPLEMENTATION : ${USE_STB_IMAGE_IMPLEMENTATION}")
if(USE_STB_IMAGE_IMPLEMENTATION)
    message("You can turn this off by setting USE_STB_IMAGE_IMPLEMENTATION to OFF")
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_STB_IMAGE_IMPLEMENTATION)
else()
    message("You can turn this on by setting USE_STB_IMAGE_IMPLEMENTATION to ON")
endif()

if(APPLE)
add_executable(AppleMetalTest "AppleMetalTest.mm" "AppleMetalTest.h")
target_link_libraries(AppleMetalTest imgui)
endif()

if(IOS)
add_executable(AppleMetalTest-IOS "AppleMetalTest.mm" "AppleMetalTest.h")

SET(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
message(STATUS "Resources path for APPLE ${RESOURCES_PATH}")

resource(RESOURCE_FILES ${RESOURCES_PATH} "Resources/Resource" *)

target_sources(AppleMetalTest-IOS PRIVATE ${RESOURCE_FILES})

set_xcode_property(AppleMetalTest-IOS PRODUCT_BUNDLE_IDENTIFIER "com.abstract.applemetaltest" All)
set_target_properties(AppleMetalTest-IOS PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "AppleMetalTest-IOS"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    MACOSX_BUNDLE_LONG_VERSION_STRING "1.0"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    MACOSX_BUNDLE_GUI_IDENTIFIER "\$\{PRODUCT_BUNDLE_IDENTIFIER\}"
)
target_link_libraries(AppleMetalTest-IOS imgui)
codesign(AppleMetalTest-IOS)
endif()

message("Final CXX_STANDARD ${CMAKE_CXX_STANDARD}")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed. 