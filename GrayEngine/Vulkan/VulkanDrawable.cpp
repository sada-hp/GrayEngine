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

		populateDescriptorSets();
	}

	void VulkanDrawable::LinkExternalStorageBuffer(VkShaderStageFlagBits stage, ShaderBuffer* buffer)
	{
		globalBuffers[stage] = buffer;
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

		globalBuffers.clear();
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
		pushConstant.size = sizeof(UniformBufferObject);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPushConstantRange pushConstant2;
		pushConstant2.offset = sizeof(UniformBufferObject);
		pushConstant2.size = sizeof(PickingBufferObject);
		pushConstant2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::vector<VkDescriptorSetLayout> layouts;
		for (std::vector<DescriptorSet>::iterator itt = descriptorSets.begin(); itt != descriptorSets.end(); ++itt)
		{
			layouts.push_back((*itt).descriptorSetLayout);
		}

		return VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant, pushConstant2 }, layouts, &pipelineLayout) == true;
	}

	bool VulkanDrawable::recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh != nullptr && object_mesh->vertexBuffer.initialized == true)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			//UpdateObjectPosition();
			//UpdateObjectOrientation();
			pushConstants(commandBuffer, extent, mode);

			for (std::vector<DescriptorSet>::iterator itt = descriptorSets.begin(); itt != descriptorSets.end(); ++itt)
			{
				if ((*itt).bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(*itt).set, 0, NULL);
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
		//rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = dynamic_cast<VulkanRenderer*>(p_Owner)->GetSampling();
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.minSampleShading = .35f;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
		pipelineInfo.renderPass = static_cast<VulkanRenderer*>(p_Owner)->getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.pDepthStencilState = &_depthStencil;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineInfo, &graphicsPipeline) != true)
			return false;

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);

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

				if (descriptorSets[i].type[j] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
				{
					write.pBufferInfo = &descriptorSets[i].bufferInfos[buffOff++];
				}
				else
				{
					write.pImageInfo = &descriptorSets[i].imageInfos[imgOff++];
				}
				writes.push_back(write);
			}
		}

		vkUpdateDescriptorSets(logicalDevice, writes.size(), writes.data(), 0, NULL);
		return true;
	}
};