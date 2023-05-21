#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vk_mem_alloc.h>
#include "Engine/Headers/Virtual/ResourceManager.h"
#include "Engine/Headers/Entities/Properties/Drawable.h"
#include "Bullet/BulletAPI.h"

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

		static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, norm);

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
			attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[4].offset = offsetof(Vertex, tang);

			attributeDescriptions[5].binding = 0;
			attributeDescriptions[5].location = 5;
			attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[5].offset = offsetof(Vertex, bitang);

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
		void* data;

		bool initialized = false;
	};

	struct DescriptorSet
	{
		std::vector<VkShaderStageFlags> stage;
		std::vector<VkDescriptorType> type;
		std::vector<std::vector<VkDescriptorImageInfo>> imageInfos;
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
		glm::vec3 bounds;
	};

	struct CollisionMesh
	{
		btCollisionShape* shape;
	};

	struct VertexConstants {
		glm::mat4 model{ 1.f };
		glm::vec4 scale = { 1.f, 1.f, 1.f, 1.f };
	};

	struct PickingBufferObject {
		glm::vec4 colors{ 1.f };
	};

	struct TextureInfo
	{
		uint32_t mipLevels = 0;
		VkFormat format;
		VkDescriptorImageInfo descriptor;
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
		TextureInfo texInfo;
		std::vector<std::string> texture_collection;
	};




	class VulkanResourceManager : public GrEngine::ResourceManager
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
		void RemoveMesh(Mesh* resource, VkDevice device, VmaAllocator allocator);
		void RemoveTexture(Texture* resource, VkDevice device, VmaAllocator allocator);
		void UpdateTexture(std::vector<std::string> names, VkDevice device, VmaAllocator allocator, Texture* newValue);

		Resource<Mesh*>* AddMeshResource(const char* name, Mesh* pointer);

		Resource<Texture*>* AddTextureResource(const char* name, Texture* pointer);

		Resource<Texture*>* AddTextureResource(std::vector<std::string> names, Texture* pointer);

		Resource<Texture*>* AddTextureResource(std::vector<std::string> names, Texture* pointer, GrEngine::TextureType type);

		Resource<Mesh*>* GetMeshResource(const char* name);

		Resource<Texture*>* GetTextureResource(const char* name);

		Resource<Texture*>* GetTextureResource(std::vector<std::string> names);

		Resource<Texture*>* GetTextureResource(std::vector<std::string> names, GrEngine::TextureType type);

		static void CalculateNormals(GrEngine_Vulkan::Mesh* target, bool clockwise = false);
		static void CalculateTangents(GrEngine_Vulkan::Mesh* target, float u_scale = 1.f, float v_scale = 1.f, bool clockwise = false);
		inline size_t CountTextures() { return texResources.size(); };

	private:
		std::vector<Resource<Mesh*>*> meshResources;
		std::vector<Resource<Texture*>*> texResources;
	};
}

template<> struct std::hash<GrEngine_Vulkan::Vertex> {
	size_t operator()(GrEngine_Vulkan::Vertex const& vertex) const {
		return ((std::hash<glm::vec4>()(vertex.pos)) ^ (std::hash<glm::vec3>()(vertex.norm)) ^ (std::hash<glm::vec2>()(vertex.uv)) >> 1);
	}
};
