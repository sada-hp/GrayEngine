#include "pch.h"
#include "VulkanTerrain.h"
#include "VulkanAPI.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GrEngine_Vulkan
{
	void VulkanTerrain::initObject(VkDevice device, VmaAllocator allocator, GrEngine::Renderer* owner)
	{
		properties.push_back(new Shader("Shaders\\terrain", this));
		shader_path = "Shaders\\terrain";

		p_Owner = owner;
		resources = &static_cast<VulkanAPI*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		physComponent = new GrEngineBullet::BulletAPI::BulletPhysObject(this);

		GrEngine::Engine::GetContext()->GetPhysics()->AddSimulationObject(physComponent);
		bool res = static_cast<VulkanAPI*>(owner)->assignTextures({ "D:\\GrEngine\\GrayEngine\\bin\\Debug-x64\\WorldEditorApp\\Content\\terrain.png" }, this);

		descriptorSets.resize(2);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptorSets[1].bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		descriptorSets[0].set.resize(1);
		descriptorSets[1].set.resize(1);

		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, &size, sizeof(ComputeSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terIn);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, size.resolution * size.resolution * sizeof(ComputeVertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terOut);
		terI = calloc(size.resolution * size.resolution, sizeof(ComputeVertex));
		vmaMapMemory(memAllocator, terOut.Allocation, (void**)&terI);

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
		createPipelineLayout();
		createGraphicsPipeline();

		GenerateTerrain(size.resolution);
	}

	void VulkanTerrain::destroyObject()
	{
		vmaUnmapMemory(memAllocator, terOut.Allocation);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terIn);
		VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terOut);

		VulkanDrawable::destroyObject();
	}


	void VulkanTerrain::GenerateTerrain(uint16_t resolution)
	{
		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = static_cast<VulkanAPI*>(p_Owner)->compute_bit.value();
		vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &computePool);
		VkCommandBufferAllocateInfo cmdAllocInfo{};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.commandPool = computePool;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(logicalDevice, &cmdAllocInfo, &computeCmd);
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(computeCmd, &beginInfo);
		vkCmdBindPipeline(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
		vkCmdBindDescriptorSets(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 1, &descriptorSets[1].set[0], 0, nullptr);
		vkCmdDispatch(computeCmd, 10, 1, 1);
		vkEndCommandBuffer(computeCmd);
		vkGetDeviceQueue(logicalDevice, static_cast<VulkanAPI*>(p_Owner)->compute_bit.value(), 0, &computeQueue);
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkCreateFence(logicalDevice, &fenceInfo, nullptr, &computeFence);
		VkSubmitInfo submiteInfo{};
		submiteInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submiteInfo.commandBufferCount = 1;
		submiteInfo.pCommandBuffers = &computeCmd;
		vkQueueSubmit(computeQueue, 1, &submiteInfo, computeFence);
		vkWaitForFences(logicalDevice, 1, &computeFence, true, UINT_MAX);

		std::vector<ComputeVertex> vert;
		vert.resize(resolution * resolution);
		memcpy_s(vert.data(), resolution * resolution * sizeof(ComputeVertex), terI, resolution * resolution * sizeof(ComputeVertex));

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		object_mesh = new Mesh();

		int row = 0;
		int col = 0;
		for (std::vector<ComputeVertex>::iterator itt = vert.begin(); itt != vert.end(); ++itt)
		{
			GrEngine_Vulkan::Vertex vertex{};
			vertex.pos = (*itt).pos;
			vertex.uv = { (*itt).uv.x, (*itt).uv.y };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(object_mesh->vertices.size());
				object_mesh->vertices.push_back(vertex);
			}

			if (row + 1 < resolution && col + 1 < resolution)
			{
				int index = resolution * row + col;
				object_mesh->indices.push_back(index);
				object_mesh->indices.push_back(index + 1);
				object_mesh->indices.push_back(index + resolution);

				object_mesh->indices.push_back(index + resolution);
				object_mesh->indices.push_back(index + 1);
				object_mesh->indices.push_back(index + resolution + 1);
			}

			col++;
			row += col / resolution;
			col %= resolution;
		}

		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->vertices.data(), sizeof(object_mesh->vertices[0]) * object_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &object_mesh->vertexBuffer);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, object_mesh->indices.data(), sizeof(object_mesh->indices[0]) * object_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &object_mesh->indexBuffer);
		calculateCollisions();

		vkFreeCommandBuffers(logicalDevice, computePool, 1, &computeCmd);
		vkDestroyCommandPool(logicalDevice, computePool, nullptr);
		vkDestroyFence(logicalDevice, computeFence, nullptr);
	}

	void VulkanTerrain::calculateCollisions()
	{
		std::unordered_map<GrEngine_Vulkan::Vertex, uint32_t> uniqueVertices{};

		if (colMesh != nullptr)
		{
			delete colMesh;
			colMesh = nullptr;
		}

		colMesh = new btTriangleMesh();
		int index = 0;
		for (std::vector<GrEngine_Vulkan::Vertex>::iterator itt = object_mesh->vertices.begin(); itt != object_mesh->vertices.end(); ++itt)
		{
			colMesh->findOrAddVertex(btVector3((*itt).pos.x, (*itt).pos.y, (*itt).pos.z), false);
		}

		for (int i = 3; i < object_mesh->indices.size(); i += 3)
		{
			colMesh->addTriangleIndices(object_mesh->indices[i - 3], object_mesh->indices[i - 2], object_mesh->indices[i - 1]);
		}

		physComponent->UpdateCollisionShape(new btBvhTriangleMeshShape(colMesh, false));
		//physComponent->CalculatePhysics();
	}

	bool VulkanTerrain::recordCommandBuffer(VkCommandBuffer commandBuffer, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh->vertexBuffer.initialized == true)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			pushConstants(commandBuffer, extent, mode);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0].set[0], 0, NULL);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object_mesh->indices.size()), 1, 0, 0, 0);

			return true;
		}

		return false;
	}

	bool VulkanTerrain::pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = glm::mat4{ 1.f };
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		ubo.view = glm::translate(glm::mat4_cast(p_Owner->getActiveViewport()->GetObjectOrientation()), -p_Owner->getActiveViewport()->GetObjectPosition()); // [ix iy iz w1( = 0)]-direction [jx jy jz w2( = 0)]-direction [kx ky kz w3( = 0)]-direction [tx ty tz w ( = 1)]-position
		ubo.proj = glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, near_plane, far_plane); //fov, aspect ratio, near clipping plane, far clipping plane
		ubo.proj[1][1] *= -1; //reverse Y coordinate
		ubo.scale = glm::vec3(size.width, size.height, size.depth);

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(UniformBufferObject), &ubo);
		return true;
	}

	bool VulkanTerrain::createPipelineLayout()
	{
		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(UniformBufferObject);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

		std::vector<VkPushConstantRange> ranges = { pushConstant };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSets[0].descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
		pipelineLayoutInfo.pPushConstantRanges = ranges.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo2{};
		pipelineLayoutInfo2.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo2.setLayoutCount = 1;
		pipelineLayoutInfo2.pSetLayouts = &descriptorSets[1].descriptorSetLayout;
		pipelineLayoutInfo2.pushConstantRangeCount = 0;
		pipelineLayoutInfo2.pPushConstantRanges = 0;

		vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout) == VK_SUCCESS;
		return vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo2, NULL, &computeLayout) == VK_SUCCESS;
	}

	bool VulkanTerrain::createGraphicsPipeline()
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();

		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
		std::vector<char> contShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_tesc.spv");
		std::vector<char> evalShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_tese.spv");
		std::vector<char> geomShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_geom.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");
		std::vector<char> compShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_comp.spv");

		VkShaderModule shaders[6] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, contShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, evalShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, compShaderCode)
		};

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaders[0];
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo tesconShaderStageInfo{};
		tesconShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		tesconShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		tesconShaderStageInfo.module = shaders[1];
		tesconShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo tesevalShaderStageInfo{};
		tesevalShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		tesevalShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		tesevalShaderStageInfo.module = shaders[2];
		tesevalShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
		geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = shaders[3];
		geomShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaders[4];
		fragShaderStageInfo.pName = "main";

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

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
		//rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
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

		VkPipelineTessellationStateCreateInfo tesselationInfo{};
		tesselationInfo.patchControlPoints = 3;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicCreateInfo;
		//pipelineInfo.pTessellationState = &tesselationInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = reinterpret_cast<VulkanAPI*>(p_Owner)->getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.pDepthStencilState = &_depthStencil;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
			return false;

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = shaders[5];
		compShaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo computeInfo{};
		computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computeInfo.layout = computeLayout;
		computeInfo.stage = compShaderStageInfo;

		vkCreateComputePipelines(logicalDevice, nullptr, 1, &computeInfo, nullptr, &computePipeline);

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[2], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[3], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[4], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[5], nullptr);

		return true;
	}

	bool VulkanTerrain::createDescriptorPool()
	{
		VkDescriptorPoolSize descriptorTypePool{};
		descriptorTypePool.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorTypePool.descriptorCount = 1;

		VkDescriptorPoolSize descriptorTypePool2{};
		descriptorTypePool2.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorTypePool2.descriptorCount = 1;

		VkDescriptorPoolSize descriptorTypePool3{};
		descriptorTypePool3.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorTypePool3.descriptorCount = 2;

		VkDescriptorPoolSize descriptorTypePool4{};
		descriptorTypePool4.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorTypePool4.descriptorCount = 1;


		std::vector<VkDescriptorPoolSize> poolSizes = { descriptorTypePool , descriptorTypePool2 };
		std::vector<VkDescriptorPoolSize> poolSizes2 = { descriptorTypePool3 , descriptorTypePool4 };
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkDescriptorPoolCreateInfo createInfo2{};
		createInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo2.pNext = nullptr;
		createInfo2.maxSets = 1;
		createInfo2.poolSizeCount = poolSizes2.size();
		createInfo2.pPoolSizes = poolSizes2.data();
		createInfo2.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		vkCreateDescriptorPool(logicalDevice, &createInfo, NULL, &descriptorSets[0].descriptorPool) == VK_SUCCESS;
		return vkCreateDescriptorPool(logicalDevice, &createInfo2, NULL, &descriptorSets[1].descriptorPool) == VK_SUCCESS;
	}

	bool VulkanTerrain::createDescriptorLayout()
	{
		VkDescriptorSetLayoutBinding descriptorBindings{};
		descriptorBindings.binding = 1; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorBindings.descriptorCount = 1;
		descriptorBindings.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptorBindings.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutBinding descriptorBindings3{};
		descriptorBindings3.binding = 0; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorBindings3.descriptorCount = 1;
		descriptorBindings3.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		descriptorBindings3.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutBinding descriptorBindings4{};
		descriptorBindings4.binding = 1; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings4.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorBindings4.descriptorCount = 1;
		descriptorBindings4.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		descriptorBindings4.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutBinding descriptorBindings5{};
		descriptorBindings5.binding = 2; // DESCRIPTOR_SET_BINDING_INDEX
		descriptorBindings5.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorBindings5.descriptorCount = 1;
		descriptorBindings5.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		descriptorBindings5.pImmutableSamplers = NULL;



		std::vector<VkDescriptorSetLayoutBinding> bindings = { descriptorBindings };
		std::vector<VkDescriptorSetLayoutBinding> bindings2 = { descriptorBindings3, descriptorBindings4, descriptorBindings5 };

		VkDescriptorSetLayoutCreateInfo descriptorLayout{};
		descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout.pNext = NULL;
		descriptorLayout.bindingCount = bindings.size();
		descriptorLayout.pBindings = bindings.data();

		VkDescriptorSetLayoutCreateInfo descriptorLayout2{};
		descriptorLayout2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayout2.pNext = NULL;
		descriptorLayout2.bindingCount = bindings2.size();
		descriptorLayout2.pBindings = bindings2.data();

		vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout, NULL, &descriptorSets[0].descriptorSetLayout) == VK_SUCCESS;
		return vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayout2, NULL, &descriptorSets[1].descriptorSetLayout) == VK_SUCCESS;
	}

	bool VulkanTerrain::createDescriptorSet()
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.pNext = NULL;
		descriptorAllocInfo.descriptorPool = descriptorSets[0].descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &descriptorSets[0].descriptorSetLayout;

		vkAllocateDescriptorSets(logicalDevice, &descriptorAllocInfo, &descriptorSets[0].set[0]);

		VkDescriptorSetAllocateInfo descriptorAllocInfo2{};
		descriptorAllocInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo2.pNext = NULL;
		descriptorAllocInfo2.descriptorPool = descriptorSets[1].descriptorPool;
		descriptorAllocInfo2.descriptorSetCount = 1;
		descriptorAllocInfo2.pSetLayouts = &descriptorSets[1].descriptorSetLayout;

		vkAllocateDescriptorSets(logicalDevice, &descriptorAllocInfo2, &descriptorSets[1].set[0]);

		std::vector<VkDescriptorImageInfo> imageInfo;
		imageInfo.resize(2);
		imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[0].imageView = object_texture->textureImageView;
		imageInfo[0].sampler = object_texture->textureSampler;

		imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[1].imageView = object_texture->textureImageView;
		imageInfo[1].sampler = object_texture->textureSampler;

		std::vector<VkDescriptorBufferInfo> bufferInfo;
		bufferInfo.resize(2);

		bufferInfo[0].buffer = terIn.Buffer;
		bufferInfo[0].offset = 0;
		bufferInfo[0].range = sizeof(ComputeSize);

		bufferInfo[1].buffer = terOut.Buffer;
		bufferInfo[1].offset = 0;
		bufferInfo[1].range = size.resolution * size.resolution * sizeof(ComputeVertex);


		std::vector<VkWriteDescriptorSet> writes{};
		//memset(&writes, 0, sizeof(writes));

		writes.resize(4);
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSets[0].set[0];
		writes[0].dstBinding = 1;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorCount = 1;
		writes[0].pImageInfo = &imageInfo[0];


		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSets[1].set[0];
		writes[1].dstBinding = 0;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[1].descriptorCount = 1;
		writes[1].pBufferInfo = &bufferInfo[0];

		writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet = descriptorSets[1].set[0];
		writes[2].dstBinding = 1;
		writes[2].dstArrayElement = 0;
		writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[2].descriptorCount = 1;
		writes[2].pBufferInfo = &bufferInfo[1];

		writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[3].dstSet = descriptorSets[1].set[0];
		writes[3].dstBinding = 2;
		writes[3].dstArrayElement = 0;
		writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[3].descriptorCount = 1;
		writes[3].pImageInfo = &imageInfo[1];

		vkUpdateDescriptorSets(logicalDevice, writes.size(), writes.data(), 0, NULL);

		return true;
	}
};