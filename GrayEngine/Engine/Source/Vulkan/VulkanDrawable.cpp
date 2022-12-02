#include <pch.h>
#include "VulkanDrawable.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	void VulkanDrawable::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		p_Owner = owner;
		Type = "VulkanDrawable";

		if (object_mesh.vertices.size() > 0)
		{
			VulkanAPI::m_createVkBuffer(device, allocator, object_mesh.vertices.data(), sizeof(object_mesh.vertices[0]) * object_mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer);
			VulkanAPI::m_createVkBuffer(device, allocator, object_mesh.indices.data(), sizeof(object_mesh.indices[0]) * object_mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer);
		}

		createDescriptorLayout(device);
		createDescriptorPool(device);
		createDescriptorSet(device, allocator);
		createPipelineLayout(device);
		createGraphicsPipeline(device);
	}

	void VulkanDrawable::destroyObject(VkDevice device, VmaAllocator allocator)
	{
		vkDestroyPipeline(device, graphicsPipeline, NULL);
		vkDestroyPipelineLayout(device, pipelineLayout, NULL);
		vkFreeDescriptorSets(device, descriptorPool, descriptorSets.size(), descriptorSets.data());

		VulkanAPI::m_destroyShaderBuffer(device, allocator, &indexBuffer);
		VulkanAPI::m_destroyShaderBuffer(device, allocator, &vertexBuffer);
		VulkanAPI::m_destroyTexture(device, allocator, &object_texture);

		vkDestroyDescriptorPool(device, descriptorPool, NULL);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

		this->~VulkanDrawable();
	}

	void VulkanDrawable::updateObject(VkDevice device, VmaAllocator allocator)
	{
		VulkanAPI::m_destroyShaderBuffer(device, allocator, &indexBuffer);
		VulkanAPI::m_destroyShaderBuffer(device, allocator, &vertexBuffer);
		VulkanAPI::m_createVkBuffer(device, allocator, object_mesh.vertices.data(), sizeof(object_mesh.vertices[0]) * object_mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer);
		VulkanAPI::m_createVkBuffer(device, allocator, object_mesh.indices.data(), sizeof(object_mesh.indices[0]) * object_mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer);
		vkFreeDescriptorSets(device, descriptorPool, descriptorSets.size(), descriptorSets.data());
		vkDestroyDescriptorPool(device, descriptorPool, NULL);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
		vkDestroyPipeline(device, graphicsPipeline, NULL);
		vkDestroyPipelineLayout(device, pipelineLayout, NULL);

		createDescriptorLayout(device);
		createDescriptorPool(device);
		createDescriptorSet(device, allocator);
		createPipelineLayout(device);
		createGraphicsPipeline(device);
	}

	void VulkanDrawable::invalidateTexture(VkDevice device, VmaAllocator allocator)
	{
		VulkanAPI::m_destroyTexture(device, allocator, &object_texture);
	}

	bool VulkanDrawable::createPipelineLayout(VkDevice device)
	{
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(UniformBufferObject);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = NULL;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

		return vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) == VK_SUCCESS;
	}

	bool VulkanDrawable::recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent)
	{
		if (vertexBuffer.initialized == true)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);

			UpdateObjectPosition();
			UpdateObjectOrientation();
			pushConstants(device, commandBuffer, extent);

			for (int ind = 0; ind < descriptorSets.size(); ind++)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[ind], 0, NULL);
			}

			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object_mesh.indices.size()), 1, 0, 0, 0);
			return true;
		}

		return false;
	}

	bool VulkanDrawable::pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		ubo.view = glm::translate(glm::mat4_cast(p_Owner->getActiveViewport()->UpdateObjectOrientation(0.2)), -p_Owner->getActiveViewport()->UpdateObjectPosition(0.65)); // [ix iy iz w1( = 0)]-direction [jx jy jz w2( = 0)]-direction [kx ky kz w3( = 0)]-direction [tx ty tz w ( = 1)]-position
		ubo.proj = glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, near_plane, far_plane); //fov, aspect ratio, near clipping plane, far clipping plane
		ubo.proj[1][1] *= -1; //reverse Y coordinate
		ubo.scale = GetObjectScale();

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);
		return true;
	}

	bool VulkanDrawable::createGraphicsPipeline(VkDevice device)
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();

		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");

		VkShaderModule shaders[2] = { VulkanAPI::m_createShaderModule(device, vertShaderCode) , VulkanAPI::m_createShaderModule(device, fragShaderCode) };

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
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

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

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
			return false;

		vkDestroyShaderModule(device, shaders[0], nullptr);
		vkDestroyShaderModule(device, shaders[1], nullptr);

		return true;
	}

	bool VulkanDrawable::createDescriptorLayout(VkDevice device)
	{
		VkDescriptorSetLayoutBinding descriptorBindings{};
		descriptorBindings.binding = 1; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorBindings.descriptorCount = object_texture.texture_path != 0;
		descriptorBindings.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptorBindings.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutCreateInfo descriptorLayout{};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = NULL;
		descriptorLayout.bindingCount = 1;
		descriptorLayout.pBindings = &descriptorBindings;

		return vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, &descriptorSetLayout) == VK_SUCCESS;
	}

	bool VulkanDrawable::createDescriptorPool(VkDevice device)
	{
		VkDescriptorPoolSize descriptorTypePool;
		descriptorTypePool.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorTypePool.descriptorCount = object_texture.texture_path != 0;
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &descriptorTypePool;
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;


		return vkCreateDescriptorPool(device, &createInfo, NULL, &descriptorPool) == VK_SUCCESS;
	}

	bool VulkanDrawable::createDescriptorSet(VkDevice device, VmaAllocator allocator)
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.pNext = NULL;
		descriptorAllocInfo.descriptorPool = descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;

		std::vector<VkDescriptorImageInfo> imageInfo;
		if (object_texture.texture_path != NULL)
		{
			VkDescriptorImageInfo info;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info.imageView = object_texture.textureImageView;
			info.sampler = object_texture.textureSampler;
			imageInfo.push_back(info);
		}

		descriptorSets.resize(1);
		vkAllocateDescriptorSets(device, &descriptorAllocInfo, descriptorSets.data());

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

		vkUpdateDescriptorSets(device, 1, &writes, 0, NULL);

		return true;
	}
}