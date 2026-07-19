#include "MetalRayTracing.h"

#include "MetalContext.h"
#include "Renderer/BatchRenderer.h"
#include "Renderer/Lighting.h"

#include <Logger.h>

#include <cstring>
#include <vector>

namespace Graphics {

namespace {

	struct SceneState {
		MTL::Buffer* positions = nullptr;   // float3, tight (also the BLAS vertex input)
		MTL::Buffer* attribs = nullptr;     // {float4 normal.xyz + matIdx bits, float4 color}
		MTL::Buffer* indices = nullptr;     // uint32
		MTL::Buffer* materials = nullptr;   // GpuMaterial table copy
		MTL::Buffer* emissive = nullptr;    // {uint triIdx, float area, float2 pad}
		MTL::Buffer* instanceDescs = nullptr;
		MTL::AccelerationStructure* blas = nullptr;
		MTL::AccelerationStructure* tlas = nullptr;
		uint32_t emissiveCount = 0;
		uint64_t geometryVersion = 0;
		uint64_t materialVersion = 0;
	};
	static SceneState s_Scene;

	void ReleaseBuffer(MTL::Buffer*& buffer)
	{
		if (buffer) { buffer->release(); buffer = nullptr; }
	}

	MTL::Buffer* MakeBuffer(MTL::Device* device, const void* data, size_t size, const char* label)
	{
		MTL::Buffer* buffer = device->newBuffer(size, MTL::ResourceStorageModeShared);
		std::memcpy(buffer->contents(), data, size);
		buffer->setLabel(NS::String::string(label, NS::UTF8StringEncoding));
		return buffer;
	}

	// Build (synchronously) an acceleration structure from a descriptor.
	// Scene edits are rare; a blocking build keeps the lifetime rules simple.
	MTL::AccelerationStructure* BuildAccelerationStructure(MTL::AccelerationStructureDescriptor* descriptor)
	{
		MTL::Device* device = MetalContext::GetCurrentDevice();
		const MTL::AccelerationStructureSizes sizes = device->accelerationStructureSizes(descriptor);
		MTL::AccelerationStructure* structure = device->newAccelerationStructure(sizes.accelerationStructureSize);
		MTL::Buffer* scratch = device->newBuffer(sizes.buildScratchBufferSize, MTL::ResourceStorageModePrivate);

		MTL::CommandBuffer* commandBuffer = MetalContext::GetCurrentCommandQueue()->commandBuffer();
		MTL::AccelerationStructureCommandEncoder* encoder = commandBuffer->accelerationStructureCommandEncoder();
		encoder->buildAccelerationStructure(structure, descriptor, scratch, 0);
		encoder->endEncoding();
		commandBuffer->commit();
		commandBuffer->waitUntilCompleted();

		scratch->release();
		return structure;
	}

	// Per-vertex attributes as the RT shaders read them (std430, 32 bytes).
	struct GpuVertexAttrib {
		float normal[3];
		float materialIndexBits; // int bits in a float slot
		float color[4];
	};

	struct GpuEmissiveTri {
		uint32_t triIndex;
		float area;
		// Closed emitters are single-sided: +1/-1 = the triangles' cross-
		// product normals point outward/inward (decided by signed volume);
		// 0 = open surface, emit from both sides. Halves the light samples
		// wasted on the far side of closed emitters (they can never pass the
		// visibility test and only add noise).
		float sideSign;
		float pad;
	};

	void RebuildGeometry(const BatchRenderer::RetainedSceneView& view)
	{
		MTL::Device* device = MetalContext::GetCurrentDevice();

		// In-flight frames may still reference the previous buffers/AS.
		MetalContext::WaitForGpuIdle();
		ReleaseBuffer(s_Scene.positions);
		ReleaseBuffer(s_Scene.attribs);
		ReleaseBuffer(s_Scene.indices);
		ReleaseBuffer(s_Scene.instanceDescs);
		if (s_Scene.blas) { s_Scene.blas->release(); s_Scene.blas = nullptr; }
		if (s_Scene.tlas) { s_Scene.tlas->release(); s_Scene.tlas = nullptr; }

		// Deinterleave the arena into tight buffers (positions double as the
		// BLAS vertex input; attributes are fetched by primitive index).
		const char* vertexBase = (const char*)view.vertexData;
		std::vector<float> positions(view.vertexCount * 3);
		std::vector<GpuVertexAttrib> attribs(view.vertexCount);
		for (size_t i = 0; i < view.vertexCount; i++) {
			const char* vertex = vertexBase + i * view.vertexStride;
			std::memcpy(&positions[i * 3], vertex + view.positionOffset, sizeof(float) * 3);
			GpuVertexAttrib& attrib = attribs[i];
			std::memcpy(attrib.normal, vertex + view.normalOffset, sizeof(float) * 3);
			int materialIndex = 0;
			std::memcpy(&materialIndex, vertex + view.materialOffset, sizeof(int));
			std::memcpy(&attrib.materialIndexBits, &materialIndex, sizeof(int));
			std::memcpy(attrib.color, vertex + view.colorOffset, sizeof(float) * 4);
		}

		s_Scene.positions = MakeBuffer(device, positions.data(), positions.size() * sizeof(float), "RT Positions");
		s_Scene.attribs = MakeBuffer(device, attribs.data(), attribs.size() * sizeof(GpuVertexAttrib), "RT Attribs");
		s_Scene.indices = MakeBuffer(device, view.indexData, view.indexCount * sizeof(uint32_t), "RT Indices");

		// BLAS over the whole retained arena as one opaque triangle geometry.
		MTL::AccelerationStructureTriangleGeometryDescriptor* geometry =
			MTL::AccelerationStructureTriangleGeometryDescriptor::alloc()->init();
		geometry->setVertexBuffer(s_Scene.positions);
		geometry->setVertexStride(sizeof(float) * 3);
		geometry->setIndexBuffer(s_Scene.indices);
		geometry->setIndexType(MTL::IndexTypeUInt32);
		geometry->setTriangleCount(view.indexCount / 3);
		geometry->setOpaque(true);

		MTL::PrimitiveAccelerationStructureDescriptor* blasDescriptor =
			MTL::PrimitiveAccelerationStructureDescriptor::alloc()->init();
		const NS::Object* geometries[] = { geometry };
		NS::Array* geometryArray = NS::Array::array(geometries, 1);
		blasDescriptor->setGeometryDescriptors(geometryArray);
		s_Scene.blas = BuildAccelerationStructure(blasDescriptor);
		blasDescriptor->release();
		geometry->release();

		// One-instance TLAS: GLSL ray queries compile to MSL
		// acceleration_structure<instancing>, so a bare BLAS can't be bound.
		MTL::AccelerationStructureInstanceDescriptor instance = {};
		instance.transformationMatrix = MTL::PackedFloat4x3(
			MTL::PackedFloat3(1.0f, 0.0f, 0.0f),
			MTL::PackedFloat3(0.0f, 1.0f, 0.0f),
			MTL::PackedFloat3(0.0f, 0.0f, 1.0f),
			MTL::PackedFloat3(0.0f, 0.0f, 0.0f));
		// CAD meshes are open surfaces — disable backface culling for rays.
		instance.options = MTL::AccelerationStructureInstanceOptionOpaque |
			MTL::AccelerationStructureInstanceOptionDisableTriangleCulling;
		instance.mask = 0xFF;
		instance.intersectionFunctionTableOffset = 0;
		instance.accelerationStructureIndex = 0;
		s_Scene.instanceDescs = MakeBuffer(device, &instance, sizeof(instance), "RT Instances");

		MTL::InstanceAccelerationStructureDescriptor* tlasDescriptor =
			MTL::InstanceAccelerationStructureDescriptor::alloc()->init();
		const NS::Object* blases[] = { s_Scene.blas };
		NS::Array* blasArray = NS::Array::array(blases, 1);
		tlasDescriptor->setInstancedAccelerationStructures(blasArray);
		tlasDescriptor->setInstanceCount(1);
		tlasDescriptor->setInstanceDescriptorBuffer(s_Scene.instanceDescs);
		s_Scene.tlas = BuildAccelerationStructure(tlasDescriptor);
		tlasDescriptor->release();

		LOG_INFO_STREAM << "MetalRayTracing: rebuilt acceleration structure over "
			<< view.indexCount / 3 << " triangles, " << view.vertexCount << " vertices";
	}

	void RebuildMaterialsAndEmissive(const BatchRenderer::RetainedSceneView& view)
	{
		MTL::Device* device = MetalContext::GetCurrentDevice();

		MetalContext::WaitForGpuIdle();
		ReleaseBuffer(s_Scene.materials);
		ReleaseBuffer(s_Scene.emissive);

		s_Scene.materials = MakeBuffer(device, view.materials,
			std::max<size_t>(1, view.materialCount) * sizeof(GpuMaterial), "RT Materials");

		// Emissive triangle list: light-source candidates for ReSTIR. The
		// material of a triangle is the material of its first vertex (meshes
		// are single-material, so any vertex works).
		const char* vertexBase = (const char*)view.vertexData;
		auto positionOf = [&](uint32_t index) {
			float p[3];
			std::memcpy(p, vertexBase + index * view.vertexStride + view.positionOffset, sizeof(p));
			return glm::vec3(p[0], p[1], p[2]);
		};

		// Meshes are contiguous in the arena, so consecutive emissive
		// triangles with one material form a "run" (one emitter). A run's
		// signed volume decides its orientation: |V| well above zero means a
		// closed mesh whose cross-product normals point outward (V > 0) or
		// inward (V < 0) — vertex winding varies per generator, so this is
		// measured, not assumed. Near-zero volume = open surface.
		std::vector<GpuEmissiveTri> emissiveTris;
		size_t runStart = 0;
		int runMaterial = -1;
		size_t runLastTri = SIZE_MAX;
		double runVolume = 0.0, runArea = 0.0;
		auto closeRun = [&]() {
			if (runStart >= emissiveTris.size())
				return;
			float sideSign = 0.0f;
			const double characteristic = std::pow(std::max(runArea, 0.0), 1.5);
			if (characteristic > 0.0 && std::abs(runVolume) > 0.02 * characteristic)
				sideSign = runVolume > 0.0 ? 1.0f : -1.0f;
			for (size_t i = runStart; i < emissiveTris.size(); i++)
				emissiveTris[i].sideSign = sideSign;
			runStart = emissiveTris.size();
			runVolume = 0.0;
			runArea = 0.0;
		};

		for (size_t tri = 0; tri * 3 + 2 < view.indexCount; tri++) {
			const uint32_t i0 = view.indexData[tri * 3];
			int materialIndex = 0;
			std::memcpy(&materialIndex, vertexBase + i0 * view.vertexStride + view.materialOffset, sizeof(int));
			if (materialIndex < 0 || (size_t)materialIndex >= view.materialCount)
				continue;
			const glm::vec4& emissive = view.materials[materialIndex].emissive;
			if (emissive.r <= 0.0f && emissive.g <= 0.0f && emissive.b <= 0.0f)
				continue;
			const glm::vec3 p0 = positionOf(view.indexData[tri * 3]);
			const glm::vec3 p1 = positionOf(view.indexData[tri * 3 + 1]);
			const glm::vec3 p2 = positionOf(view.indexData[tri * 3 + 2]);
			const float area = 0.5f * glm::length(glm::cross(p1 - p0, p2 - p0));

			if (materialIndex != runMaterial || tri != runLastTri + 1)
				closeRun();
			runMaterial = materialIndex;
			runLastTri = tri;
			// The divergence-theorem volume uses every triangle (even
			// degenerate pole slivers — they contribute zero).
			runVolume += glm::dot(p0, glm::cross(p1, p2)) / 6.0;
			runArea += area;

			if (area <= 1e-12f)
				continue;
			emissiveTris.push_back({ (uint32_t)tri, area, 0.0f, 0.0f });
		}
		closeRun();

		s_Scene.emissiveCount = (uint32_t)emissiveTris.size();
		if (emissiveTris.empty())
			emissiveTris.push_back({ 0, 0.0f, 0.0f, 0.0f }); // keep the binding valid
		s_Scene.emissive = MakeBuffer(device, emissiveTris.data(),
			emissiveTris.size() * sizeof(GpuEmissiveTri), "RT Emissive Tris");

		LOG_INFO_STREAM << "MetalRayTracing: " << s_Scene.emissiveCount << " emissive triangles";
	}

} // namespace

	bool MetalRayTracing::Supported()
	{
		static const bool supported = [] {
			MTL::Device* device = MetalContext::GetCurrentDevice();
			return device && device->supportsRaytracing();
		}();
		return supported;
	}

	bool MetalRayTracing::EnsureScene()
	{
		if (!Supported())
			return false;

		const BatchRenderer::RetainedSceneView view = BatchRenderer::GetRetainedSceneView();
		if (view.indexCount < 3 || view.vertexCount == 0)
			return false;

		const bool geometryChanged = view.geometryVersion != s_Scene.geometryVersion;
		const bool materialsChanged = view.materialVersion != s_Scene.materialVersion;
		if (geometryChanged)
			RebuildGeometry(view);
		if (geometryChanged || materialsChanged)
			RebuildMaterialsAndEmissive(view);
		s_Scene.geometryVersion = view.geometryVersion;
		s_Scene.materialVersion = view.materialVersion;

		return s_Scene.tlas != nullptr;
	}

	uintptr_t MetalRayTracing::GetTLAS() { return (uintptr_t)s_Scene.tlas; }
	uintptr_t MetalRayTracing::GetPositionsBuffer() { return (uintptr_t)s_Scene.positions; }
	uintptr_t MetalRayTracing::GetAttribsBuffer() { return (uintptr_t)s_Scene.attribs; }
	uintptr_t MetalRayTracing::GetIndexBuffer() { return (uintptr_t)s_Scene.indices; }
	uintptr_t MetalRayTracing::GetMaterialsBuffer() { return (uintptr_t)s_Scene.materials; }
	uintptr_t MetalRayTracing::GetEmissiveBuffer() { return (uintptr_t)s_Scene.emissive; }
	uint32_t MetalRayTracing::GetEmissiveTriangleCount() { return s_Scene.emissiveCount; }

	void MetalRayTracing::MakeResident()
	{
		MTL::ComputeCommandEncoder* encoder = MetalContext::GetCurrentComputeCommandEncoder();
		if (encoder && s_Scene.blas)
			encoder->useResource(s_Scene.blas, MTL::ResourceUsageRead);
	}

	void MetalRayTracing::Shutdown()
	{
		MetalContext::WaitForGpuIdle();
		ReleaseBuffer(s_Scene.positions);
		ReleaseBuffer(s_Scene.attribs);
		ReleaseBuffer(s_Scene.indices);
		ReleaseBuffer(s_Scene.materials);
		ReleaseBuffer(s_Scene.emissive);
		ReleaseBuffer(s_Scene.instanceDescs);
		if (s_Scene.blas) { s_Scene.blas->release(); s_Scene.blas = nullptr; }
		if (s_Scene.tlas) { s_Scene.tlas->release(); s_Scene.tlas = nullptr; }
		s_Scene.geometryVersion = 0;
		s_Scene.materialVersion = 0;
	}

}
