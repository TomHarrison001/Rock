#pragma once

#include "core/device.hpp"
#include "utils/resources.hpp"

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
	VkDescriptorSetLayout* getGraphicsDescriptorSetLayout() { return &m_graphicsDescriptorSetLayout; } //!< returns a pointer to the graphics descriptor set layout
	VkDescriptorSetLayout* getComputeDescriptorSetLayout() { return &m_computeDescriptorSetLayout; } //!< returns a pointer to the compute descriptor set layout
	VkDescriptorSetLayoutBinding getDescriptorSetLayoutBinding(Stage stage, uint32_t index) { return (stage == Stage::GRAPHICS) ? m_graphicsBindings[index] : m_computeBindings[index]; } //!< returns the descriptor set layout binding for the input stage and index
	std::vector<VkDescriptorSet> getDescriptorSets(Stage stage) { return (stage == Stage::GRAPHICS) ? m_graphicsDescriptorSets : m_computeDescriptorSets; } //!< returns the descriptor sets for the input stage
	/// descriptor pool methods
	void addPoolSize(VkDescriptorType type, uint32_t count); //!< adds a VkDescriptorPoolSize to the pool sizes
	void buildDescriptorPool(); //!< creates the descriptor pool using m_poolSizes
	void allocateDescriptorSets(Stage stage); //!< allocates descriptor sets to the descriptor pool for the input stage
	void freeDescriptors() const; //!< frees the descriptors on the destruction of the DescriptorManager
	void resetDescriptorPool(); //!< resets the descriptor pool
	/// descriptor set layout methods
	void addBinding(Stage stage, uint32_t binding, VkDescriptorType type, VkShaderStageFlags flags, uint32_t count = 1, VkSampler* samplers = nullptr); //!< adds a descriptor set layout binding to the vector
	bool bindingFound(Stage stage, uint32_t index); //!< checks if a binding is already being used
	void buildDescriptorSetLayout(Stage stage); //!< creates the descriptor set layout
	/// write descriptor set methods
	void addWriteDescriptorSet(Stage stage, uint32_t binding, VkDescriptorBufferInfo* bufferInfo = nullptr, VkDescriptorImageInfo* imageInfo = nullptr); //!< adds a write descriptor set to the vector
	void overwrite(Stage stage, uint32_t index); //!< updates the data in the relevant descriptor set
private:
	Device* m_device; //!< device object pointer
	VkDescriptorPool m_descriptorPool; //!< handle to the descriptor pool
	std::vector<VkDescriptorPoolSize> m_poolSizes; //!< vector of the pool sizes
	VkDescriptorSetLayout m_graphicsDescriptorSetLayout; //!< the descriptor set layout for the graphics descriptor set
	VkDescriptorSetLayout m_computeDescriptorSetLayout; //!< the descriptor set layout for the compute descriptor set
	std::vector<VkDescriptorSetLayoutBinding> m_graphicsBindings; //!< the bindings for the graphics descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> m_computeBindings; //!< the bindings for the compute descriptor set layout
	std::vector<VkDescriptorSet> m_graphicsDescriptorSets; //!< the graphics descriptor sets
	std::vector<VkDescriptorSet> m_computeDescriptorSets; //!< the compute descriptor sets
	std::vector<VkWriteDescriptorSet> m_graphicsDescriptorWrites; //!< the writes for the graphics descriptor set
	std::vector<VkWriteDescriptorSet> m_computeDescriptorWrites; //!< the writes for the command descriptor set
	uint32_t m_maxSets; //< the max number of sets for each vector of descriptor sets
};
