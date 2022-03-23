#include <pch.h>
#include "DrawableObj.h"
#include "VulkanAPI.h"

namespace GrEngine_Vulkan
{
	DrawableObj::DrawableObj()
	{

	}

	DrawableObj::~DrawableObj()
	{

	}

	void DrawableObj::initObject(VkDevice device)
	{
		createVkBuffer(VulkanAPI::m_getRenderer()->getMemAllocator(), object_mesh.vertices.data(), sizeof(object_mesh.vertices[0]) * object_mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer);
		createVkBuffer(VulkanAPI::m_getRenderer()->getMemAllocator(), object_mesh.indices.data(), sizeof(object_mesh.indices[0]) * object_mesh.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer);

		//createDescriptorLayout(device);
		//createDescriptorPool(device);
		//createDescriptorSet(device);
		createPipelineLayout(device);
		createGraphicsPipeline(device);
	}

	void DrawableObj::destroyObject(VkDevice device)
	{
		vkDestroyPipeline(device, graphicsPipeline, NULL);
		vkDestroyPipelineLayout(device, pipelineLayout, NULL);
		//vkFreeDescriptorSets(device, descriptorPool, descriptorSets.size(), descriptorSets.data());
		//vkDestroyDescriptorPool(device, descriptorPool, NULL);
		//vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

		destroyShaderBuffer(device, &indexBuffer);
		destroyShaderBuffer(device, &vertexBuffer);
		//destroyShaderBuffer(device, &uniformBuffer);

		this->~DrawableObj();
	}

	bool DrawableObj::createPipelineLayout(VkDevice device)
	{
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(UniformBufferObject);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		//pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = NULL;
		//pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

		return vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) == VK_SUCCESS;
	}

	bool DrawableObj::recordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkExtent2D extent)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0], 0, NULL);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.Buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);

		pushConstants(device, commandBuffer, extent);

		//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object_mesh.indices.size()), 1, 0, 0, 0);

		return true;
	}

	bool DrawableObj::createVkBuffer(VmaAllocator allocator, const void* bufData, uint32_t dataSize, VkBufferUsageFlags usage, ShaderBuffer* shaderBuffer)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = NULL;

#pragma region No_vma_code_backup

		//if (vkCreateBuffer(VulkanAPI::getRenderer()->logicalDevice, &bufferCreateInfo, nullptr, &shaderBuffer->Buffer) != VK_SUCCESS)
			//	return false;
		//VkMemoryAllocateInfo allocInfo{};
		//allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//allocInfo.allocationSize = shaderBuffer->MemoryRequirements.size;

		//VmaAllocationCreateInfo allocationInfo{};
		//allocationInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
		//allocationInfo.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		//allocationInfo.pool = VK_NULL_HANDLE;
		//allocationInfo.preferredFlags = NULL;
		//allocationInfo.priority = 1.0f;
		//allocationInfo.pUserData = VK_NULL_HANDLE;
		//allocationInfo.requiredFlags = 0;
		//allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		//vmaFindMemoryTypeIndexForBufferInfo(VulkanAPI::getRenderer()->memAllocator, &bufferCreateInfo, &allocationInfo, &allocInfo.memoryTypeIndex);

		//if (vkAllocateMemory(VulkanAPI::getRenderer()->logicalDevice, &allocInfo, nullptr, &shaderBuffer->BufferMemory) != VK_SUCCESS)
		//	return false;

		//vkBindBufferMemory(VulkanAPI::getRenderer()->logicalDevice, shaderBuffer->Buffer, shaderBuffer->BufferMemory, 0);

		//vkMapMemory(VulkanAPI::getRenderer()->logicalDevice, shaderBuffer->Allocation, 0, shaderBuffer->MemoryRequirements.size, 0, (void**)&shaderBuffer->pData);
		//memcpy(shaderBuffer->pData, bufData, dataSize);
		//vkUnmapMemory(VulkanAPI::getRenderer()->logicalDevice, shaderBuffer->Allocation);

#pragma endregion

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaallocInfo, &shaderBuffer->Buffer, &shaderBuffer->Allocation, nullptr);
		vkGetBufferMemoryRequirements(VulkanAPI::m_getRenderer()->logicalDevice, shaderBuffer->Buffer, &shaderBuffer->MemoryRequirements);

		vmaMapMemory(allocator, shaderBuffer->Allocation, (void**)&shaderBuffer->pData);
		memcpy(shaderBuffer->pData, bufData, dataSize);
		vmaUnmapMemory(allocator, shaderBuffer->Allocation);

		shaderBuffer->BufferInfo.buffer = shaderBuffer->Buffer;
		shaderBuffer->BufferInfo.offset = 0;
		shaderBuffer->BufferInfo.range = dataSize;

		shaderBuffer->MappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		shaderBuffer->MappedMemoryRange.memory = reinterpret_cast<VkDeviceMemory>(shaderBuffer->Allocation);
		shaderBuffer->MappedMemoryRange.offset = 0;
		shaderBuffer->MappedMemoryRange.size = dataSize;

		return true;
	}

	bool DrawableObj::pushConstants(VkDevice devicce, VkCommandBuffer cmd, VkExtent2D extent)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);

		return true;
	}

	void DrawableObj::destroyShaderBuffer(VkDevice device, ShaderBuffer* shader)
	{
		vkFlushMappedMemoryRanges(device, 1, &(shader->MappedMemoryRange));
		vkDestroyBuffer(device, shader->Buffer, NULL);
		vmaFreeMemory(VulkanAPI::m_getRenderer()->getMemAllocator(), shader->Allocation);
	}

	bool DrawableObj::createGraphicsPipeline(VkDevice device)
	{
		std::string solution_path = SOLUTION_DIR;
		std::vector<char> vertShaderCode = VulkanAPI::readFile(solution_path + "GrayEngine//Engine//Source//Vulkan//Shaders//vert.spv");
		std::vector<char> fragShaderCode = VulkanAPI::readFile(solution_path + "GrayEngine//Engine//Source//Vulkan//Shaders//frag.spv");

		VkShaderModule shaders[2] = { VulkanAPI::createShaderModule(device, vertShaderCode) , VulkanAPI::createShaderModule(device, fragShaderCode) };

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

		auto bindingDescription = DrawableObj::getBindingDescription();
		auto attributeDescriptions = DrawableObj::getAttributeDescriptions();

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
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

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
		pipelineInfo.renderPass = VulkanAPI::m_getRenderer()->getRenderPass();
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

	/*
	*Depricated function
	*/
	bool DrawableObj::createDescriptorLayout(VkDevice device)
	{
		VkDescriptorSetLayoutBinding descriptorBindings{};
		descriptorBindings.binding = 0; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorBindings.descriptorCount = 1;
		descriptorBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		descriptorBindings.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutCreateInfo descriptorLayout{};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = NULL;
		descriptorLayout.bindingCount = 1;
		descriptorLayout.pBindings = &descriptorBindings;

		return vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, &descriptorSetLayout) == VK_SUCCESS;
	}

	/*
	*Depricated function
	*/
	bool DrawableObj::createDescriptorPool(VkDevice device)
	{
		VkDescriptorPoolSize descriptorTypePool;
		descriptorTypePool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorTypePool.descriptorCount = 1;
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &descriptorTypePool;


		return vkCreateDescriptorPool(device, &createInfo, NULL, &descriptorPool) == VK_SUCCESS;
	}

	/*
	*Depricated function
	*/
	bool DrawableObj::createDescriptorSet(VkDevice device)
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.pNext = NULL;
		descriptorAllocInfo.descriptorPool = descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;

		descriptorSets.resize(1);
		vkAllocateDescriptorSets(device, &descriptorAllocInfo, descriptorSets.data());

		VkWriteDescriptorSet writes{};
		memset(&writes, 0, sizeof(writes));

		writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes.pNext = NULL;
		writes.dstSet = descriptorSets[0];
		writes.descriptorCount = 1;
		writes.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes.pBufferInfo = &uniformBuffer.BufferInfo;
		writes.dstArrayElement = 0;
		writes.dstBinding = 0;

		vkUpdateDescriptorSets(device, 1, &writes, 0, NULL);

		return true;
	}
}