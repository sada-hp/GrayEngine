#include <pch.h>
#include "VulkanDrawable.h"
#include "VulkanRenderer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanDrawable::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		p_Owner = owner;
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
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
				{{{ 0.25, 0.25, 0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, 0.25, -0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, 0.25 , 0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, 0.25, -0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 0.0f }}},

				{{{ 0.25, -0.25, 0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 1.0f }}},
				{{{ -0.25, -0.25, -0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 0.0f }}},
				{{{ -0.25, -0.25 , 0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 1.f }}},
				{{{ 0.25, -0.25, -0.25, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 0.0f }}}
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

		populateDescriptorSets();
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

		VulkanAPI::DestroyPipeline(graphicsPipeline);
		VulkanAPI::DestroyPipelineLayout(pipelineLayout);

		for (std::vector<DescriptorSet>::iterator itt = descriptorSets.begin(); itt != descriptorSets.end(); ++itt)
		{
			VulkanAPI::FreeDescriptorSets(&(*itt).set, 1);
			VulkanAPI::DestroyDescriptorPool((*itt).descriptorPool);
			VulkanAPI::DestroyDescriptorLayout((*itt).descriptorSetLayout);
		}
	}

	void VulkanDrawable::updateObject()
	{
		VulkanAPI::DestroyPipeline(graphicsPipeline);
		VulkanAPI::DestroyPipelineLayout(pipelineLayout);

		for (std::vector<DescriptorSet>::iterator itt = descriptorSets.begin(); itt != descriptorSets.end(); ++itt)
		{
			VulkanAPI::FreeDescriptorSets(&(*itt).set, 1);
			VulkanAPI::DestroyDescriptorPool((*itt).descriptorPool);
			VulkanAPI::DestroyDescriptorLayout((*itt).descriptorSetLayout);
		}

		populateDescriptorSets();
		createPipelineLayout();
		createGraphicsPipeline();
	}

	void VulkanDrawable::invalidateTexture()
	{
		if (object_texture != nullptr)
		{
			resources->RemoveTexture(object_texture->texture_collection, logicalDevice, memAllocator);
			object_texture = nullptr;
		}
	}

	bool VulkanDrawable::createPipelineLayout()
	{
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(VertexConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPushConstantRange pushConstant2;
		pushConstant2.offset = sizeof(VertexConstants);
		pushConstant2.size = sizeof(PickingBufferObject);
		pushConstant2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkDescriptorSetLayout> layouts;
		for (std::vector<DescriptorSet>::iterator itt = descriptorSets.begin(); itt != descriptorSets.end(); ++itt)
		{
			layouts.push_back((*itt).descriptorSetLayout);
		}

		return VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant, pushConstant2 }, layouts, &pipelineLayout) == true;
	}

	bool VulkanDrawable::recordCommandBuffer(VkCommandBuffer commandBuffer, UINT32 mode)
	{
		if (mode == DrawMode::NORMAL && transparency > 0 || mode == DrawMode::TRANSPARENCY && transparency == 0) return false;

		if (object_mesh != nullptr && object_mesh->vertexBuffer.initialized == true)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			//UpdateObjectPosition();
			//UpdateObjectOrientation();
			pushConstants(commandBuffer);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0].set, 0, NULL);

			draw(commandBuffer);
			return true;
		}


		return false;
	}

	bool VulkanDrawable::draw(VkCommandBuffer commandBuffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
		//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object_mesh->indices.size()), 1, 0, 0, 0);

		return true;
	}

	bool VulkanDrawable::pushConstants(VkCommandBuffer cmd)
	{
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexConstants), &ubo);
		return true;
	}

	bool VulkanDrawable::createGraphicsPipeline()
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode;
		std::vector<char> fragShaderCode;
		std::array<VkShaderModule, 2> shaders;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = double_sided == 1 ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthBiasConstantFactor = 0.0f;
		rasterizationState.depthBiasClamp = 0.0f;
		rasterizationState.depthBiasSlopeFactor = 0.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;
		colorBlendState.blendConstants[0] = 1.0f; // Optional
		colorBlendState.blendConstants[1] = 1.0f; // Optional
		colorBlendState.blendConstants[2] = 1.0f; // Optional
		colorBlendState.blendConstants[3] = 1.0f; // Optional

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.pNext = nullptr;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f; // Optional
		depthStencilState.maxDepthBounds = 1.0f; // Optional
		depthStencilState.stencilTestEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.minSampleShading = 0;

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();


		// Final fullscreen pass pipeline
		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelineLayout;
		pipelineCI.renderPass = static_cast<VulkanRenderer*>(p_Owner)->getRenderPass();
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.subpass = 0;
		pipelineCI.pVertexInputState = &vertexInputInfo;

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		if (transparency == 0)
		{
			vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
			fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");

			shaders = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode) , VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = shaders[0];
			vertShaderStageInfo.pName = "main";

			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = shaders[1];
			fragShaderStageInfo.pName = "main";

			shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
			pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCI.pStages = shaderStages.data();

			blendAttachmentStates.resize(4);
			blendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentStates[0].blendEnable = VK_FALSE;
			blendAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentStates[1].blendEnable = VK_FALSE;
			blendAttachmentStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentStates[2].blendEnable = VK_FALSE;
			blendAttachmentStates[3].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentStates[3].blendEnable = VK_FALSE;

			colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
			colorBlendState.pAttachments = blendAttachmentStates.data();

			VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &graphicsPipeline);

			vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
			vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
		}
		else
		{
			vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert_transparent.spv");
			fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag_transparent.spv");

			shaders = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode) , VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = shaders[0];
			vertShaderStageInfo.pName = "main";

			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = shaders[1];
			fragShaderStageInfo.pName = "main";

			shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
			pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCI.pStages = shaderStages.data();

			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.pNext = nullptr;
			depthStencilState.depthTestEnable = VK_FALSE;
			depthStencilState.depthWriteEnable = VK_FALSE;
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.minDepthBounds = 0.0f; // Optional
			depthStencilState.maxDepthBounds = 1.0f; // Optional
			depthStencilState.stencilTestEnable = VK_FALSE;

			blendAttachmentStates.resize(0);

			colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
			colorBlendState.pAttachments = blendAttachmentStates.data();

			pipelineCI.subpass = 2;
			VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &graphicsPipeline);
			vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
			vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
		}
		
		return true;
	}

	void VulkanDrawable::subscribeDescriptor(VkShaderStageFlags shaderStage, uint8_t binding, VkDescriptorType descType, VkDescriptorImageInfo imageInfo, int targetLayout)
	{
		descriptorSets[targetLayout].bindings.push_back(binding);
		descriptorSets[targetLayout].stage.push_back(shaderStage);
		descriptorSets[targetLayout].imageInfos.push_back(imageInfo);
		descriptorSets[targetLayout].type.push_back(descType);
	}

	void VulkanDrawable::subscribeDescriptor(VkShaderStageFlags shaderStage, uint8_t binding, VkDescriptorType descType, VkDescriptorBufferInfo bufferInfo, int targetLayout)
	{
		descriptorSets[targetLayout].bindings.push_back(binding);
		descriptorSets[targetLayout].stage.push_back(shaderStage);
		descriptorSets[targetLayout].bufferInfos.push_back(bufferInfo);
		descriptorSets[targetLayout].type.push_back(descType);
	}

	bool VulkanDrawable::createDescriptorLayout()
	{
		bool res = true;
		for (int i = 0; i < descriptorSets.size(); i++)
		{
			std::vector<VkDescriptorSetLayoutBinding> descriptorBindings;

			for (int j = 0; j < descriptorSets[i].bindings.size(); j++)
			{
				VkDescriptorSetLayoutBinding descriptorBinding{};
				descriptorBinding.binding = descriptorSets[i].bindings[j]; // DESCRIPTOR_SET_BINDING_INDEX
				descriptorBinding.descriptorType = descriptorSets[i].type[j];
				descriptorBinding.descriptorCount = 1;
				descriptorBinding.stageFlags = descriptorSets[i].stage[j];
				descriptorBinding.pImmutableSamplers = NULL;
				descriptorBindings.push_back(descriptorBinding);
			}

			res = VulkanAPI::CreateDescriptorSetLayout(logicalDevice, descriptorBindings, &descriptorSets[i].descriptorSetLayout) & res;
		}

		return res;
	}

	bool VulkanDrawable::createDescriptorPool()
	{
		bool res = true;
		for (int i = 0; i < descriptorSets.size(); i++)
		{
			std::vector<VkDescriptorPoolSize> descriptorTypePools;

			for (int j = 0; j < descriptorSets[i].bindings.size(); j++)
			{
				VkDescriptorPoolSize descriptorTypePool;
				descriptorTypePool.type = descriptorSets[i].type[j];
				descriptorTypePool.descriptorCount = 1;
				descriptorTypePools.push_back(descriptorTypePool);
			}

			res = VulkanAPI::CreateDescriptorPool(logicalDevice, descriptorTypePools, &descriptorSets[i].descriptorPool) & res;
		}

		return res;
	}

	bool VulkanDrawable::createDescriptorSet()
	{
		std::vector<VkWriteDescriptorSet> writes;
		int buffOff, imgOff;
		for (int i = 0; i < descriptorSets.size(); i++)
		{
			buffOff = 0;
			imgOff = 0;
			VulkanAPI::AllocateDescriptorSet(logicalDevice, descriptorSets[i].descriptorPool, descriptorSets[i].descriptorSetLayout, &descriptorSets[i].set);

			for (int j = 0; j < descriptorSets[i].bindings.size(); j++)
			{
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descriptorSets[i].set;
				write.dstBinding = descriptorSets[i].bindings[j];
				write.dstArrayElement = 0;
				write.descriptorType = descriptorSets[i].type[j];
				write.descriptorCount = 1;

				if (descriptorSets[i].type[j] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || descriptorSets[i].type[j] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					write.pBufferInfo = &descriptorSets[i].bufferInfos[buffOff++];
				}
				else
				{
					auto a = &descriptorSets[i].imageInfos[imgOff];
					write.pImageInfo = &descriptorSets[i].imageInfos[imgOff++];
				}
				writes.push_back(write);
			}
		}

		vkUpdateDescriptorSets(logicalDevice, writes.size(), writes.data(), 0, NULL);
		return true;
	}
};