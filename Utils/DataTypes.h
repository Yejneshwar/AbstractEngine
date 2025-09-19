#include <string>
#include <simd/simd.h>

inline simd_float4x4 operator*(const simd_float4x4& lhs, const simd_float4x4& rhs) {
    return simd_mul(lhs, rhs);
}

inline simd_float3 operator*(const simd_float4x4& lhs, const simd_float4& rhs) {
    // Multiply the matrix by the 4D vector.
    simd_float4 result = simd_mul(lhs, rhs);

    // Perform the perspective divide (divide xyz by w).
    // This is crucial for correctness when using a perspective projection matrix.
    // For affine transformations (like translate, rotate, scale), result.w will be 1, so this does nothing.
    return result.xyz / result.w;
}

namespace GUI {


#ifdef __APPLE__


// Forward declarations
template<typename VectorType, int N> struct SIMDVec;
using SIMDVec2 = SIMDVec<simd_float2, 2>;
using SIMDVec3 = SIMDVec<simd_float3, 3>;
using SIMDVec4 = SIMDVec<simd_float4, 4>;

/**
 * @class SIMDVec
 * @brief A generic C++ wrapper for SIMD vector types (simd_floatN).
 *
 * This template class provides a unified C++ interface for SIMD vector types,
 * enabling modern features like initializer lists and operator overloading
 * without code duplication.
 *
 * @tparam VectorType The underlying SIMD vector type (e.g., simd_float3).
 * @tparam N The number of floating-point elements in the vector (e.g., 3).
 */
template<typename VectorType, int N>
struct SIMDVec {
    VectorType data;

    // Default constructor (initializes to zero)
    SIMDVec() : data{} {}

    // Constructor from an existing underlying SIMD vector
    SIMDVec(const VectorType& v) : data(v) {}
    
//    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
//    SIMDVec(Args... args) : data{static_cast<float>(args)...} {}

    // Constructor from an initializer list
    // New variadic constructor to replace the initializer_list one.
     // This constructor enables brace-initialization like {x, y, z} and uses
     // SFINAE to ensure it only participates in overload resolution when the
     // number of arguments is correct, thus avoiding ambiguity.
     template<typename... Args,
              typename = std::enable_if_t<
                  sizeof...(Args) == N &&
                  (std::is_convertible_v<Args, float> && ...)
              >>
     SIMDVec(Args... args) : data{static_cast<float>(args)...} {}
    
    // -- Conversion Constructors --

    // Combined constructor for SIMDVec3(from SIMDVec2) and SIMDVec4(from SIMDVec3).
    // Uses `if constexpr` (C++17) to select the correct implementation and avoid the
    // ambiguity of having two templates with the same function parameter signature.
//    template <typename V>
//    SIMDVec(const V& v, float val) {
//        if constexpr (N == 3 && std::is_same_v<std::decay_t<V>, SIMDVec2>) {
//            // Constructing SIMDVec3 from SIMDVec2 and a float
//            data.x = v.x();
//            data.y = v.y();
//            data.z = val;
//        } else if constexpr (N == 4 && std::is_same_v<std::decay_t<V>, SIMDVec3>) {
//            // Constructing SIMDVec4 from SIMDVec3 and a float
//            data.x = v.x();
//            data.y = v.y();
//            data.z = v.z();
//            data.w = val;
//        } else {
//            // This static_assert will fail compilation for any other combination,
//            // preventing this constructor from being used incorrectly. It is dependent
//            // on `V` to ensure it's only evaluated when the template is instantiated.
//            static_assert(!std::is_same_v<V, V>, "Invalid arguments for widening vector constructor(const V&, float)");
//        }
//    }

    // Constructor for SIMDVec4 from SIMDVec2 and two floats.
    // Enabled only when constructing a SIMDVec4 from a SIMDVec2.
    template <typename V, int D = N,
        typename = std::enable_if_t<D == 4 && std::is_same_v<std::decay_t<V>, SIMDVec2>>>
    SIMDVec(const V& v, float z_val, float w_val) {
        data.x = v.x();
        data.y = v.y();
        data.z = z_val;
        data.w = w_val;
    }
    
    // -- Narrowing Conversion Constructors (Conditionally Enabled) --
    
    // Constructor for SIMDVec2 from SIMDVec3, enabled only when N=2.
    // Made explicit to prevent unintended implicit narrowing conversions.
    template<int D = N, typename std::enable_if<D == 2, int>::type = 0>
    SIMDVec(const SIMDVec3& v) {
        data.x = v.x();
        data.y = v.y();
    }
    
    // Constructor for SIMDVec3 from SIMDVec4, enabled only when N=3.
    // Made explicit to prevent unintended implicit narrowing conversions.
    template<int D = N, typename std::enable_if<D == 3, int>::type = 0>
    SIMDVec(const SIMDVec4& v) {
        data.x = v.x();
        data.y = v.y();
        data.z = v.z();
    }

    // Implicit conversion operator to allow passing this class to functions expecting the raw SIMD type.
    operator VectorType() const {
        return data;
    }

    // Compound assignment operators
    SIMDVec& operator+=(const SIMDVec& rhs) { data += rhs.data; return *this; }
    SIMDVec& operator-=(const SIMDVec& rhs) { data -= rhs.data; return *this; }
    SIMDVec& operator*=(float scalar) { data *= scalar; return *this; }
    SIMDVec& operator/=(float scalar) { data /= scalar; return *this; }
    
    // Component Accessors
    float& x() { static_assert(N >= 1, "Vector does not have an 'x' component."); return reinterpret_cast<float*>(&data)[0]; }
    const float& x() const { static_assert(N >= 1, "Vector does not have an 'x' component."); return reinterpret_cast<const float*>(&data)[0]; }

    float& y() { static_assert(N >= 2, "Vector does not have a 'y' component."); return reinterpret_cast<float*>(&data)[1]; }
    const float& y() const { static_assert(N >= 2, "Vector does not have a 'y' component."); return reinterpret_cast<const float*>(&data)[1]; }

    float& z() { static_assert(N >= 3, "Vector does not have a 'z' component."); return reinterpret_cast<float*>(&data)[2]; }
    const float& z() const { static_assert(N >= 3, "Vector does not have a 'z' component."); return reinterpret_cast<const float*>(&data)[2]; }

    float& w() { static_assert(N >= 4, "Vector does not have a 'w' component."); return reinterpret_cast<float*>(&data)[3]; }
    const float& w() const { static_assert(N >= 4, "Vector does not have a 'w' component."); return reinterpret_cast<const float*>(&data)[3]; }
};

// Non-member binary operators
template<typename VT, int N> inline SIMDVec<VT, N> operator+(const SIMDVec<VT, N>& lhs, const SIMDVec<VT, N>& rhs) { return SIMDVec<VT, N>(lhs.data + rhs.data); }
template<typename VT, int N> inline SIMDVec<VT, N> operator-(const SIMDVec<VT, N>& lhs, const SIMDVec<VT, N>& rhs) { return SIMDVec<VT, N>(lhs.data - rhs.data); }
template<typename VT, int N> inline SIMDVec<VT, N> operator*(const SIMDVec<VT, N>& lhs, float scalar) { return SIMDVec<VT, N>(lhs.data * scalar); }
template<typename VT, int N> inline SIMDVec<VT, N> operator*(float scalar, const SIMDVec<VT, N>& rhs) { return SIMDVec<VT, N>(scalar * rhs.data); }
template<typename VT, int N> inline SIMDVec<VT, N> operator/(const SIMDVec<VT, N>& lhs, float scalar) { return SIMDVec<VT, N>(lhs.data / scalar); }

// Generic utility functions
template<typename VT, int N> inline float length(const SIMDVec<VT, N>& v) { return simd_length(v.data); }
template<typename VT, int N> inline SIMDVec<VT, N> normalize(const SIMDVec<VT, N>& v) { return SIMDVec<VT, N>(simd_normalize(v.data)); }
template<typename VT, int N> inline float dot(const SIMDVec<VT, N>& lhs, const SIMDVec<VT, N>& rhs) { return simd_dot(lhs.data, rhs.data); }


namespace DataType {
    typedef SIMDVec4 vec4;
    typedef SIMDVec3 vec3;
    typedef SIMDVec2 vec2;
    typedef simd_float1 float1;
    typedef simd_int1 int1;
    
    typedef simd_float4x4 mat4;

    const mat4 Identity = matrix_identity_float4x4;
        
}
namespace DataOp {
    /**
     * @brief Helper function to mimic glm::length's behavior.
     */
    template <typename T>
    inline DataType::float1 length(T const& l) {
        return simd_length(l);
    }

    template <typename T>
    inline T normalize(T const& n) {
        return simd_normalize(n);
    }

    /**
     * @brief Creates an alias for simd_inverse to match GLM's naming.
     * This is an inline function that forwards the call.
     */
    inline simd_float4x4 inverse(simd_float4x4 const& m) {
        return simd_inverse(m);
    }

    /**
     * @brief Helper function to mimic glm::rotate's behavior.
     */
    inline simd_float4x4 rotate(simd_float4x4 const& m, float angle, simd_float3 const& axis) {
        simd_float4x4 rotationMatrix = simd_matrix4x4(simd_quaternion(angle, axis));
        return simd_mul(m, rotationMatrix);
    }

    /**
    * @brief Rotates a 2D vector by a given angle.
    *
    * This function mimics the behavior of glm::rotate(glm::vec2, float).
    * It constructs a 2x2 rotation matrix and multiplies it with the vector.
    *
    * @param v The 2D vector to rotate.
    * @param angle The angle of rotation in radians.
    * @return The rotated 2D vector.
    */
    inline simd_float2 rotate(const simd_float2& v, float angle) {
        // Calculate sine and cosine of the angle
        const float cos_angle = cosf(angle);
        const float sin_angle = sinf(angle);
        
        // Create a 2x2 rotation matrix (column-major)
        // | cos(a)  -sin(a) |
        // | sin(a)   cos(a) |
        simd_float2x2 rotationMatrix = simd_float2x2({
            { cos_angle, sin_angle },   // First column
            { -sin_angle, cos_angle }  // Second column
        });
        
        // Multiply the matrix by the vector to get the rotated vector
        return simd_mul(rotationMatrix, v);
    }

    /**
    * @brief Helper function to mimic glm::translate's behavior.
    */
    inline simd_float4x4 translate(simd_float4x4 const& m, simd_float3 const& v) {
        simd_float4x4 translationMatrix = matrix_identity_float4x4;
        translationMatrix.columns[3] = simd_make_float4(v, 1.0f);
        return simd_mul(m, translationMatrix);
    }
    
    /**
    * @brief Helper function to mimic glm::scale's behavior.
    */
    inline simd_float4x4 scale(simd_float4x4 const& m, simd_float3 const& v) {
        simd_float4x4 scaleMatrix = matrix_from_diagonal(simd_make_float4(v, 1.0f));
        return simd_mul(m, scaleMatrix);
    }
}
#else
namespace DataType {
    typedef glm::vec4 vec4;
    typedef glm::vec3 vec3;
    typedef glm::vec2 vec2;
    typedef float float1;
    typedef int int1;
    
    typedef glm::mat4 mat4;

    const mat4 Identity = glm::mat4(1.0f);
        
}

namespace DataOp {
    using glm::rotate;
    using glm::inverse;
    using glm::translate;
    using glm::scale;
    using glm::length;
    using glm::normalize;
}
#endif
}

