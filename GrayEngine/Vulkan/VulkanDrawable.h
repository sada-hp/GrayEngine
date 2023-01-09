#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vk_mem_alloc.h>
#include "Engine/Headers/Virtual/Renderer.h"

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
			attributeDescriptions[1].location = 2;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, uv);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 3;
			attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[2].offset = offsetof(Vertex, uv_index);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 4;
			attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[3].offset = offsetof(Vertex, uses_texture);

			attributeDescriptions[4].binding = 0;
			attributeDescriptions[4].location = 5;
			attributeDescriptions[4].format = VK_FORMAT_R32G32B32_UINT;
			attributeDescriptions[4].offset = offsetof(Vertex, inID);

			return attributeDescriptions;
		}
	};

	struct Mesh : public GrEngine::Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;
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
		glm::vec3 scale = { 1.f, 1.f, 1.f };
	};

	struct PickingBufferObject {
		uint32_t draw_mode = 0;
		glm::uvec3 selected_entity{ 0, 0, 0 };
		uint32_t highlight_enabled = 1;
		glm::vec4 color_mask;
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
	class VulkanDrawable
	{
	public:
		
		Texture object_texture;
		const char* shader_path = "Shaders//default";
		float near_plane = 0.1;
		float far_plane = 1000;

		virtual void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner);
		virtual void destroyObject(VkDevice device, VmaAllocator allocator);
		virtual void updateObject(VkDevice device, VmaAllocator allocator);
		virtual void invalidateTexture(VkDevice device, VmaAllocator allocator);
		virtual bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode);
		virtual bool recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode);

	protected:
		GrEngine::Renderer* p_Owner;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkDescriptorSet> descriptorSets;
		Mesh object_mesh;

		VkDescriptorPool descriptorPool;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;

		ShaderBuffer vertexBuffer;
		ShaderBuffer indexBuffer;

		UniformBufferObject ubo{};

		bool createDescriptorLayout(VkDevice device);
		bool createDescriptorPool(VkDevice device);
		bool createPipelineLayout(VkDevice device);
		bool createDescriptorSet(VkDevice device, VmaAllocator allocator);
		bool createGraphicsPipeline(VkDevice device);
	};
}
