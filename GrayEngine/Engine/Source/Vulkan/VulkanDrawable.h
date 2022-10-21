#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vk_mem_alloc.h>
#include "Engine/Source/Headers/Renderer.h"

#define TEXTURE_ARRAY_SIZE 5

namespace GrEngine_Vulkan
{
	struct Vertex : public GrEngine::Vertex
	{
		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, uv);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[3].offset = offsetof(Vertex, uv_index);

			attributeDescriptions[4].binding = 0;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[4].offset = offsetof(Vertex, uses_texture);

			return attributeDescriptions;
		}
	};

	struct Mesh : public GrEngine::Mesh
	{
		std::vector<Vertex> vertices = {
			{{{ 0.25, 0.25, 0.25, 1.0f },{ 0.f, 1.f, 0.f, 1.f },{ 1.f, 1.0f }}},
			{{{ -0.25, 0.25, -0.25, 1.0f },{ 0.f, 1.f, 0.f, 1.f },{ 0.f, 0.0f }}},
			{{{ -0.25, 0.25 , 0.25, 1.0f },{0.f, 1.f, 0.f, 1.f},{ 0.f, 1.f }}},
			{{{ 0.25, 0.25, -0.25, 1.0f },{ 0.f, 1.f, 0.f, 1.f },{ 1.f, 0.0f }}},

			{{{ 0.25, -0.25, 0.25, 1.0f },{ 0.f, 1.f, 0.f, 1.f },{ 1.f, 1.0f }}},
			{{{ -0.25, -0.25, -0.25, 1.0f },{ 0.f, 1.f, 0.f, 1.f },{ 0.f, 0.0f }}},
			{{{ -0.25, -0.25 , 0.25, 1.0f },{0.f, 1.f, 0.f, 1.f},{ 0.f, 1.f }}},
			{{{ 0.25, -0.25, -0.25, 1.0f },{ 0.f, 1.f, 0.f, 1.f },{ 1.f, 0.0f }}}
		};;
		std::vector<uint16_t> indices = { 0, 1, 2, 0, 3, 1,
			4, 5, 6, 4, 7, 5,
			0, 2, 6, 0, 4, 6,
			0, 3, 4, 3, 7, 4,
			1, 3, 7, 1, 5, 7,
			1, 2, 6, 1, 5, 6,
		};
	};

	struct ShaderBuffer
	{
		VkBuffer Buffer;
		VkDescriptorBufferInfo BufferInfo;
		VkMemoryRequirements MemoryRequirements;
		VkMappedMemoryRange MappedMemoryRange;
		uint8_t* pData;
		VmaAllocation Allocation;

		bool initialized = false;
	};

	struct UniformBufferObject {
		glm::mat4 model{ 1.f };
		glm::mat4 view{1.f};
		glm::mat4 proj{ 1.f };
	};

	struct AllocatedImage {
		VkImage allocatedImage;
		VmaAllocation allocation;
	};

	struct Texture : public GrEngine::Texture
	{
		AllocatedImage newImage;
		VkImageView textureImageView;
		VkSampler textureSampler;
		int8_t material_index = -1;
	};
}

	template<> struct std::hash<GrEngine_Vulkan::Vertex> {
		size_t operator()(GrEngine_Vulkan::Vertex const& vertex) const {
			return ((std::hash<glm::vec4>()(vertex.pos)) ^ (std::hash<glm::vec2>()(vertex.uv)) >> 1);
		}
	};

namespace GrEngine_Vulkan
{

	class VulkanDrawable : public GrEngine::DrawableObject
	{
	public:
		Mesh object_mesh;
		std::vector<Texture> object_texture;
		const char* shader_path = "Shaders//default";
		float near_plane = 0.1;
		float far_plane = 1000;

		void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner);
		void destroyObject(VkDevice device, VmaAllocator allocator);
		void updateObject(VkDevice device, VmaAllocator allocator);
		void invalidateTexture(VkDevice device, VmaAllocator allocator);
		bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent);
		bool recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent);

	protected:
		GrEngine::Renderer* p_Owner;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkDescriptorSet> descriptorSets;

		VkDescriptorPool descriptorPool;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		ShaderBuffer vertexBuffer;
		ShaderBuffer indexBuffer;

		UniformBufferObject ubo{};

		bool createDescriptorLayout(VkDevice device);
		bool createDescriptorPool(VkDevice device);
		bool createPipelineLayout(VkDevice device);
		bool createDescriptorSet(VkDevice device);
		bool createGraphicsPipeline(VkDevice device);
	};
}
