#include "pch.h"
#include "VulkanTerrain.h"
#include "VulkanRenderer.h"
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
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		physComponent = new GrEngineBullet::BulletAPI::BulletPhysObject(this);

		GrEngine::Engine::GetContext()->GetPhysics()->AddSimulationObject(physComponent);
	}

	void VulkanTerrain::destroyObject()
	{
		if (ready)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terIn);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terOut);
		}
		resources->RemoveTexture(heightMap->texture_collection, logicalDevice, memAllocator);
		resources->RemoveTexture(foliageMask->texture_collection, logicalDevice, memAllocator);
		VulkanAPI::DestroyPipeline(computePipeline);
		VulkanAPI::DestroyPipelineLayout(computeLayout);
		GrEngine::Engine::GetContext()->GetPhysics()->RemoveSimulationObject(physComponent);

		ready = false;
		VulkanDrawable::destroyObject();
	}

	void VulkanTerrain::UpdateFoliageMask(void* pixels)
	{
		static_cast<VulkanRenderer*>(p_Owner)->updateResource(foliageMask, pixels);
		VulkanAPI::DestroyPipeline(computePipeline);
		VulkanAPI::DestroyPipelineLayout(computeLayout);
		VulkanAPI::DestroyPipeline(graphicsPipeline);
		VulkanAPI::DestroyPipelineLayout(pipelineLayout);
		updateObject();
	}

	void VulkanTerrain::GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images)
	{
		if (ready)
		{
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->vertexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &object_mesh->indexBuffer);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terIn);
			VulkanAPI::m_destroyShaderBuffer(logicalDevice, memAllocator, &terOut);
			VulkanAPI::DestroyImage(heightMap->newImage.allocatedImage);
			VulkanAPI::DestroyImage(foliageMask->newImage.allocatedImage);
			VulkanAPI::DestroyPipeline(computePipeline);
			VulkanAPI::DestroyPipelineLayout(computeLayout);

			ready = false;
		}

		size = { resolution, width, depth, height };

		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, &size, sizeof(ComputeSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terIn);
		VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, nullptr, size.resolution * size.resolution * sizeof(ComputeVertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &terOut);

		heightMap = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[0] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY)->AddLink();
		foliageMask = static_cast<VulkanRenderer*>(p_Owner)->loadTexture({ images[1] }, VK_IMAGE_VIEW_TYPE_2D_ARRAY)->AddLink();
		bool res = static_cast<VulkanRenderer*>(p_Owner)->assignTextures({ images[2], images[3], images[4], images[5] }, this);

		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = static_cast<VulkanRenderer*>(p_Owner)->compute_bit.value();
		VulkanAPI::CreateCommandPool(logicalDevice, static_cast<VulkanRenderer*>(p_Owner)->compute_bit.value(), &computePool);
		VulkanAPI::AllocateCommandBuffers(logicalDevice, computePool, &computeCmd, 1);
		VulkanAPI::BeginCommandBuffer(computeCmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkCmdBindPipeline(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
		vkCmdBindDescriptorSets(computeCmd, VK_PIPELINE_BIND_POINT_COMPUTE, computeLayout, 0, 1, &descriptorSets[1].set, 0, nullptr);
		vkCmdDispatch(computeCmd, 10, 1, 1);
		VulkanAPI::CreateVkFence(logicalDevice, &computeFence);
		VulkanAPI::GetDeviceQueue(logicalDevice, static_cast<VulkanRenderer*>(p_Owner)->compute_bit.value(), &computeQueue);
		VulkanAPI::EndAndSubmitCommandBuffer(logicalDevice, computePool, computeCmd, computeQueue, computeFence);

		std::vector<ComputeVertex> vert;
		vert.resize(resolution * resolution);
		vmaMapMemory(memAllocator, terOut.Allocation, (void**)&terI);
		memcpy_s(vert.data(), resolution * resolution * sizeof(ComputeVertex), terI, resolution * resolution * sizeof(ComputeVertex));
		vmaUnmapMemory(memAllocator, terOut.Allocation);

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

		VulkanAPI::DestroyCommandPool(computePool);
		VulkanAPI::DestroyFence(computeFence);

		ready = true;
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

		bool res = VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant }, { descriptorSets[0].descriptorSetLayout }, &pipelineLayout);
		res = VulkanAPI::CreatePipelineLayout(logicalDevice, {}, { descriptorSets[1].descriptorSetLayout }, &computeLayout) & res;
		return res;
	}

	bool VulkanTerrain::createGraphicsPipeline()
	{
		std::string solution_path = GrEngine::Globals::getExecutablePath();

		std::vector<char> vertShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_vert.spv");
		std::vector<char> geomShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_geom.spv");
		std::vector<char> fragShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_frag.spv");
		std::vector<char> compShaderCode = GrEngine::Globals::readFile(solution_path + shader_path + "_comp.spv");

		VkShaderModule shaders[4] = { VulkanAPI::m_createShaderModule(logicalDevice, vertShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, geomShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, fragShaderCode),
			VulkanAPI::m_createShaderModule(logicalDevice, compShaderCode)
		};

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
		//rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = dynamic_cast<VulkanRenderer*>(p_Owner)->GetSampling();
		multisampling.minSampleShading = 1.0f;
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
		pipelineInfo.renderPass = reinterpret_cast<VulkanRenderer*>(p_Owner)->getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.pDepthStencilState = &_depthStencil;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		if (VulkanAPI::CreateGraphicsPipeline(logicalDevice, &pipelineInfo, &graphicsPipeline) != true)
			return false;

		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = shaders[3];
		compShaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo computeInfo{};
		computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computeInfo.layout = computeLayout;
		computeInfo.stage = compShaderStageInfo;

		if (VulkanAPI::CreateComputePipeline(logicalDevice, &computeInfo, &computePipeline) != true)
			return false;

		vkDestroyShaderModule(logicalDevice, shaders[0], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[1], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[2], nullptr);
		vkDestroyShaderModule(logicalDevice, shaders[3], nullptr);

		return true;
	}

	void VulkanTerrain::populateDescriptorSets()
	{
		descriptorSets.clear();
		descriptorSets.resize(2);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptorSets[1].bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

		VkDescriptorImageInfo texInfo{};
		texInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texInfo.imageView = object_texture->textureImageView;
		texInfo.sampler = object_texture->textureSampler;
		VkDescriptorImageInfo heightInfo{};
		heightInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		heightInfo.imageView = heightMap->textureImageView;
		heightInfo.sampler = heightMap->textureSampler;
		VkDescriptorImageInfo maskInfo{};
		maskInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		maskInfo.imageView = foliageMask->textureImageView;
		maskInfo.sampler = foliageMask->textureSampler;

		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texInfo);
		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maskInfo);

		int binding = 2;
		for (auto buffer : globalBuffers)
		{
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = buffer.second->Buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(VulkanRenderer::PickingInfo);
			subscribeDescriptor(buffer.first, binding++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bufferInfo);
		}

		VkDescriptorBufferInfo inBuffer{};
		inBuffer.buffer = terIn.Buffer;
		inBuffer.offset = 0;
		inBuffer.range = sizeof(ComputeSize);
		VkDescriptorBufferInfo outBuffer{};
		outBuffer.buffer = terOut.Buffer;
		outBuffer.offset = 0;
		outBuffer.range = size.resolution * size.resolution * sizeof(ComputeVertex);
		subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, inBuffer, 1);
		subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, outBuffer, 1);
		subscribeDescriptor(VK_SHADER_STAGE_COMPUTE_BIT, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, heightInfo, 1);

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}
};