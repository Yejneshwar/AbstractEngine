#pragma once

// GPU scene for the ray-traced pipeline (Metal backend): builds a BLAS over
// the retained-mesh arena, a one-instance TLAS (SPIRV-Cross translates GLSL
// ray queries against acceleration_structure<instancing>), and the flat
// attribute/material/emissive buffers the RT compute shaders fetch from.
//
// Everything is lazily (re)built from BatchRenderer::GetRetainedSceneView()
// version counters — cheap to call every frame.

#include <cstdint>

namespace Graphics {

	class MetalRayTracing {
	public:
		// Hardware ray tracing available on the current device (Apple6+/M1+).
		static bool Supported();

		// Ensure GPU buffers + acceleration structures match the current
		// retained scene. Returns false when there is nothing to trace (no
		// retained meshes) or ray tracing is unsupported.
		static bool EnsureScene();

		// Native handles for ComputeShader::BindBuffer /
		// BindAccelerationStructure. Only valid after EnsureScene() == true.
		static uintptr_t GetTLAS();
		static uintptr_t GetPositionsBuffer(); // float[3] per vertex, tight
		static uintptr_t GetAttribsBuffer();   // {vec4 normal+matIdx, vec4 color} per vertex
		static uintptr_t GetIndexBuffer();     // uint32 per index
		static uintptr_t GetMaterialsBuffer(); // GpuMaterial per entry
		static uintptr_t GetEmissiveBuffer();  // {uint triIndex, float area, vec2 pad} per emissive tri
		static uint32_t GetEmissiveTriangleCount();

		// Mark indirectly-referenced resources (the BLAS behind the TLAS)
		// resident on the currently bound compute encoder. Call between the
		// compute shader's Bind() and Dispatch().
		static void MakeResident();

		static void Shutdown();
	};

}
