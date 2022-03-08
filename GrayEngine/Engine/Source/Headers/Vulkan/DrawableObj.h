#pragma once

class DrawableObj
{
	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;
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
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	void initObject(VkDevice device);
	void destroyObject();
	std::vector<VkDescriptorSetLayout> setLayout;
	VkPipelineLayout pipelineLayout;

private:
	VkDevice logicalDevice;
	VkDescriptorPool descriptorPool;

	bool createDescriptorLayout(VkDevice device);
	bool createDescriptorPool(VkDevice device);
	bool createPipelineLayout(VkDevice device);
};

