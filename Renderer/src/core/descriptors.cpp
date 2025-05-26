/** \file descriptors.cpp */

#include "core/descriptors.hpp"

DescriptorManager::~DescriptorManager()
{
	freeDescriptors();
	vkDestroyDescriptorSetLayout(m_device->getDevice(), m_descriptorSetLayout, nullptr);
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
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
	poolInfo.pPoolSizes = m_poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(m_maxSets * 2);

	if (vkCreateDescriptorPool(m_device->getDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool.");
}

void DescriptorManager::allocateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(m_maxSets, m_descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_maxSets);
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(m_maxSets);
	if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets.");
}

void DescriptorManager::freeDescriptors() const
{
	if (m_descriptorSets.size() == 0) return;
	vkFreeDescriptorSets(m_device->getDevice(), m_descriptorPool, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data());
}

void DescriptorManager::resetDescriptorPool()
{
	vkResetDescriptorPool(m_device->getDevice(), m_descriptorPool, 0);
}

void DescriptorManager::addBinding(uint32_t index, VkDescriptorType type, VkShaderStageFlags flags, uint32_t count, VkSampler* samplers)
{
	if (bindingFound(index))
		throw std::runtime_error("Binding already in use.");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = index;
	layoutBinding.descriptorCount = count;
	layoutBinding.descriptorType = type;
	layoutBinding.stageFlags = flags;
	layoutBinding.pImmutableSamplers = samplers;
	m_bindings.push_back(layoutBinding);
}

bool DescriptorManager::bindingFound(uint32_t index)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings = m_bindings;

	for (auto& binding : bindings)
	{
		if (binding.binding == index)
			return true;
	}
	return false;
}

void DescriptorManager::buildDescriptorSetLayout()
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
	layoutInfo.pBindings = m_bindings.data();

	if (vkCreateDescriptorSetLayout(m_device->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout.");
}

void DescriptorManager::addWriteDescriptorSet(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo)
{
	if (!bindingFound(binding))
		throw std::runtime_error("Layout doesn't contain specified binding.");
	VkDescriptorSetLayoutBinding bindingDescription = getDescriptorSetLayoutBinding(binding);
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
	m_descriptorWrites.push_back(write);
}

void DescriptorManager::overwrite(uint32_t index)
{
	std::vector<VkWriteDescriptorSet> writes;
	for (auto& write : m_descriptorWrites)
	{
		write.dstSet = m_descriptorSets[index];
		writes.push_back(write);
	}

	vkUpdateDescriptorSets(m_device->getDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
