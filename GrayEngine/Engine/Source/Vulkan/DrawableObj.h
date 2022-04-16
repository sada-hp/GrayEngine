#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>

#define TEXTURE_ARRAY_SIZE 5

namespace GrEngine_Vulkan
{
	struct Vertex
	{
		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}

		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;

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

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		const char* mesh_path;
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
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct AllocatedImage {
		VkImage allocatedImage;
		VmaAllocation allocation;
	};

	struct Texture
	{
		AllocatedImage newImage;
		VkImageView textureImageView;
		VkSampler textureSampler;

		const char* texture_path;
	};
}

namespace std {
	template<> struct hash<GrEngine_Vulkan::Vertex> {
		size_t operator()(GrEngine_Vulkan::Vertex const& vertex) const {
			return ((hash<glm::vec4>()(vertex.pos)) ^ (hash<glm::vec2>()(vertex.uv)) >> 1);
		}
	};
}

namespace GrEngine_Vulkan
{

	class DrawableObj
	{
	public:
		DrawableObj();
		~DrawableObj();

		Mesh object_mesh;
		std::vector<Texture> object_texture;
		glm::vec3 bound;

		void initObject(VkDevice device, VmaAllocator allocator, void* owner);
		void destroyObject(VkDevice device, VmaAllocator allocator);
		void updateObject(VkDevice device);
		void invalidateTexture(VkDevice device, VmaAllocator allocator);
		bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent);
		bool recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent);

	private:
		void* p_Owner;
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
