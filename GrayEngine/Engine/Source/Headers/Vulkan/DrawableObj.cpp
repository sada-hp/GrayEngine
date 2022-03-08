#include "pch.h"
#include "DrawableObj.h"

DrawableObj::DrawableObj()
{

}

DrawableObj::~DrawableObj()
{

}

void DrawableObj::initObject(VkDevice device)
{
	logicalDevice = device;
	createDescriptorLayout(logicalDevice);
	createDescriptorPool(logicalDevice);
	createPipelineLayout(logicalDevice);
}

void DrawableObj::destroyObject()
{
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);
	vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);
	for (auto layout : setLayout)
	{
		vkDestroyDescriptorSetLayout(logicalDevice, layout, NULL);
	}

	this->~DrawableObj();
}

bool DrawableObj::createDescriptorLayout(VkDevice device)
{
	VkDescriptorSetLayoutBinding descriptorBindings;
	descriptorBindings.binding = 0; // DESCRIPTOR_SET_BINDING_INDEX
	descriptorBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorBindings.descriptorCount = 1;
	descriptorBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorBindings.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = 1;
	descriptorLayout.pBindings = &descriptorBindings;

	setLayout.resize(descriptorLayout.bindingCount);

	return vkCreateDescriptorSetLayout(device, &descriptorLayout, NULL, setLayout.data()) == VK_SUCCESS;
}

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

bool DrawableObj::createPipelineLayout(VkDevice device)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = setLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	return vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout) == VK_SUCCESS;
}