#pragma once
#include <string>

#define BIT(x) (1 << x)

#define APP_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


#define HZ_ASSERT(...)
#define HZ_CORE_ASSERT(...)


#define HZ_PROFILE_FUNCTION(...)
#define HZ_PROFILE_SCOPE(...)

#define HZ_CORE_ERROR(...)
#define HZ_CORE_INFO(...)
