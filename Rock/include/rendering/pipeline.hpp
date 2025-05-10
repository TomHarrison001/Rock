/** \file pipeline.hpp */

#pragma once

#include "core/device.hpp"
#include "utils/resources.hpp"

#include <string>
#include <fstream>

/* \struct PipelineSettings
*  \brief stores the relevant descriptions and CIs for a pipeline
*/
struct PipelineSettings
{
	PipelineSettings() = default; //!< default constructor
	PipelineSettings(const PipelineSettings&) = delete; //!< constructor
	PipelineSettings& operator=(const PipelineSettings&) = delete; //!< copy assignment

	VkVertexInputBindingDescription bindingDescription; //!< binding description used in VkPipelineVertexInputStateCreateInfo
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions; // attribute descriptions used in VkPipelineVertexInputStateCreateInfo
	VkPipelineInputAssemblyStateCreateInfo inputAssembly; //!< pipeline input assembly state
	VkPipelineViewportStateCreateInfo viewportState; //!< viewports and scissors with pointers
	VkPipelineRasterizationStateCreateInfo rasteriser; //!< rasterisation settings e.g. depth and culling
	VkPipelineMultisampleStateCreateInfo multisampling; //!< multisampling state
	VkPipelineColorBlendAttachmentState colourBlendAttachment; //!< colour blend attachment
	VkPipelineColorBlendStateCreateInfo colourBlending; //!< colour blending
	std::vector<VkDynamicState> dynamicStates; //!< dynamic states
	VkPipelineDynamicStateCreateInfo dynamicState; //!< dynamic state, stores dynamic states
	VkPipelineLayout pipelineLayout = nullptr; //!< handle to a pipeline layout object
	VkRenderPass renderPass = nullptr; //!< handle to a render pass
	uint32_t subpass = 0; //!< index of subpass in the render pass where this pipeline will be used
};

/* \class Pipeline
*  \brief stores handle to pipeline object, creates default PipelineSettings, bind pipeline to VK_PIPELINE_BIND_POINT
*/
class Pipeline
{
public:
	Pipeline(Device* device, const PipelineSettings& settings, const std::string& vertFilepath, const std::string& fragFilepath); //!< graphics pipeline constructor
	Pipeline(Device* device, const PipelineSettings& settings, const std::string& compFilepath); //!< compute pipeline constructor
	~Pipeline(); //!< destructor

	Pipeline(const Pipeline&) = delete; //!< copy constructor
	Pipeline& operator=(const Pipeline&) = delete; //!< copy assignment
private:
	void createGraphicsPipeline(const PipelineSettings& settings, const std::string& vertFilepath, const std::string& fragFilepath); //!< creates the graphics pipeline using the PipelineSettings and vert + frag shaders
	void createComputePipeline(const PipelineSettings& settings, const std::string& compFilepath); //!< creates the compute pipeline using the PipelineSettings and compute shader
	VkShaderModule createShaderModule(const std::vector<char>& code); //!< creates shader module from code
	static std::vector<char> readFile(const std::string& filename); //!< reads code in shader filepath
public:
	void bindGraphics(VkCommandBuffer commandBuffer); //!< binds pipeline to VK_PIPELINE_BIND_POINT_GRAPHICS
	void bindCompute(VkCommandBuffer commandBuffer); //!< binds pipeline to VK_PIPELINE_BIND_POINT_COMPUTE
	static void defaultPipelineSettings(PipelineSettings& settings); //!< populates referenced PipelineSettings with default data
	static void enableAlphaBlending(PipelineSettings& settings); //!< adapts referenced PipelineSettings to enable alpha blending
	VkPipelineLayout getPipelineLayout() { return m_pipelineLayout; } //!< returns the handle to pipeline layout object
private:
	Device* m_device; //!< device object pointer
	VkPipeline m_pipeline; //!< handle to pipeline object
	VkPipelineLayout m_pipelineLayout; //!< handle to pipeline layout object
};
