/** \file descriptors.cpp */

#include "core/descriptors.hpp"

DescriptorManager::~DescriptorManager()
{
	freeDescriptors();
	vkDestroyDescriptorSetLayout(m_device->getDevice(), m_graphicsDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device->getDevice(), m_computeDescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_device->getDevice(), m_descriptorPool, nullptr);
	m_device = nullptr;
}

void DescriptorManager::addPoolSize(VkDescriptorType type, uint32_t count)
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = type;
	poolSize.descriptorCount = count;
	m_poolSizes.push_back(poolSize);
}

void DescriptorManager::buildDescriptorPool()
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
	poolInfo.pPoolSizes = m_poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(m_maxSets);
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(m_device->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool.");
}

void DescriptorManager::allocateDescriptorSets(bool compute)
{
	if (compute)
	{
		std::vector<VkDescriptorSetLayout> layouts(m_maxSets, m_computeDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(m_maxSets);
		allocInfo.pSetLayouts = layouts.data();

		m_computeDescriptorSets.resize(m_maxSets);
		if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, m_computeDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets.");
	}
	else
	{
		std::vector<VkDescriptorSetLayout> layouts(m_maxSets, m_graphicsDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(m_maxSets);
		allocInfo.pSetLayouts = layouts.data();

		m_graphicsDescriptorSets.resize(m_maxSets);
		if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, m_graphicsDescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets.");
	}
}

void DescriptorManager::freeDescriptors() const
{
	if (m_graphicsDescriptorSets.size() != 0)
		vkFreeDescriptorSets(m_device->getDevice(), m_descriptorPool, static_cast<uint32_t>(m_graphicsDescriptorSets.size()), m_graphicsDescriptorSets.data());
	if (m_computeDescriptorSets.size() != 0)
		vkFreeDescriptorSets(m_device->getDevice(), m_descriptorPool, static_cast<uint32_t>(m_computeDescriptorSets.size()), m_computeDescriptorSets.data());
}

void DescriptorManager::resetDescriptorPool()
{
	vkResetDescriptorPool(m_device->getDevice(), m_descriptorPool, 0);
}

void DescriptorManager::addBinding(bool compute, uint32_t index, VkDescriptorType type, VkShaderStageFlags flags, uint32_t count, VkSampler* samplers)
{
	if (bindingFound(compute, index))
		throw std::runtime_error("Binding already in use.");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = index;
	layoutBinding.descriptorCount = count;
	layoutBinding.descriptorType = type;
	layoutBinding.stageFlags = flags;
	layoutBinding.pImmutableSamplers = samplers;
	if (compute)
		m_computeBindings.push_back(layoutBinding);
	else
		m_graphicsBindings.push_back(layoutBinding);
}

bool DescriptorManager::bindingFound(bool compute, uint32_t index)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	if (compute)
		bindings = m_computeBindings;
	else
		bindings = m_graphicsBindings;

	for (auto& binding : bindings)
	{
		if (binding.binding == index)
			return true;
	}
	return false;
}

void DescriptorManager::buildDescriptorSetLayout(bool compute)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	if (compute)
	{
		layoutInfo.bindingCount = static_cast<uint32_t>(m_computeBindings.size());
		layoutInfo.pBindings = m_computeBindings.data();

		if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_computeDescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics descriptor set layout.");
	}
	else
	{
		layoutInfo.bindingCount = static_cast<uint32_t>(m_graphicsBindings.size());
		layoutInfo.pBindings = m_graphicsBindings.data();

		if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_graphicsDescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics descriptor set layout.");
	}
}

void DescriptorManager::addWriteDescriptorSet(bool compute, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo)
{
	if (!bindingFound(compute, binding))
		throw std::runtime_error("Layout doesn't contain specified binding.");
	VkDescriptorSetLayoutBinding bindingDescription = getDescriptorSetLayoutBinding(compute, binding);
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = bindingDescription.descriptorType;
	write.descriptorCount = 1;
	if (bufferInfo != nullptr)
		write.pBufferInfo = bufferInfo;
	if (imageInfo != nullptr)
		write.pImageInfo = imageInfo;
	if (compute)
		m_computeDescriptorWrites.push_back(write);
	else	
		m_graphicsDescriptorWrites.push_back(write);
}

void DescriptorManager::overwrite(bool compute, uint32_t index)
{
	std::vector<VkWriteDescriptorSet> writes;
	if (compute)
	{
		for (auto& write : m_computeDescriptorWrites)
		{
			write.dstSet = m_computeDescriptorSets[index];
			writes.push_back(write);
		}
	}
	else
	{
		for (auto& write : m_graphicsDescriptorWrites)
		{
			write.dstSet = m_graphicsDescriptorSets[index];
			writes.push_back(write);
		}
	}

	vkUpdateDescriptorSets(m_device->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
