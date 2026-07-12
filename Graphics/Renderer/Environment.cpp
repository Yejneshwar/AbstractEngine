#include "Environment.h"

#include <Logger.h>

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace Graphics {

namespace {

	constexpr uint32_t kEnvWidth = 128;
	constexpr uint32_t kEnvHeight = 64;
	constexpr uint32_t kEnvMips = 6; // 128x64 ... 4x2
	constexpr uint32_t kPrefilterSamples = 64;
	constexpr uint32_t kMatcapSize = 256;

	struct EnvironmentData {
		bool initialized = false;
		Ref<Texture2D> specular;
		Ref<Texture2D> matcap;
		glm::vec4 shIrradiance[9] = {};
	};

	EnvironmentData& Data()
	{
		static EnvironmentData s_Data;
		return s_Data;
	}

	// Texel center -> world direction. Row v=0 is the zenith (+Y); u wraps
	// around the horizon (matches dirToEquirect in LightingDeclarations.h).
	glm::vec3 EquirectDirection(float u, float v)
	{
		const float theta = v * glm::pi<float>();          // 0 = +Y
		const float phi = (u - 0.5f) * glm::two_pi<float>();
		const float sinTheta = std::sin(theta);
		return { sinTheta * std::cos(phi), std::cos(theta), sinTheta * std::sin(phi) };
	}

	// Smooth rectangular "softbox" area light in (azimuth, elevation) space.
	float Softbox(const glm::vec3& d, float azimuthDeg, float elevationDeg, float halfWidthDeg, float halfHeightDeg, float softnessDeg)
	{
		const float azimuth = glm::degrees(std::atan2(d.z, d.x));
		const float elevation = glm::degrees(std::asin(glm::clamp(d.y, -1.0f, 1.0f)));

		float dAz = azimuth - azimuthDeg;
		while (dAz > 180.0f) dAz -= 360.0f;
		while (dAz < -180.0f) dAz += 360.0f;
		const float dEl = elevation - elevationDeg;

		const float wx = 1.0f - glm::smoothstep(halfWidthDeg - softnessDeg, halfWidthDeg + softnessDeg, std::abs(dAz));
		const float wy = 1.0f - glm::smoothstep(halfHeightDeg - softnessDeg, halfHeightDeg + softnessDeg, std::abs(dEl));
		return wx * wy;
	}

	// The procedural studio: soft vertical gradient, dark ground with a hint
	// of bounce, and three softboxes (key / fill / rim) for shaped speculars.
	glm::vec3 SampleStudio(const glm::vec3& d)
	{
		glm::vec3 color;
		if (d.y >= 0.0f) {
			const glm::vec3 zenith = { 0.42f, 0.47f, 0.56f };
			const glm::vec3 horizon = { 0.85f, 0.83f, 0.80f };
			color = glm::mix(horizon, zenith, std::pow(glm::clamp(d.y, 0.0f, 1.0f), 0.65f));
		} else {
			const glm::vec3 ground = { 0.22f, 0.21f, 0.20f };
			const glm::vec3 nadir = { 0.10f, 0.10f, 0.10f };
			color = glm::mix(ground, nadir, std::pow(glm::clamp(-d.y, 0.0f, 1.0f), 0.8f));
		}

		// Key: large warm box, high and to the front-right.
		color += glm::vec3(1.0f, 0.96f, 0.90f) * (9.0f * Softbox(d, 35.0f, 45.0f, 30.0f, 18.0f, 6.0f));
		// Fill: broad cool box, lower on the opposite side.
		color += glm::vec3(0.75f, 0.82f, 1.0f) * (3.5f * Softbox(d, 205.0f, 25.0f, 40.0f, 22.0f, 8.0f));
		// Rim: narrow bright strip behind/above.
		color += glm::vec3(1.0f, 1.0f, 1.0f) * (12.0f * Softbox(d, 310.0f, 62.0f, 14.0f, 7.0f, 4.0f));

		return color;
	}

	// Bilinear sample of a CPU equirect level, wrapping U, clamping V.
	glm::vec3 SampleBilinear(const std::vector<glm::vec3>& img, uint32_t width, uint32_t height, float u, float v)
	{
		u = u - std::floor(u);
		v = glm::clamp(v, 0.0f, 1.0f);
		const float x = u * (float)width - 0.5f;
		const float y = v * (float)height - 0.5f;
		const int x0 = (int)std::floor(x);
		const int y0 = (int)std::floor(y);
		const float fx = x - (float)x0;
		const float fy = y - (float)y0;

		auto texel = [&](int xi, int yi) -> const glm::vec3& {
			xi = ((xi % (int)width) + (int)width) % (int)width;
			yi = glm::clamp(yi, 0, (int)height - 1);
			return img[(size_t)yi * width + xi];
		};

		return glm::mix(glm::mix(texel(x0, y0), texel(x0 + 1, y0), fx),
		                glm::mix(texel(x0, y0 + 1), texel(x0 + 1, y0 + 1), fx), fy);
	}

	float RadicalInverseVdC(uint32_t bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return (float)bits * 2.3283064365386963e-10f;
	}

	// GGX importance sample around +Z in tangent space.
	glm::vec3 ImportanceSampleGGX(float e1, float e2, float roughness)
	{
		const float a = roughness * roughness;
		const float phi = glm::two_pi<float>() * e1;
		const float cosTheta = std::sqrt((1.0f - e2) / (1.0f + (a * a - 1.0f) * e2));
		const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
		return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
	}

	// Split-sum prefilter (N = V = R) of one output level.
	std::vector<glm::vec3> PrefilterLevel(const std::vector<glm::vec3>& base, uint32_t width, uint32_t height, float roughness)
	{
		std::vector<glm::vec3> out((size_t)width * height);
		for (uint32_t y = 0; y < height; ++y) {
			for (uint32_t x = 0; x < width; ++x) {
				const glm::vec3 N = EquirectDirection(((float)x + 0.5f) / (float)width,
				                                      ((float)y + 0.5f) / (float)height);
				const glm::vec3 up = std::abs(N.y) < 0.999f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
				const glm::vec3 tangent = glm::normalize(glm::cross(up, N));
				const glm::vec3 bitangent = glm::cross(N, tangent);

				glm::vec3 sum(0.0f);
				float weight = 0.0f;
				for (uint32_t s = 0; s < kPrefilterSamples; ++s) {
					const glm::vec3 h = ImportanceSampleGGX(((float)s + 0.5f) / (float)kPrefilterSamples,
					                                        RadicalInverseVdC(s), roughness);
					const glm::vec3 H = tangent * h.x + bitangent * h.y + N * h.z;
					const glm::vec3 L = 2.0f * glm::dot(N, H) * H - N;
					const float NoL = glm::dot(N, L);
					if (NoL <= 0.0f)
						continue;
					const float u = std::atan2(L.z, L.x) / glm::two_pi<float>() + 0.5f;
					const float v = std::acos(glm::clamp(L.y, -1.0f, 1.0f)) / glm::pi<float>();
					sum += SampleBilinear(base, kEnvWidth, kEnvHeight, u, v) * NoL;
					weight += NoL;
				}
				out[(size_t)y * width + x] = weight > 0.0f ? sum / weight : glm::vec3(0.0f);
			}
		}
		return out;
	}

	void ProjectIrradianceSH(const std::vector<glm::vec3>& env, glm::vec4 outSH[9])
	{
		glm::vec3 L[9] = {};
		for (uint32_t y = 0; y < kEnvHeight; ++y) {
			const float v = ((float)y + 0.5f) / (float)kEnvHeight;
			const float theta = v * glm::pi<float>();
			// Solid angle of one texel row element on the equirect sphere.
			const float dOmega = (glm::two_pi<float>() / kEnvWidth) * (glm::pi<float>() / kEnvHeight) * std::sin(theta);
			for (uint32_t x = 0; x < kEnvWidth; ++x) {
				const glm::vec3 d = EquirectDirection(((float)x + 0.5f) / (float)kEnvWidth, v);
				const glm::vec3 c = env[(size_t)y * kEnvWidth + x] * dOmega;

				L[0] += c * 0.282095f;
				L[1] += c * (0.488603f * d.y);
				L[2] += c * (0.488603f * d.z);
				L[3] += c * (0.488603f * d.x);
				L[4] += c * (1.092548f * d.x * d.y);
				L[5] += c * (1.092548f * d.y * d.z);
				L[6] += c * (0.315392f * (3.0f * d.z * d.z - 1.0f));
				L[7] += c * (1.092548f * d.x * d.z);
				L[8] += c * (0.546274f * (d.x * d.x - d.y * d.y));
			}
		}

		// Pack so the shader polynomial (1, y, z, x, xy, yz, 3z^2-1, xz,
		// x^2-y^2) evaluates E(n)/pi directly (Ramamoorthi & Hanrahan; the
		// c3*z^2 - c5 pair folds into c5*(3z^2-1) since c3 == 3*c5).
		const float c1 = 0.429043f, c2 = 0.511664f, c4 = 0.886227f, c5 = 0.247708f;
		const float invPi = 1.0f / glm::pi<float>();
		auto pack = [&](int i, const glm::vec3& value) { outSH[i] = glm::vec4(value * invPi, 0.0f); };

		pack(0, c4 * L[0]);
		pack(1, 2.0f * c2 * L[1]);
		pack(2, 2.0f * c2 * L[2]);
		pack(3, 2.0f * c2 * L[3]);
		pack(4, 2.0f * c1 * L[4]);
		pack(5, 2.0f * c1 * L[5]);
		pack(6, c5 * L[6]);
		pack(7, 2.0f * c1 * L[7]);
		pack(8, c1 * L[8]);
	}

	void UploadEquirect(const Ref<Texture2D>& texture, const std::vector<glm::vec3>& level, uint32_t width, uint32_t height, uint32_t mip)
	{
		std::vector<float> rgba((size_t)width * height * 4);
		for (size_t i = 0; i < level.size(); ++i) {
			rgba[i * 4 + 0] = level[i].x;
			rgba[i * 4 + 1] = level[i].y;
			rgba[i * 4 + 2] = level[i].z;
			rgba[i * 4 + 3] = 1.0f;
		}
		texture->SetMipData(rgba.data(), (uint32_t)(rgba.size() * sizeof(float)), mip);
	}

	Ref<Texture2D> GenerateMatcap()
	{
		std::vector<uint8_t> pixels((size_t)kMatcapSize * kMatcapSize * 4);

		const glm::vec3 keyDir = glm::normalize(glm::vec3(0.45f, 0.55f, 0.70f));
		const glm::vec3 fillDir = glm::normalize(glm::vec3(-0.60f, 0.15f, 0.78f));
		const glm::vec3 keyHalf = glm::normalize(keyDir + glm::vec3(0, 0, 1));
		const glm::vec3 fillHalf = glm::normalize(fillDir + glm::vec3(0, 0, 1));

		for (uint32_t y = 0; y < kMatcapSize; ++y) {
			for (uint32_t x = 0; x < kMatcapSize; ++x) {
				glm::vec2 p = { ((float)x + 0.5f) / kMatcapSize * 2.0f - 1.0f,
				                ((float)y + 0.5f) / kMatcapSize * 2.0f - 1.0f };
				const float r2 = glm::dot(p, p);
				if (r2 > 1.0f)
					p /= std::sqrt(r2);
				const glm::vec3 n = { p.x, p.y, std::sqrt(std::max(0.0f, 1.0f - glm::dot(p, p))) };

				float c = 0.10f;                                                     // ambient floor
				c += 0.85f * std::pow(std::max(glm::dot(n, keyDir), 0.0f), 1.3f);    // key diffuse
				c += 0.22f * std::max(glm::dot(n, fillDir), 0.0f);                   // fill diffuse
				c += 0.30f * std::pow(1.0f - std::max(n.z, 0.0f), 3.0f);             // rim
				float spec = 0.55f * std::pow(std::max(glm::dot(n, keyHalf), 0.0f), 48.0f)
				           + 0.15f * std::pow(std::max(glm::dot(n, fillHalf), 0.0f), 24.0f);

				const float value = glm::clamp(c, 0.0f, 1.4f);
				glm::vec3 color = glm::vec3(value) + glm::vec3(spec);
				// Store display-referred (the shader linearizes on sample).
				color = glm::pow(glm::clamp(color, 0.0f, 1.0f), glm::vec3(1.0f / 2.2f));

				const size_t idx = ((size_t)y * kMatcapSize + x) * 4;
				pixels[idx + 0] = (uint8_t)std::round(color.x * 255.0f);
				pixels[idx + 1] = (uint8_t)std::round(color.y * 255.0f);
				pixels[idx + 2] = (uint8_t)std::round(color.z * 255.0f);
				pixels[idx + 3] = 255;
			}
		}

		Ref<Texture2D> texture = Texture2D::Create(kMatcapSize, kMatcapSize, TextureFormat::RGBA8);
		texture->SetData(pixels.data(), (uint32_t)pixels.size());
		return texture;
	}

} // namespace

	void EnvironmentIBL::Init()
	{
		EnvironmentData& data = Data();
		if (data.initialized)
			return;

		// Base level.
		std::vector<glm::vec3> base((size_t)kEnvWidth * kEnvHeight);
		for (uint32_t y = 0; y < kEnvHeight; ++y)
			for (uint32_t x = 0; x < kEnvWidth; ++x)
				base[(size_t)y * kEnvWidth + x] = SampleStudio(
					EquirectDirection(((float)x + 0.5f) / kEnvWidth, ((float)y + 0.5f) / kEnvHeight));

		ProjectIrradianceSH(base, data.shIrradiance);

		data.specular = Texture2D::Create(kEnvWidth, kEnvHeight, TextureFormat::RGBA32FLOAT, kEnvMips);
		UploadEquirect(data.specular, base, kEnvWidth, kEnvHeight, 0);
		for (uint32_t mip = 1; mip < kEnvMips; ++mip) {
			const uint32_t width = std::max(1u, kEnvWidth >> mip);
			const uint32_t height = std::max(1u, kEnvHeight >> mip);
			const float roughness = (float)mip / (float)(kEnvMips - 1);
			UploadEquirect(data.specular, PrefilterLevel(base, width, height, roughness), width, height, mip);
		}

		data.matcap = GenerateMatcap();
		data.initialized = true;
		LOG_INFO_STREAM << "EnvironmentIBL: generated " << kEnvWidth << "x" << kEnvHeight
		                << " studio environment (" << kEnvMips << " prefiltered mips)";
	}

	bool EnvironmentIBL::IsInitialized() { return Data().initialized; }
	const Ref<Texture2D>& EnvironmentIBL::GetSpecularMap() { return Data().specular; }
	uint32_t EnvironmentIBL::GetSpecularMipCount() { return kEnvMips; }
	const glm::vec4* EnvironmentIBL::GetIrradianceSH() { return Data().shIrradiance; }
	const Ref<Texture2D>& EnvironmentIBL::GetMatcap() { return Data().matcap; }

}
