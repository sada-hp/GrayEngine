#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>

namespace GrEngine_Vulkan
{
	struct Vertex
	{
		bool operator==(const Vertex& other) const
		{
			return pos == other.pos;
		}

		glm::vec4 pos;
		glm::vec4 color;
	};

	struct Mesh
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
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct AllocatedImage {
		VkImage allocatedImage;
		VmaAllocation allocation;
	};
}

namespace std {
	template<> struct hash<GrEngine_Vulkan::Vertex> {
		size_t operator()(GrEngine_Vulkan::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos)) >> 1);
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

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}

		void initObject(VkDevice device, void* owner);
		void destroyObject(VkDevice device);
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
