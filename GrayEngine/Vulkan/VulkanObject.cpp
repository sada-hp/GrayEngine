#include <pch.h>
#include "VulkanObject.h"
#include "VulkanRenderer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	uint32_t VulkanObject::selected_id = 0;

	void VulkanObject::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		UINT id = ownerEntity->GetEntityID();
		ownerEntity->AddNewProperty("Shader");

		p_Owner = owner;
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		GenerateBoxMesh(0.5, 0.5, 0.5);

		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({""}, ownerEntity, GrEngine::TextureType::Color,false);
		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({""}, ownerEntity, GrEngine::TextureType::Normal);
		//updateSelectionPipeline();
		//updateShadowPipeline();
	}

	void VulkanObject::destroyObject()
	{
		VulkanAPI::DestroyPipeline(selectionPipeline);
		VulkanAPI::DestroyPipelineLayout(selectionLayout);
		VulkanAPI::DestroyPipeline(shadowPipeline);
		VulkanAPI::DestroyPipelineLayout(shadowLayout);

		VulkanDrawable::destroyObject();
	}

	void VulkanObject::Refresh()
	{
		updateObject();
	}

	void VulkanObject::updateObject()
	{
		VulkanDrawable::updateObject();
		updateSelectionPipeline();
		updateShadowPipeline();
	}

	void VulkanObject::recordSelection(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh != nullptr && object_mesh->vertexBuffer.initialized == true && selectable)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectionPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(cmd, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectionLayout, 0, 1, &descriptorSets[2].set, 0, NULL);
			pushConstants(cmd);

			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(object_mesh->indices.size()), 1, 0, 0, 0);
		}
	}

	void VulkanObject::recordShadowPass(VkCommandBuffer cmd, int instances)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
		vkCmdBindIndexBuffer(cmd, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowLayout, 0, 1, &descriptorSets[1].set, 0, NULL);
		ubo.model = GetObjectTransformation();
		ubo.scale = glm::vec4(ownerEntity->GetPropertyValue(PropertyType::Scale, glm::vec3(1.f)), 1.f);
		vkCmdPushConstants(cmd, shadowLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexConstants), &ubo);
		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(object_mesh->indices.size()), instances, 0, 0, 0);
	}

	bool VulkanObject::pushConstants(VkCommandBuffer cmd)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = GetObjectTransformation();
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		ubo.scale = glm::vec4(ownerEntity->GetPropertyValue(PropertyType::Scale, glm::vec3(1.f)), 1.f);
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexConstants), &ubo);

		opo.colors = ownerEntity->GetPropertyValue(PropertyType::Color, glm::vec4(1));
		if (ownerEntity->GetEntityID() == selected_id && ownerEntity->IsStatic() == false)
		{
			opo.colors *= glm::vec4(0.5, 0.75, 2, opo.colors.w);
		}

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VertexConstants), sizeof(FragmentBuffer), &opo);
		return true;
	}

	void VulkanObject::updateSelectionPipeline()
	{
		VulkanAPI::DestroyPipeline(selectionPipeline);
		VulkanAPI::DestroyPipelineLayout(selectionLayout);

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode;
		std::vector<char> fragShaderCode;
		vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_selection_vert.spv");
		fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_selection_frag.spv");
		if (vertShaderCode.size() == 0 && fragShaderCode.size() == 0)
		{
			selectable = false;
			return;
		}

		selectable = true;
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(VertexConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


		VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant }, { descriptorSets[2].descriptorSetLayout }, &selectionLayout);


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
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
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
		pipelineCI.layout = selectionLayout;
		pipelineCI.renderPass = static_cast<VulkanRenderer*>(p_Owner)->selectionPass;
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

		uint32_t obj_id = ownerEntity->GetEntityID();
		std::array<VkSpecializationMapEntry, 1> entries;
		entries[0].constantID = 0;
		entries[0].offset = 0;
		entries[0].size = sizeof(uint32_t);

		VkSpecializationInfo specializationInfo;
		specializationInfo.mapEntryCount = entries.size();
		specializationInfo.pMapEntries = entries.data();
		specializationInfo.dataSize = sizeof(uint32_t);
		specializationInfo.pData = &obj_id;

		shaderStages[1].pSpecializationInfo = &specializationInfo;

		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		blendAttachmentStates.resize(1);
		blendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentStates[0].blendEnable = VK_FALSE;

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &selectionPipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
	}

	void VulkanObject::updateShadowPipeline()
	{
		VulkanAPI::DestroyPipeline(shadowPipeline);
		VulkanAPI::DestroyPipelineLayout(shadowLayout);

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(VertexConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant }, { descriptorSets[1].descriptorSetLayout }, &shadowLayout);

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
		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_TRUE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = double_sided == 1 ? VK_CULL_MODE_NONE : VK_CULL_MODE_FRONT_BIT;
		//rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = VK_TRUE;
		rasterizationState.depthBiasConstantFactor = 2.75f;
		rasterizationState.depthBiasClamp = 0.01f;
		rasterizationState.depthBiasSlopeFactor = 2.75f;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 0;
		colorBlendState.pAttachments = nullptr;

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

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pNext = nullptr;
		dynamicState.flags = 0;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

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

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//shadows_vert.spv");
		std::vector<char> geomShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//shadows_geom.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + "Shaders//shadows_frag.spv");
		VkShaderModule shaders[3] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode), VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode), VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
		geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = shaders[1];
		geomShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[2];
		fragShaderStageInfo.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

		uint32_t spec = static_cast<VulkanRenderer*>(p_Owner)->lightsCount();
		std::array<VkSpecializationMapEntry, 1> entries;
		entries[0].constantID = 0;
		entries[0].offset = 0;
		entries[0].size = sizeof(uint32_t);

		VkSpecializationInfo specializationInfo;
		specializationInfo.mapEntryCount = entries.size();
		specializationInfo.pMapEntries = entries.data();
		specializationInfo.dataSize = sizeof(uint32_t);
		specializationInfo.pData = &spec;

		shaderStages[0].pSpecializationInfo = &specializationInfo;

		struct Specs {
			float thresh;
			uint32_t arr_size;
		} spec_frag;
		spec_frag = { alpha_threshold, (uint32_t)object_texture.size() };
		std::array<VkSpecializationMapEntry, 2> entriesFrag;
		entriesFrag[0].constantID = 0;
		entriesFrag[0].offset = 0;
		entriesFrag[0].size = sizeof(float);
		entriesFrag[1].constantID = 1;
		entriesFrag[1].offset = sizeof(float);
		entriesFrag[1].size = sizeof(uint32_t);

		VkSpecializationInfo specializationInfoFrag;
		specializationInfoFrag.mapEntryCount = entriesFrag.size();
		specializationInfoFrag.pMapEntries = entriesFrag.data();
		specializationInfoFrag.dataSize = sizeof(Specs);
		specializationInfoFrag.pData = &spec_frag;

		shaderStages[2].pSpecializationInfo = &specializationInfoFrag;

		VkGraphicsPipelineCreateInfo pipelineCI{};

		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = shadowLayout;
		pipelineCI.renderPass = static_cast<VulkanRenderer*>(p_Owner)->shadowPass;
		pipelineCI.basePipelineIndex = 0;
		pipelineCI.pVertexInputState = &vertexInputInfo;
		pipelineCI.pInputAssemblyState = &inputAssembly;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.subpass = 0;

		bool res = VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineCI, &shadowPipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[2], nullptr);
	}

	bool VulkanObject::createGraphicsPipeline()
	{
		shader_path = ownerEntity->GetPropertyValue(PropertyType::Shader, std::string(shader_path));
		transparency = ownerEntity->GetPropertyValue(PropertyType::Transparency, 0);
		double_sided = ownerEntity->GetPropertyValue(PropertyType::DoubleSided, 0);
		alpha_threshold = ownerEntity->GetPropertyValue(PropertyType::AlphaThreshold, 0.5f);

		std::string solution_path = GrEngine::Globals::getExecutablePath();
		std::vector<char> vertShaderCode;
		std::vector<char> geomShaderCode;
		std::vector<char> fragShaderCode;

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
		VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		std::vector<VkShaderModule> shaders;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		if (transparency == 0)
		{
			struct Specs
			{
				uint32_t arr_size;
				float near_pl;
				float far_pl;
				uint32_t use_normal;
				float threshold;
				float n_strength;
			} specs;
			specs = { (uint32_t)object_texture.size(), VulkanRenderer::NearPlane, VulkanRenderer::FarPlane, (uint32_t)object_normal.size(), alpha_threshold, GetOwnerEntity()->GetPropertyValue(PropertyType::NormalStrength, 1.f)};
			std::array<VkSpecializationMapEntry, 6> entries;
			entries[0].constantID = 0;
			entries[0].offset = 0;
			entries[0].size = sizeof(uint32_t);
			entries[1].constantID = 1;
			entries[1].offset = offsetof(Specs, near_pl);
			entries[1].size = sizeof(float);
			entries[2].constantID = 2;
			entries[2].offset = offsetof(Specs, far_pl);
			entries[2].size = sizeof(float);
			entries[3].constantID = 3;
			entries[3].offset = offsetof(Specs, use_normal);
			entries[3].size = sizeof(uint32_t);
			entries[4].constantID = 4;
			entries[4].offset = offsetof(Specs, threshold);
			entries[4].size = sizeof(float);
			entries[5].constantID = 5;
			entries[5].offset = offsetof(Specs, n_strength);
			entries[5].size = sizeof(float);

			VkSpecializationInfo specializationInfo;
			specializationInfo.mapEntryCount = entries.size();
			specializationInfo.pMapEntries = entries.data();
			specializationInfo.dataSize = sizeof(Specs);
			specializationInfo.pData = &specs;

			vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
			geomShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_geom.spv");
			fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");
			if (vertShaderCode.size() > 0)
			{
				shaders.push_back(VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode));
				vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				vertShaderStageInfo.module = shaders.back();
				vertShaderStageInfo.pName = "main";
				shaderStages.push_back(vertShaderStageInfo);
			}
			if (geomShaderCode.size() > 0)
			{
				shaders.push_back(VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode));
				geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				geomShaderStageInfo.module = shaders.back();
				geomShaderStageInfo.pName = "main";
				shaderStages.push_back(geomShaderStageInfo);
			}
			if (fragShaderCode.size() > 0)
			{
				shaders.push_back(VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode));
				fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				fragShaderStageInfo.module = shaders.back();
				fragShaderStageInfo.pName = "main";
				shaderStages.push_back(fragShaderStageInfo);
				shaderStages.back().pSpecializationInfo = &specializationInfo;
			}

			//shaders = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode), VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode), VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode) };
			//shaderStages = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };
			//shaderStages[2].pSpecializationInfo = &specializationInfo;

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

			for (int i = 0; i < shaders.size(); i++)
			{
				vkDestroyShaderModule(logicalDevice, shaders[i], nullptr);
			}
		}
		else
		{
			struct Specs
			{
				uint32_t arr_size;
				float near_pl;
				float far_pl;
			} specs;
			specs = { (uint32_t)object_texture.size(), VulkanRenderer::NearPlane, VulkanRenderer::FarPlane };
			std::array<VkSpecializationMapEntry, 3> entries;
			entries[0].constantID = 0;
			entries[0].offset = 0;
			entries[0].size = sizeof(uint32_t);
			entries[1].constantID = 1;
			entries[1].offset = offsetof(Specs, near_pl);
			entries[1].size = sizeof(float);
			entries[2].constantID = 2;
			entries[2].offset = offsetof(Specs, far_pl);
			entries[2].size = sizeof(float);

			VkSpecializationInfo specializationInfo;
			specializationInfo.mapEntryCount = entries.size();
			specializationInfo.pMapEntries = entries.data();
			specializationInfo.dataSize = sizeof(Specs);
			specializationInfo.pData = &specs;

			vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert_transparent.spv");
			geomShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_geom.spv");
			fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag_transparent.spv");

			if (vertShaderCode.size() > 0)
			{
				shaders.push_back(VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode));
				vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				vertShaderStageInfo.module = shaders.back();
				vertShaderStageInfo.pName = "main";
				shaderStages.push_back(vertShaderStageInfo);
			}
			if (geomShaderCode.size() > 0)
			{
				shaders.push_back(VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode));
				geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				geomShaderStageInfo.module = shaders.back();
				geomShaderStageInfo.pName = "main";
				shaderStages.push_back(geomShaderStageInfo);
			}
			if (fragShaderCode.size() > 0)
			{
				shaders.push_back(VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode));
				fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				fragShaderStageInfo.module = shaders.back();
				fragShaderStageInfo.pName = "main";
				shaderStages.push_back(fragShaderStageInfo);
				shaderStages.back().pSpecializationInfo = &specializationInfo;
			}

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

			for (int i = 0; i < shaders.size(); i++)
			{
				vkDestroyShaderModule(logicalDevice, shaders[i], nullptr);
			}
		}

		return true;
	}

	void VulkanObject::createDescriptors()
	{
		descriptorSets.clear();
		descriptorSets.resize(3);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptorSets[1].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptorSets[2].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		populateDescriptorSets();

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}


	void VulkanObject::populateDescriptorSets()
	{
		transparency = ownerEntity->GetPropertyValue(PropertyType::Transparency, 0);

		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->viewProjUBO.BufferInfo);

		std::vector<VkDescriptorImageInfo> imageInfo;
		std::vector<VkDescriptorImageInfo> normalsInfo;
		if (object_texture.size() > 0)
		{
			for (int i = 0; i < object_texture.size(); i++)
			{
				if (object_texture[i] != nullptr && object_texture[i]->texInfo.descriptor.imageView != VK_NULL_HANDLE)
				{
					imageInfo.push_back(object_texture[i]->texInfo.descriptor);
				}
			}
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo, 1);
		}

		if (object_normal.size() > 0)
		{
			for (int i = 0; i < object_normal.size(); i++)
			{
				if (object_normal[i] != nullptr && object_normal[i]->texInfo.descriptor.imageView != VK_NULL_HANDLE)
				{
					normalsInfo.push_back(object_normal[i]->texInfo.descriptor);
				}
			}
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, normalsInfo);
		}

		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->shadowBuffer.BufferInfo, 1);
		subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->viewProjUBO.BufferInfo, 2);

		int index = 3;

		if (transparency > 0)
		{
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, static_cast<VulkanRenderer*>(p_Owner)->position.texInfo.descriptor);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->transBuffer.BufferInfo);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<VulkanRenderer*>(p_Owner)->headIndex.texInfo.descriptor);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->nodeBfffer.BufferInfo);
		}

		//subscribeDescriptor(VK_SHADER_STAGE_VERTEX_BIT, index++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->shadowBuffer.BufferInfo);
		//subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<VulkanRenderer*>(p_Owner)->shadowMap.texInfo.descriptor);
	}

	bool VulkanObject::LoadModel(const char* gmf_path, const char* mesh_path, std::vector<std::string> textures_vector, std::vector<std::string> normals_vector)
	{
		static_cast<VulkanRenderer*>(p_Owner)->waitForRenderer();

		VulkanObject* ref_obj = this;
		GrEngine::Entity* ref_own = ownerEntity;

		VulkanRenderer* inst = static_cast<VulkanRenderer*>(p_Owner);
		std::map<int, std::future<void>> processes_map;
		std::map<std::string, std::vector<int>> materials_map;

		texture_names.clear();
		processes_map[processes_map.size()] = std::async(std::launch::async, [mesh_path, textures_vector, ref_obj, inst]()
			{
				ref_obj->LoadMesh(mesh_path);
			});

		inst->assignTextures(textures_vector, ownerEntity, GrEngine::TextureType::Color);
		//Ideally normals should match the number of used surfaces
		if (normals_vector.size() == 0) normals_vector.resize(textures_vector.size());
		inst->assignTextures(normals_vector, ownerEntity, GrEngine::TextureType::Normal);

		for (int ind = 0; ind < processes_map.size(); ind++)
		{
			if (processes_map[ind].valid())
			{
				processes_map[ind].wait();
			}
		}

		texture_names = textures_vector;
		gmf_name = gmf_path;
		mesh_name = mesh_path;
		updateObject();

		return true;
	}

	bool VulkanObject::LoadMesh(const char* mesh_path)
	{
		auto resource = resources->GetMeshResource(mesh_path);
		std::string solution = GrEngine::Globals::getExecutablePath();
		std::string model_path = mesh_path;

		if (resource == nullptr)
		{
			if (object_mesh != nullptr)
			{
				resources->RemoveMesh(object_mesh, logicalDevice, memAllocator);
				object_mesh = nullptr;
			}

			std::unordered_map<Vertex, uint32_t> uniqueVertices{};
			Assimp::Importer importer;
			Mesh* target_mesh = new Mesh();

			const aiScene* model;

			if (model_path.size() >= solution.size() && model_path.substr(0, solution.size()) == solution)
			{
				model = importer.ReadFile(mesh_path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
			}
			else
			{
				model = importer.ReadFile(solution + mesh_path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
			}

			if (model == NULL)
			{
				Logger::Out("Could not load the mesh %c%s%c!", OutputType::Error, '"', mesh_path, '"');
				return false;
			}

			target_mesh->mesh_path = mesh_path;

			float highest_pointx = 0.f;
			float highest_pointy = 0.f;
			float highest_pointz = 0.f;

			for (int mesh_ind = 0; mesh_ind < model->mNumMeshes; mesh_ind++)
			{
				auto num_vert = model->mMeshes[mesh_ind]->mNumVertices;
				auto cur_mesh = model->mMeshes[mesh_ind];
				auto name3 = model->mMeshes[mesh_ind]->mName;
				auto uv_ind = mesh_ind;

				for (int vert_ind = 0; vert_ind < num_vert; vert_ind++)
				{
					auto coord = model->mMeshes[mesh_ind]->mTextureCoords[0];

					GrEngine_Vulkan::Vertex vertex{};
					vertex.pos = { cur_mesh->mVertices[vert_ind].x, cur_mesh->mVertices[vert_ind].y, cur_mesh->mVertices[vert_ind].z, 1.0f };
					vertex.uv = { coord[vert_ind].x, coord[vert_ind].y };
					vertex.uv_index = uv_ind;
					if (cur_mesh->HasNormals())
						vertex.norm = { cur_mesh->mNormals[vert_ind].x, cur_mesh->mNormals[vert_ind].y, cur_mesh->mNormals[vert_ind].z };
					if (cur_mesh->HasTangentsAndBitangents())
					{
						vertex.tang = { cur_mesh->mTangents[vert_ind].x, cur_mesh->mTangents[vert_ind].y, cur_mesh->mTangents[vert_ind].z };
						vertex.bitang = { cur_mesh->mBitangents[vert_ind].x, cur_mesh->mBitangents[vert_ind].y, cur_mesh->mBitangents[vert_ind].z };
					}

					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(target_mesh->vertices.size());
						target_mesh->vertices.push_back(vertex);
					}

					if (highest_pointx < cur_mesh->mVertices[vert_ind].x)
						highest_pointx = cur_mesh->mVertices[vert_ind].x;
					if (highest_pointy < cur_mesh->mVertices[vert_ind].y)
						highest_pointy = cur_mesh->mVertices[vert_ind].y;
					if (highest_pointz < cur_mesh->mVertices[vert_ind].z)
						highest_pointz = cur_mesh->mVertices[vert_ind].z;

					int index = uniqueVertices[vertex];
					target_mesh->indices.push_back(index);
				}
			}

			//VulkanResourceManager::CalculateNormals(target_mesh);
			//VulkanResourceManager::CalculateTangents(target_mesh);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);
			target_mesh->bounds = glm::vec3(highest_pointx, highest_pointy, highest_pointz);
			bound = target_mesh->bounds;

			auto resource = resources->AddMeshResource(mesh_path, target_mesh);
			object_mesh = resource->AddLink();
		}
		else if (!resource->Compare(object_mesh))
		{
			if (object_mesh != nullptr)
			{
				resources->RemoveMesh(object_mesh, logicalDevice, memAllocator);
				object_mesh = nullptr;
			}

			object_mesh = resource->AddLink();
			bound = object_mesh->bounds;
		}

		return true;
	}

	void VulkanObject::GeneratePlaneMesh(float width, int subdivisions)
	{
		subdivisions = 1+subdivisions;
		std::string res_name = std::string("plane") + std::to_string(width) + std::to_string(subdivisions);
		auto resource = resources->GetMeshResource(res_name.c_str());

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh, logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		if (resource == nullptr)
		{
			std::unordered_map<Vertex, uint32_t> uniqueVertices{};
			Mesh* target_mesh = new Mesh();
			double divisions = width / subdivisions;

			for (int row = 0; row < subdivisions; row++)
			{
				for (int col = 0; col < subdivisions; col++)
				{
					Vertex vertex{};
					vertex.uv = glm::vec2(divisions * col, 1.0f - divisions * row);
					vertex.pos = glm::vec4(width * (divisions * col - width / 2), 0, width * (divisions * row - width / 2), 1.0f);
					target_mesh->vertices.push_back(vertex);

					if (row + 1 < subdivisions && col + 1 < subdivisions)
					{
						int index = subdivisions * row + col;

						target_mesh->indices.push_back(index + subdivisions);
						target_mesh->indices.push_back(index + 1);
						target_mesh->indices.push_back(index);

						target_mesh->indices.push_back(index + 1);
						target_mesh->indices.push_back(index + subdivisions);
						target_mesh->indices.push_back(index + subdivisions + 1);
					}
				}
			}

			//VulkanResourceManager::CalculateNormals(target_mesh);
			//VulkanResourceManager::CalculateTangents(target_mesh);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);

			auto resource = resources->AddMeshResource(res_name.c_str(), target_mesh);
			object_mesh = resource->AddLink();
		}
		else
		{
			object_mesh = resource->AddLink();
		}
	}

	void VulkanObject::GenerateBoxMesh(float width, float height, float depth)
	{
		std::string res_name = std::string("box") + std::to_string(width) + std::to_string(height) + std::to_string(depth);
		auto resource = resources->GetMeshResource(res_name.c_str());

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh, logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		float ycoord = height / 2;
		float xcoord = width / 2;
		float zcoord = depth / 2;

		if (resource == nullptr)
		{
			Mesh* target_mesh = new Mesh();

			target_mesh->vertices = {
				{{{xcoord, ycoord, -zcoord, 1.f},{0.577400, 0.577400, -0.577300},{ 1.f, 1.0f }}},
				{{{xcoord, ycoord, zcoord, 1.f},{0.577400, 0.577400, 0.577300},{ 1.f, 1.0f }}},
				{{{xcoord, -ycoord, zcoord, 1.f},{0.577400, -0.577300, 0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, ycoord, -zcoord, 1.f},{-0.577300, 0.577400, -0.577400},{ 1.f, 1.0f }}},
				{{{xcoord, -ycoord, -zcoord, 1.f},{0.577400, -0.577300, -0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, -ycoord, -zcoord, 1.f},{-0.577400, -0.577300, -0.577400},{ 1.f, 1.0f }}},
				{{{xcoord, ycoord, -zcoord, 1.f},{0.577300, 0.577400, -0.577400},{ 1.f, 1.0f }}},
				{{{xcoord, ycoord, zcoord, 1.f},{0.577300, 0.577400, 0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, ycoord, zcoord, 1.f},{-0.577300, 0.577400, 0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, -ycoord, zcoord, 1.f},{-0.577400, -0.577300, 0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, ycoord, zcoord, 1.f},{-0.577400, 0.577400, 0.577300},{ 1.f, 1.0f }}},
				{{{-xcoord, ycoord, -zcoord, 1.f},{-0.577400, 0.577300, -0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, -ycoord, -zcoord, 1.f},{-0.577400, -0.577300, -0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, ycoord, -zcoord, 1.f},{-0.577300, 0.577400, -0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, ycoord, zcoord, 1.f},{-0.577300, 0.577400, 0.577400},{ 1.f, 1.0f }}},
				{{{xcoord, -ycoord, zcoord, 1.f},{0.577400, -0.577300, 0.577400},{ 1.f, 1.0f }}},
				{{{-xcoord, -ycoord, zcoord, 1.f},{-0.577400, -0.577300, 0.577400},{ 1.f, 1.0f }}},
				{{{xcoord, ycoord, -zcoord, 1.f},{0.577300, 0.577400, -0.577400},{ 1.f, 1.0f }}},
				{{{xcoord, -ycoord, -zcoord, 1.f},{0.577400, -0.577400, -0.577300},{ 1.f, 1.0f }}},
				{{{-xcoord, -ycoord, zcoord, 1.f},{-0.577400, -0.577400, 0.577300},{ 1.f, 1.0f }}},
				{{{-xcoord, -ycoord, -zcoord, 1.f},{-0.577300, -0.577400, -0.577400},{ 1.f, 1.0f }}},
				{{ {xcoord, -ycoord, zcoord, 1.f},{0.577300, -0.577400, 0.577400},{ 1.f, 1.0f }}}
			};

			target_mesh->indices = { 0, 1, 2, 3, 4, 5, 3, 6, 4, 7, 8, 9, 10, 11, 12, 13, 14, 7, 0, 2, 4, 7, 9, 15, 10, 12, 16, 13, 7, 17, 18, 19, 20, 18, 21, 19 };

			//CalculateNormals(target_mesh);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);
			resource = resources->AddMeshResource(res_name.c_str(), target_mesh);
			object_mesh = resource->AddLink();
		}
		else
		{
			object_mesh = resource->AddLink();
		}
	}

	void VulkanObject::GenerateSphereMesh(double radius, int rings, int slices)
	{
		std::string res_name = std::string("sphere") + std::to_string(radius) + std::to_string(rings) + std::to_string(slices);
		auto resource = resources->GetMeshResource(res_name.c_str());

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh, logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		if (resource == nullptr)
		{
			double slice_sub = 360. / (double)slices;
			double ring_sub = 2 * radius / (float)rings;
			Mesh* target_mesh = new Mesh();
			target_mesh->vertices.push_back({ {{0, radius, 0, 1.f},{1, 1, 1},{ .5, .5 }} });

			double h = radius - ring_sub;
			double actual_radius = glm::sqrt(glm::pow(radius, 2)-glm::pow(h, 2));
			double frac = actual_radius / (double)radius;
			for (int s = 0; s < slices; s++)
			{
				double sin = (double)glm::sin(glm::radians((float)(slice_sub * s)));
				double cos = (double)glm::cos(glm::radians((float)(slice_sub * s)));
				target_mesh->vertices.push_back({ {{actual_radius *sin, h, actual_radius * cos, 1.},{1., 1., 1.},{ .5 * .5 * frac * sin, .5 + .5 * frac * cos }} });
			}
			target_mesh->vertices.push_back({ {{actual_radius * (double)glm::sin(0), h, actual_radius * (double)glm::cos(0), 1.},{1., 1., 1.}, { .5 + .5 * frac * (double)glm::sin(0), .5 + .5 * frac * (double)glm::cos(0) }} });

			for (int i = 1; i < target_mesh->vertices.size() - 1; i++)
			{
				target_mesh->indices.push_back(0);
				target_mesh->indices.push_back(i);
				target_mesh->indices.push_back(i + 1);
			}

			target_mesh->indices.push_back(0);
			target_mesh->indices.push_back(target_mesh->vertices.size() - 1);
			target_mesh->indices.push_back(1);

			for (int r = 1; r < rings - 1; r++)
			{
				h = radius - ring_sub * (r + 1);
				actual_radius = glm::sqrt(glm::pow(radius, 2) - glm::pow(h, 2));
				frac = actual_radius / radius;
				for (int s = 0; s < slices; s++)
				{
					double sin = (double)glm::sin(glm::radians((float)(slice_sub * s)));
					double cos = (double)glm::cos(glm::radians((float)(slice_sub * s)));
					target_mesh->vertices.push_back({ {{actual_radius * sin, h, actual_radius * cos, 1.},{1., 1., 1.}, { .5 + .5 * frac * sin, .5 + .5 * frac * cos }} });
				}
				target_mesh->vertices.push_back({ {{actual_radius * (double)glm::sin(0), h, actual_radius * (double)glm::cos(0), 1.},{1., 1., 1.}, { .5 + .5 * frac * (double)glm::sin(0), .5 + .5 * frac * (double)glm::cos(0) }} });
			}

			for (int i = slices + 1; i < target_mesh->vertices.size() - 1; i++)
			{
				target_mesh->indices.push_back(i - slices);
				target_mesh->indices.push_back(i);
				target_mesh->indices.push_back(i + 1);

				target_mesh->indices.push_back(i + 1);
				target_mesh->indices.push_back(i - slices + 1);
				target_mesh->indices.push_back(i - slices);
			}

			target_mesh->vertices.push_back({ {{0, -radius, 0, 1.},{1., 1., 1.},{ .5, .5 }} });

			for (int i = target_mesh->vertices.size() - slices - 1; i < target_mesh->vertices.size() - 2; i++)
			{
				target_mesh->indices.push_back(target_mesh->vertices.size() - 1);
				target_mesh->indices.push_back(i + 1);
				target_mesh->indices.push_back(i);
			}

			target_mesh->indices.push_back(target_mesh->vertices.size() - 1);
			target_mesh->indices.push_back(target_mesh->vertices.size() - slices - 1);
			target_mesh->indices.push_back(target_mesh->vertices.size() - 2);

			VulkanResourceManager::CalculateNormals(target_mesh);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);
			resource = resources->AddMeshResource(res_name.c_str(), target_mesh);
			object_mesh = resource->AddLink();
		}
		else
		{
			object_mesh = resource->AddLink();
		}
	}
};