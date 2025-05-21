/** \file descriptors.hpp */

#pragma once

#include "core/device.hpp"

/* \class DescriptorManager
*  \brief stores the descriptor pool, descriptor set layouts and descriptor sets
*/
class DescriptorManager
{
public:
	DescriptorManager(Device* device, uint32_t maxSets) : m_device(device), m_maxSets(maxSets) {} //!< constructor
	~DescriptorManager(); //!< destructor

	DescriptorManager(const DescriptorManager&) = delete; //!< copy constructor
	DescriptorManager& operator=(const DescriptorManager&) = delete; //!< copy assignment
public:
	VkDescriptorPool getDescriptorPool() { return m_descriptorPool; } //!< returns the descriptor pool
	VkDescriptorSetLayout* getDescriptorSetLayout() { return &m_descriptorSetLayout; } //!< returns a pointer to the descriptor set layout
	VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding(uint32_t index) { return m_bindings[index]; } //!< returns the descriptor set layout binding for the input index
	std::vector<VkDescriptorSet> getDescriptorSets() { return m_descriptorSets; } //!< returns the descriptor sets
	/// descriptor pool methods
	void addPoolSize(VkDescriptorType type, uint32_t count); //!< adds a VkDescriptorPoolSize to the pool sizes
	void buildDescriptorPool(); //!< creates the descriptor pool using m_poolSizes
	void allocateDescriptorSets(); //!< allocates descriptor sets to the descriptor pool
	void freeDescriptors() const; //!< frees the descriptors on the destruction of the DescriptorManager
	void resetDescriptorPool(); //!< resets the descriptor pool
	/// descriptor set layout methods
	void addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags flags, uint32_t count = 1, VkSampler* samplers = nullptr); //!< adds a descriptor set layout binding to the vector
	bool bindingFound(uint32_t index); //!< checks if a binding is already being used
	void buildDescriptorSetLayout(); //!< creates the descriptor set layout
	/// write descriptor set methods
	void addWriteDescriptorSet(uint32_t binding, VkDescriptorBufferInfo* bufferInfo = nullptr, VkDescriptorImageInfo* imageInfo = nullptr); //!< adds a write descriptor set to the vector
	void overwrite(uint32_t index); //!< updates the data in the descriptor set
private:
	Device* m_device; //!< device object pointer
	VkDescriptorPool m_descriptorPool; //!< handle to the descriptor pool
	std::vector<VkDescriptorPoolSize> m_poolSizes; //!< vector of the pool sizes
	VkDescriptorSetLayout m_descriptorSetLayout; //!< descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> m_bindings; //!< bindings for the descriptor set layout
	std::vector<VkDescriptorSet> m_descriptorSets; //!< descriptor sets
	std::vector<VkWriteDescriptorSet> m_descriptorWrites; //!< writes for the descriptor set
	uint32_t m_maxSets; //< max number of sets for each vector of descriptor sets
};
