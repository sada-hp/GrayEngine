#pragma once
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine/Source/Libs/VkMemAlloc/vk_mem_alloc.h"

class DrawableObj
{
	struct Vertex {
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

	void initObject(VkDevice device);
	void destroyObject(VkDevice device);
	void updateUniformBuffer(VkDevice device, uint32_t imageIndex);
	bool pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent);
	bool recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent);
private:
	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;

	VkDescriptorPool descriptorPool;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	ShaderBuffer vertexBuffer;
	ShaderBuffer indexBuffer;
	ShaderBuffer uniformBuffer;

	UniformBufferObject ubo{};

	bool createDescriptorLayout(VkDevice device);
	bool createDescriptorPool(VkDevice device);
	bool createPipelineLayout(VkDevice device);
	bool createDescriptorSet(VkDevice device);
	bool createGraphicsPipeline(VkDevice device);

	bool createVkBuffer(VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shader);
	void destroyShaderBuffer(VkDevice device, ShaderBuffer shader);
};

