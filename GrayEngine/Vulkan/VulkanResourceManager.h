#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vk_mem_alloc.h>
#include "Engine/Headers/Entities/DrawableObject.h"

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

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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

			return attributeDescriptions;
		}
	};

	struct ShaderBuffer
	{
		VkBuffer Buffer;
		VkDescriptorBufferInfo BufferInfo;
		VkMemoryRequirements MemoryRequirements;
		VkMappedMemoryRange MappedMemoryRange;
		VmaAllocation Allocation;

		bool initialized = false;
	};

	struct DescriptorSet
	{
		std::vector<VkShaderStageFlags> stage;
		std::vector<VkDescriptorType> type;
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<uint16_t> bindings;
		VkDescriptorSet set;
		VkPipelineBindPoint bindPoint;
		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
	};

	struct Mesh : public GrEngine::Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		ShaderBuffer vertexBuffer;
		ShaderBuffer indexBuffer;
		btTriangleMesh collisions;

		glm::uvec3 bounds;
	};

	struct UniformBufferObject {
		glm::mat4 model{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 proj{ 1.f };
		glm::vec3 scale = { 1.f, 1.f, 1.f };
	};

	struct PickingBufferObject {
		uint32_t draw_mode = 0;
		glm::vec4 colors{ 1.f };
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
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t channels = 1;
		uint32_t mipLevels = 0;
	};


	template<typename T>
	struct Resource
	{
		Resource(const char* new_name, T new_pointer)
		{
			name = new_name;
			pointer = new_pointer;
		}

		~Resource()
		{

		}

		T AddLink()
		{
			links++;
			return pointer;
		}

		void RemoveLink()
		{
			links--;
		}

		T PopResource()
		{
			links--;
			return pointer;
		}

		void Update(T new_pointer)
		{
			pointer = new_pointer;
		}

		uint8_t getNumOfLinks()
		{
			return links;
		}

		std::string name;
	private:
		uint8_t links = 0;
		T pointer;
	};

	class VulkanResourceManager
	{
	public:
		VulkanResourceManager()
		{

		}

		~VulkanResourceManager()
		{
			meshResources.clear();
			texResources.clear();
		};

		void Clean(VkDevice device, VmaAllocator allocator);
		void RemoveMesh(const char* name, VkDevice device, VmaAllocator allocator);
		void RemoveTexture(const char* name, VkDevice device, VmaAllocator allocator);
		void RemoveTexture(std::vector<std::string> names, VkDevice device, VmaAllocator allocator);
		void UpdateTexture(std::vector<std::string> names, VkDevice device, VmaAllocator allocator, Texture* newValue);

		Resource<Mesh*>* AddMeshResource(const char* name, Mesh* pointer)
		{
			std::string string_name = NormalizeName(name);
			meshResources.push_back(new Resource<Mesh*>(string_name.c_str(), pointer));
			return meshResources.back();
		}

		Resource<Texture*>* AddTextureResource(const char* name, Texture* pointer)
		{
			std::string string_name = NormalizeName(name);
			texResources.push_back(new Resource<Texture*>(string_name.c_str(), pointer));
			return texResources.back();
		}

		Resource<Texture*>* AddTextureResource(std::vector<std::string> names, Texture* pointer)
		{
			std::string output = "";
			for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
			{
				output += NormalizeName((*itt));
			}

			texResources.push_back(new Resource<Texture*>(output.c_str(), pointer));
			return texResources.back();
		}

		Resource<Mesh*>* GetMeshResource(const char* name)
		{
			std::string string_name = NormalizeName(name);

			for (std::vector<Resource<Mesh*>*>::iterator itt = meshResources.begin(); itt != meshResources.end(); ++itt)
			{
				if ((*itt)->name == string_name)
				{
					return (*itt);
				}
			}

			return nullptr;
		}

		Resource<Texture*>* GetTextureResource(const char* name)
		{
			std::string string_name = NormalizeName(name);

			for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
			{
				if ((*itt)->name == string_name)
				{
					return (*itt);
				}
			}

			return nullptr;
		}

		Resource<Texture*>* GetTextureResource(std::vector<std::string> names)
		{
			std::string output = "";
			for (std::vector<std::string>::iterator itt = names.begin(); itt != names.end(); ++itt)
			{
				output += NormalizeName((*itt));
			}

			for (std::vector<Resource<Texture*>*>::iterator itt = texResources.begin(); itt != texResources.end(); ++itt)
			{
				if ((*itt)->name == output)
				{
					return (*itt);
				}
			}

			return nullptr;
		}

	private:
		std::string NormalizeName(std::string name)
		{
			std::string path = GrEngine::Globals::getExecutablePath();

			if (name.starts_with(path))
			{
				return name.substr(path.size(), name.size() - path.size());
			}
			else
			{
				return name;
			}
		}

		std::vector<Resource<Mesh*>*> meshResources;
		std::vector<Resource<Texture*>*> texResources;
	};
}

template<> struct std::hash<GrEngine_Vulkan::Vertex> {
	size_t operator()(GrEngine_Vulkan::Vertex const& vertex) const {
		return ((std::hash<glm::vec4>()(vertex.pos)) ^ (std::hash<glm::vec2>()(vertex.uv)) >> 1);
	}
};
