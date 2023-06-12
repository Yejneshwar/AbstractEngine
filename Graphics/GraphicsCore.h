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
	
		// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
		// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define GRAPHICS_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { GRAPHICS##type##ERROR(msg, __VA_ARGS__); GRAPHICS_DEBUGBREAK(); } }
	#define GRAPHICS_INTERNAL_ASSERT_WITH_MSG(type, check, ...) GRAPHICS_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define GRAPHICS_INTERNAL_ASSERT_NO_MSG(type, check) GRAPHICS_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", GRAPHICS_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)
	
	#define GRAPHICS_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define GRAPHICS_INTERNAL_ASSERT_GET_MACRO(...) GRAPHICS_EXPAND_MACRO( GRAPHICS_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, GRAPHICS_INTERNAL_ASSERT_WITH_MSG, GRAPHICS_INTERNAL_ASSERT_NO_MSG) )
	
	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define GRAPHICS_ASSERT(...) GRAPHICS_EXPAND_MACRO( GRAPHICS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define GRAPHICS_CORE_ASSERT(...) GRAPHICS_EXPAND_MACRO( GRAPHICS_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
	#else
	#define GRAPHICS_ASSERT(...)
	#define GRAPHICS_CORE_ASSERT(...)
	#endif

}