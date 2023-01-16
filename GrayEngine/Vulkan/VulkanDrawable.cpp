#include <pch.h>
#include "VulkanDrawable.h"
#include "VulkanAPI.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanDrawable::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		p_Owner = owner;
		resources = &static_cast<VulkanAPI*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh->mesh_path.c_str(), logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		auto resource = resources->GetMeshResource("default");
		if (resource == nullptr)
		{
			Mesh* default_mesh = new Mesh();

			default_mesh->vertices = {
				{{{ 0.25, 0.25, 0.25, 1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, 0.25, -0.25, 1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, 0.25 , 0.25, 1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, 0.25, -0.25, 1.0f },{ 1.f, 0.0f }}},

				{{{ 0.25, -0.25, 0.25, 1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, -0.25, -0.25, 1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, -0.25 , 0.25, 1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, -0.25, -0.25, 1.0f },{ 1.f, 0.0f }}}
			};

			default_mesh->indices = { 0, 1, 2, 0, 3, 1,
				4, 5, 6, 4, 7, 5,
				0, 2, 6, 0, 4, 6,
				0, 3, 4, 3, 7, 4,
				1, 3, 7, 1, 5, 7,
				1, 2, 6, 1, 5, 6,
			};

			resource = resources->AddMeshResource("default", default_mesh);
			object_mesh = resource->AddLink();

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->vertices.data(), sizeof(object_mesh->vertices[0]) * object_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &object_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->indices.data(), sizeof(object_mesh->indices[0]) * object_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &object_mesh->indexBuffer);
		}
		else
		{
			object_mesh = resource->AddLink();
		}

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
		createPipelineLayout();
		createGraphicsPipeline();
	}

	void VulkanDrawable::destroyObject()
	{
		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh->mesh_path.c_str(), logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		if (object_texture != nullptr)
		{
			resources->RemoveTexture(object_texture->texture_collection, logicalDevice, memAllocator);
			object_texture = nullptr;
		}

		vkDestroyPipeline(logicalDevice, graphicsPipeline, NULL);
		vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);
		vkFreeDescriptorSets(logicalDevice, descriptorPool, descriptorSets.size(), descriptorSets.data());
		vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);
		vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, NULL);


		//VulkanAPI::m_destroyTexture(logicalDevice, memAllocator, object_texture);

		this->~VulkanDrawable();
	}

	void VulkanDrawable::updateObject()
	{
		vkFreeDescriptorSets(logicalDevice, descriptorPool, descriptorSets.size(), descriptorSets.data());
		vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);
		vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, NULL);
		vkDestroyPipeline(logicalDevice, graphicsPipeline, NULL);
		vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
		createPipelineLayout();
		createGraphicsPipeline();
	}

	void VulkanDrawable::invalidateTexture()
	{
		//VulkanAPI::m_destroyTexture(logicalDevice, memAllocator, object_texture);
	}

	bool VulkanDrawable::createPipelineLayout()
	{
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(UniformBufferObject);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPushConstantRange pushConstant2;
		pushConstant2.offset = sizeof(UniformBufferObject);
		pushConstant2.size = sizeof(PickingBufferObject);
		pushConstant2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		std::vector<VkPushConstantRange> ranges = {pushConstant, pushConstant2};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 2;
		pipelineLayoutInfo.pPushConstantRanges = ranges.data();

		return vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout) == VK_SUCCESS;
	}

	bool VulkanDrawable::recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh->vertexBuffer.initialized == true)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);

			//UpdateObjectPosition();
			//UpdateObjectOrientation();
			pushConstants(commandBuffer, extent, mode);

			for (int ind = 0; ind < descriptorSets.size(); ind++)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[ind], 0, NULL);
			}

			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object_mesh->indices.size()), 1, 0, 0, 0);
			return true;
		}

		return false;
	}

	bool VulkanDrawable::pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);
		return true;
	}

	bool VulkanDrawable::createGraphicsPipeline()
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();

		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");

		VkShaderModule shaders[2] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode) , VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[1];
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = dynamic_cast<VulkanAPI*>(p_Owner)->GetSampling();
		multisampling.minSampleShading = 1.0f;
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.minSampleShading = .35f;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 1.0f; // Optional
		colorBlending.blendConstants[1] = 1.0f; // Optional
		colorBlending.blendConstants[2] = 1.0f; // Optional
		colorBlending.blendConstants[3] = 1.0f; // Optional

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicCreateInfo{};
		dynamicCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicCreateInfo.pNext = nullptr;
		dynamicCreateInfo.flags = 0;
		dynamicCreateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicCreateInfo.pDynamicStates = dynamicStates.data();

		VkPipelineDepthStencilStateCreateInfo _depthStencil{};
		_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		_depthStencil.pNext = nullptr;
		_depthStencil.depthTestEnable = VK_TRUE;
		_depthStencil.depthWriteEnable = VK_TRUE;
		_depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		_depthStencil.depthBoundsTestEnable = VK_FALSE;
		_depthStencil.minDepthBounds = 0.0f; // Optional
		_depthStencil.maxDepthBounds = 1.0f; // Optional
		_depthStencil.stencilTestEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicCreateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = reinterpret_cast<VulkanAPI*>(p_Owner)->getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.pDepthStencilState = &_depthStencil;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
			return false;

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);

		return true;
	}

	bool VulkanDrawable::createDescriptorLayout()
	{
		VkDescriptorSetLayoutBinding descriptorBindings{};
		descriptorBindings.binding = 1; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorBindings.descriptorCount = object_texture != nullptr;
		descriptorBindings.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptorBindings.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutCreateInfo descriptorLayout{};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = NULL;
		descriptorLayout.bindingCount = 1;
		descriptorLayout.pBindings = &descriptorBindings;

		return vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, NULL, &descriptorSetLayout) == VK_SUCCESS;
	}

	bool VulkanDrawable::createDescriptorPool()
	{
		VkDescriptorPoolSize descriptorTypePool;
		descriptorTypePool.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorTypePool.descriptorCount = object_texture != nullptr;
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &descriptorTypePool;
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;


		return vkCreateDescriptorPool(logicalDevice, &createInfo, NULL, &descriptorPool) == VK_SUCCESS;
	}

	bool VulkanDrawable::createDescriptorSet()
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.pNext = NULL;
		descriptorAllocInfo.descriptorPool = descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;

		std::vector<VkDescriptorImageInfo> imageInfo;
		if (object_texture != nullptr)
		{
			VkDescriptorImageInfo info;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info.imageView = object_texture->textureImageView;
			info.sampler = object_texture->textureSampler;
			imageInfo.push_back(info);
		}

		descriptorSets.resize(1);
		vkAllocateDescriptorSets(logicalDevice, &descriptorAllocInfo, descriptorSets.data());

		VkWriteDescriptorSet writes{};
		memset(&writes, 0, sizeof(writes));

		writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes.dstSet = descriptorSets[0];
		writes.dstBinding = 1;
		writes.dstArrayElement = 0;
		writes.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes.descriptorCount = imageInfo.size();
		writes.pImageInfo = imageInfo.data();

		if (imageInfo.size() == 0) 
			return true;

		vkUpdateDescriptorSets(logicalDevice, 1, &writes, 0, NULL);

		return true;
	}
};