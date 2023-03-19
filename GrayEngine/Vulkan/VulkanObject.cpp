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
		properties.push_back(new Transparency(0, this));
		properties.push_back(new DoubleSided(0, this));
		properties.push_back(new Shader("Shaders//default", this));

		UINT id = GetEntityID();

		physComponent = new GrEngineBullet::BulletAPI::BulletPhysObject(this);
		GrEngine::Engine::GetContext()->GetPhysics()->AddSimulationObject(physComponent);

		p_Owner = owner;
		resources = &static_cast<VulkanRenderer*>(owner)->GetResourceManager();
		logicalDevice = device;
		memAllocator = allocator;

		GenerateBoxMesh(0.5, 0.5, 0.5);
		updateSelectionPipeline();

		static_cast<VulkanRenderer*>(p_Owner)->assignTextures({""}, this);
	}

	void VulkanObject::destroyObject()
	{
		GrEngine::Engine::GetContext()->GetPhysics()->RemoveSimulationObject(physComponent);
		VulkanAPI::DestroyPipeline(selectionPipeline);
		VulkanAPI::DestroyPipelineLayout(selectionLayout);

		VulkanDrawable::destroyObject();
	}

	void VulkanObject::Refresh()
	{
		updateObject();
		updateSelectionPipeline();
	}

	void VulkanObject::recordSelection(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		if (object_mesh != nullptr && object_mesh->vertexBuffer.initialized == true && selectable)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectionPipeline);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &object_mesh->vertexBuffer.Buffer, offsets);
			vkCmdBindIndexBuffer(cmd, object_mesh->indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			//UpdateObjectPosition();
			//UpdateObjectOrientation();
			pushConstants(cmd, extent, mode);

			//vkCmdDraw(commandBuffer, static_cast<uint32_t>(object_mesh.vertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(object_mesh->indices.size()), 1, 0, 0, 0);
		}
	}

	bool VulkanObject::pushConstants(VkCommandBuffer cmd, VkExtent2D extent, UINT32 mode)
	{
		/*orientation relative to the position in a 3D space (?)*/
		ubo.model = GetObjectTransformation();
		/*Math for Game Programmers: Understanding Homogeneous Coordinates GDC 2015*/
		ubo.view = glm::translate(glm::mat4_cast(p_Owner->getActiveViewport()->UpdateCameraOrientation(0.2)), -p_Owner->getActiveViewport()->UpdateCameraPosition(0.65)); // [ix iy iz w1( = 0)]-direction [jx jy jz w2( = 0)]-direction [kx ky kz w3( = 0)]-direction [tx ty tz w ( = 1)]-position
		ubo.proj = glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, near_plane, far_plane); //fov, aspect ratio, near clipping plane, far clipping plane
		ubo.proj[1][1] *= -1; //reverse Y coordinate
		ubo.scale = GetPropertyValue("Scale", glm::vec3(1.f));
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);

		opo.object_id = GetEntityID();
		opo.colors = GetPropertyValue("Color", glm::vec4(1));
		if (opo.object_id == selected_id && IsStatic() == false)
		{
			opo.colors *= glm::vec4(0.5, 0.75, 2, opo.colors.w);
		}

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(UniformBufferObject), sizeof(PickingBufferObject), &opo);
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
		pushConstant.size = sizeof(UniformBufferObject);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPushConstantRange pushConstant2;
		pushConstant2.offset = sizeof(UniformBufferObject);
		pushConstant2.size = sizeof(PickingBufferObject);
		pushConstant2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


		VulkanAPI::CreatePipelineLayout(logicalDevice, { pushConstant, pushConstant2 }, {}, &selectionLayout);


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

	bool VulkanObject::createGraphicsPipeline()
	{
		shader_path = GetPropertyValue("Shader", std::string(shader_path));
		transparency = GetPropertyValue("Transparency", 0);
		double_sided = GetPropertyValue("DoubleSided", 0);

		return VulkanDrawable::createGraphicsPipeline();
	}

	void VulkanObject::populateDescriptorSets()
	{
		transparency = GetPropertyValue("Transparency", 0);

		descriptorSets.clear();
		descriptorSets.resize(1);
		descriptorSets[0].bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, object_texture->texInfo.descriptor);

		int index = 1;

		if (transparency > 0)
		{
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, static_cast<VulkanRenderer*>(p_Owner)->position.texInfo.descriptor);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->transBuffer.BufferInfo);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<VulkanRenderer*>(p_Owner)->headIndex.texInfo.descriptor);
			subscribeDescriptor(VK_SHADER_STAGE_FRAGMENT_BIT, index++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<VulkanRenderer*>(p_Owner)->nodeBfffer.BufferInfo);
		}

		createDescriptorLayout();
		createDescriptorPool();
		createDescriptorSet();
	}

	void VulkanObject::updateCollisions()
	{
		if (colMesh != nullptr)
		{
			delete colMesh;
		}

		colMesh = new btTriangleMesh();
		for (std::vector<Vertex>::iterator itt = object_mesh->vertices.begin(); itt != object_mesh->vertices.end(); ++itt)
		{
			colMesh->findOrAddVertex(btVector3((*itt).pos.x, (*itt).pos.y, (*itt).pos.z), false);
		}

		for (std::vector<uint32_t>::iterator itt = object_mesh->indices.begin(); itt != object_mesh->indices.end(); ++itt)
		{
			colMesh->addTriangleIndices(*(++itt), *(++itt), *itt);
		}

		physComponent->UpdateCollisionShape(new btBvhTriangleMeshShape(colMesh, false));
	}

	bool VulkanObject::LoadModel(const char* model_path)
	{
		auto start = std::chrono::steady_clock::now();

		std::string mesh_path = "";
		std::vector<std::string> textures_vector;

		if (!GrEngine::Globals::readGMF(model_path, &mesh_path, &textures_vector)) 
			return false;

		LoadModel(mesh_path.c_str(), textures_vector);

		auto end = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		Logger::Out("Model %s loaded in %d ms", OutputColor::Gray, OutputType::Log, model_path, (int)time);

		return true;
	}

	bool VulkanObject::LoadModel(const char* mesh_path, std::vector<std::string> textures_vector)
	{
		VulkanObject* ref_obj = this;

		VulkanRenderer* inst = static_cast<VulkanRenderer*>(p_Owner);
		std::vector<std::string>* out_materials_collection = &material_names;
		std::map<int, std::future<void>> processes_map;
		std::map<std::string, std::vector<int>> materials_map;

		material_names.clear();
		texture_names.clear();

		processes_map[processes_map.size()] = std::async(std::launch::async, [textures_vector, ref_obj, inst]()
			{
				inst->assignTextures(textures_vector, ref_obj);
			});

		processes_map[processes_map.size()] = std::async(std::launch::async, [mesh_path, textures_vector, ref_obj, out_materials_collection, inst]()
			{
				ref_obj->LoadMesh(mesh_path, out_materials_collection);
			});

		for (int ind = 0; ind < processes_map.size(); ind++)
		{
			if (processes_map[ind].valid())
			{
				processes_map[ind].wait();
			}
		}

		texture_names = textures_vector;
		updateObject();

		return true;
	}

	bool VulkanObject::LoadMesh(const char* mesh_path, std::vector<std::string>* out_materials)
	{
		auto resource = resources->GetMeshResource(mesh_path);
		std::string solution = GrEngine::Globals::getExecutablePath();

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh->mesh_path.c_str(), logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		if (resource == nullptr)
		{
			std::unordered_map<Vertex, uint32_t> uniqueVertices{};
			Assimp::Importer importer;
			Mesh* target_mesh = new Mesh();

			auto model = importer.ReadFile(solution + mesh_path, 0);

			if (model == NULL)
			{
				Logger::Out("Could not load the mesh %c%s%c!", OutputColor::Red, OutputType::Error, '"', mesh_path, '"');
				return false;
			}

			target_mesh->mesh_path = mesh_path;

			if (colMesh != nullptr)
			{
				delete colMesh;
				colMesh = nullptr;
			}

			colMesh = new btTriangleMesh();

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

					GrEngine_Vulkan::Vertex vertex{ {
					{ cur_mesh->mVertices[vert_ind].x, cur_mesh->mVertices[vert_ind].y, cur_mesh->mVertices[vert_ind].z, 1.0f },
					{ cur_mesh->mNormals[vert_ind].x, cur_mesh->mNormals[vert_ind].y, cur_mesh->mNormals[vert_ind].z, 1.0f },
					{ coord[vert_ind].x, 1.0f - coord[vert_ind].y },
					(uint32_t)uv_ind
					} };


					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(target_mesh->vertices.size());
						target_mesh->vertices.push_back(vertex);
						target_mesh->collisions.findOrAddVertex(btVector3(vertex.pos.x, vertex.pos.y, vertex.pos.z), false);
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

				aiString material;
				aiGetMaterialString(model->mMaterials[model->mMeshes[mesh_ind]->mMaterialIndex], AI_MATKEY_NAME, &material);

				if (out_materials)
				{
					out_materials->push_back(material.C_Str());
				}
			}

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);

			target_mesh->bounds = glm::uvec3(highest_pointx, highest_pointy, highest_pointz);
			bound = target_mesh->bounds;

			for (std::vector<uint32_t>::iterator itt = target_mesh->indices.begin(); itt != target_mesh->indices.end(); ++itt)
			{
				target_mesh->collisions.addTriangleIndices(*(++itt), *(++itt), *itt);
			}

			auto resource = resources->AddMeshResource(mesh_path, target_mesh);
			object_mesh = resource->AddLink();
			physComponent->UpdateCollisionShape(new btBvhTriangleMeshShape(&object_mesh->collisions, false));
		}
		else
		{
			object_mesh = resource->AddLink();
			bound = object_mesh->bounds;
			physComponent->UpdateCollisionShape(new btBvhTriangleMeshShape(&object_mesh->collisions, false));
		}
	}

	void VulkanObject::GeneratePlaneMesh(float width, int subdivisions)
	{
		subdivisions = 1+subdivisions;
		std::string res_name = std::string("plane") + std::to_string(width) + std::to_string(subdivisions);
		auto resource = resources->GetMeshResource(res_name.c_str());

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh->mesh_path.c_str(), logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		if (resource == nullptr)
		{
			std::unordered_map<Vertex, uint32_t> uniqueVertices{};
			Mesh* target_mesh = new Mesh();
			double divisions = width / subdivisions;

			if (colMesh != nullptr)
			{
				delete colMesh;
				colMesh = nullptr;
			}

			colMesh = new btTriangleMesh();

			for (int row = 0; row < subdivisions; row++)
			{
				for (int col = 0; col < subdivisions; col++)
				{
					Vertex vertex{};
					vertex.uv = glm::vec2(divisions * col, 1.0f - divisions * row);
					vertex.pos = glm::vec4(width * (divisions * col - width / 2), 0, width * (divisions * row - width / 2), 1.0f);
					target_mesh->vertices.push_back(vertex);
					target_mesh->collisions.findOrAddVertex(btVector3(vertex.pos.x, vertex.pos.y, vertex.pos.z), false);

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

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);

			for (std::vector<uint32_t>::iterator itt = target_mesh->indices.begin(); itt != target_mesh->indices.end(); ++itt)
			{
				target_mesh->collisions.addTriangleIndices(*(++itt), *(++itt), *itt);
			}

			auto resource = resources->AddMeshResource(res_name.c_str(), target_mesh);
			object_mesh = resource->AddLink();
			physComponent->UpdateCollisionShape(new btBvhTriangleMeshShape(&object_mesh->collisions, false));
		}
		else
		{
			object_mesh = resource->AddLink();
			physComponent->UpdateCollisionShape(new btBvhTriangleMeshShape(&object_mesh->collisions, false));
		}
	}

	void VulkanObject::GenerateBoxMesh(float width, float height, float depth)
	{
		std::string res_name = std::string("box") + std::to_string(width) + std::to_string(height) + std::to_string(depth);
		auto resource = resources->GetMeshResource(res_name.c_str());

		if (object_mesh != nullptr)
		{
			resources->RemoveMesh(object_mesh->mesh_path.c_str(), logicalDevice, memAllocator);
			object_mesh = nullptr;
		}

		float ycoord = height / 2;
		float xcoord = width / 2;
		float zcoord = depth / 2;

		if (resource == nullptr)
		{
			Mesh* target_mesh = new Mesh();

			target_mesh->vertices = {
				{{{ xcoord, ycoord, zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 1.0f }}},
				{{{ -xcoord, ycoord, -zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 0.0f }}},
				{{{ -xcoord, ycoord , zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 1.f }}},
				{{{ xcoord, ycoord, -zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 0.0f }}},

				{{{ xcoord, -ycoord, zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 1.0f }}},
				{{{ -xcoord, -ycoord, -zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 0.0f }}},
				{{{ -xcoord, -ycoord, zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 0.f, 1.f }}},
				{{{ xcoord, -ycoord, -zcoord, 1.0f },{  0.0f,  0.0f,  0.0f, 1.0f },{ 1.f, 0.0f }}}
			};

			target_mesh->indices = { 0, 1, 2, 0, 3, 1,
				6, 5, 4, 5, 7, 4,
				0, 2, 6, 6, 4, 0,
				4, 3, 0, 4, 7, 3,
				1, 3, 7, 7, 5, 1,
				6, 2, 1, 1, 5, 6,
			};

			resource = resources->AddMeshResource("default", target_mesh);
			object_mesh = resource->AddLink();

			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->vertices.data(), sizeof(target_mesh->vertices[0]) * target_mesh->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &target_mesh->vertexBuffer);
			VulkanAPI::m_createVkBuffer(logicalDevice, memAllocator, target_mesh->indices.data(), sizeof(target_mesh->indices[0]) * target_mesh->indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &target_mesh->indexBuffer);
			physComponent->UpdateCollisionShape(new btBoxShape(btVector3(xcoord, ycoord, zcoord)));
		}
		else
		{
			object_mesh = resource->AddLink();
			physComponent->UpdateCollisionShape(new btBoxShape(btVector3(xcoord, ycoord, zcoord)));
		}
	}
};