#pragma once
#include <memory>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#define GRAPHICS_ENABLE_ASSERTS 1

namespace Graphics {
	
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
	
	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	#ifdef GRAPHICS_ENABLE_ASSERTS
    // =========================
    // Utility Macros
    // =========================
    
    // Expands the macro passed to it (forces full expansion)
    #define GRAPHICS_EXPAND_MACRO(x) x
    
    // Converts a macro argument to a string literal
    #define GRAPHICS_STRINGIFY_MACRO(x) GRAPHICS_STRINGIFY_MACRO_IMPL(x)
    #define GRAPHICS_STRINGIFY_MACRO_IMPL(x) #x
    
    // Platform-independent debug break (Windows/MSVC or fallback)
    #if defined(_MSC_VER)
        #define GRAPHICS_DEBUGBREAK() __debugbreak()
    #elif defined(__clang__) || defined(__GNUC__)
        #define GRAPHICS_DEBUGBREAK() __builtin_trap()
    #else
        #include <cstdlib>
        #define GRAPHICS_DEBUGBREAK() std::abort()
    #endif
    
    // =========================
    // Logging Stubs
    // Replace these with your actual logging system
    // =========================
    
    #define GRAPHICS_ERROR(msg, ...) \
        std::cerr << "[GRAPHICS ERROR] " << std::format(msg, __VA_ARGS__) << std::endl;
    
    #define GRAPHICS_CORE_ERROR(msg, ...) \
        std::cerr << "[GRAPHICS CORE ERROR] " << std::format(msg, __VA_ARGS__) << std::endl;
    
    // =========================
    // Internal Assertion System
    // =========================
    
    // Core implementation for assertions
    #define GRAPHICS_INTERNAL_ASSERT_IMPL(type, check, msg, ...) \
        { if (!(check)) { GRAPHICS##type##ERROR(msg, __VA_ARGS__); GRAPHICS_DEBUGBREAK(); } }
    
    // Assertion with a user-specified message
    #define GRAPHICS_INTERNAL_ASSERT_WITH_MSG(type, check, ...) \
        GRAPHICS_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
    
    // Assertion with no message; generates automatic context
    #define GRAPHICS_INTERNAL_ASSERT_NO_MSG(type, check) \
        GRAPHICS_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", \
            GRAPHICS_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)
    
    // Select the correct macro based on number of arguments
    #define GRAPHICS_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
    #define GRAPHICS_INTERNAL_ASSERT_GET_MACRO(...) \
        GRAPHICS_EXPAND_MACRO(GRAPHICS_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, \
            GRAPHICS_INTERNAL_ASSERT_WITH_MSG, GRAPHICS_INTERNAL_ASSERT_NO_MSG))
    
    // =========================
    // Public Assertion Macros
    // =========================
    
    // Use this in application code
    #define GRAPHICS_ASSERT(...) \
        GRAPHICS_EXPAND_MACRO(GRAPHICS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__))
    
    // Use this inside the engine/core code
    #define GRAPHICS_CORE_ASSERT(...) \
        GRAPHICS_EXPAND_MACRO(GRAPHICS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__))
	#else
	#define GRAPHICS_ASSERT(...)
	#define GRAPHICS_CORE_ASSERT(...)
	#endif
}
