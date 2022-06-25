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

		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

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
		uint8_t material_index = 0;
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

		void initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner);
		void destroyObject(VkDevice device, VmaAllocator allocator);
		void updateObject(VkDevice device);
		void invalidateTexture(VkDevice device, VmaAllocator allocator);
		bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent);
		bool recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent);

	private:
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
