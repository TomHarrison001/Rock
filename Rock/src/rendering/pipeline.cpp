/** \file pipeline.cpp */

#include "rendering/pipeline.hpp"

Pipeline::Pipeline(Device* device, const PipelineSettings& settings, const std::string& vertFilepath, const std::string& fragFilepath)
	: m_device(device)
{
    if (settings.pipelineLayout != nullptr) m_pipelineLayout = settings.pipelineLayout;
	createGraphicsPipeline(settings, vertFilepath, fragFilepath);
}

Pipeline::Pipeline(Device* device, const PipelineSettings& settings, const std::string& compFilepath)
	: m_device(device)
{
    if (settings.pipelineLayout != nullptr) m_pipelineLayout = settings.pipelineLayout;
	createComputePipeline(settings, compFilepath);
}

Pipeline::~Pipeline()
{
    vkDestroyPipelineLayout(m_device->getDevice(), m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_device->getDevice(), m_pipeline, nullptr);
	m_device = nullptr;
}

void Pipeline::createGraphicsPipeline(const PipelineSettings& settings, const std::string& vertFilepath, const std::string& fragFilepath)
{
    std::vector<char> vertShaderCode = readFile(vertFilepath);
    std::vector<char> fragShaderCode = readFile(fragFilepath);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    vertShaderCode = std::vector<char>();
    fragShaderCode = std::vector<char>();

    VkPipelineShaderStageCreateInfo shaderStages[2];
    // vert shader stage info
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main"; // function to invoke; i.e. entrypoint
    shaderStages[0].flags = 0; // optional
    shaderStages[0].pNext = nullptr; // optional
    shaderStages[0].pSpecializationInfo = nullptr; // optional
    // frag shader stage info
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main"; // function to invoke; i.e. entrypoint
    shaderStages[1].flags = 0; // optional
    shaderStages[1].pNext = nullptr; // optional
    shaderStages[1].pSpecializationInfo = nullptr; // optional

    auto& bindingDescription = settings.bindingDescription;
    auto& attributeDescriptions = settings.attributeDescriptions;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &settings.inputAssembly;
    pipelineInfo.pViewportState = &settings.viewportState;
    pipelineInfo.pRasterizationState = &settings.rasteriser;
    pipelineInfo.pMultisampleState = &settings.multisampling;
    pipelineInfo.pColorBlendState = &settings.colourBlending;
    pipelineInfo.pDynamicState = &settings.dynamicState;
    pipelineInfo.layout = settings.pipelineLayout;
    pipelineInfo.renderPass = settings.renderPass;
    pipelineInfo.subpass = settings.subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1; // optional

    if (vkCreateGraphicsPipelines(m_device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline.");

    vkDestroyShaderModule(m_device->getDevice(), vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device->getDevice(), fragShaderModule, nullptr);
}

void Pipeline::createComputePipeline(const PipelineSettings& settings, const std::string& compFilepath)
{
    std::vector<char> compShaderCode = readFile(compFilepath);

    VkShaderModule compShaderModule = createShaderModule(compShaderCode);
    compShaderCode = std::vector<char>();

    VkPipelineShaderStageCreateInfo compShaderStageInfo{};
    compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compShaderStageInfo.module = compShaderModule;
    compShaderStageInfo.pName = "main";
    compShaderStageInfo.flags = 0; // optional
    compShaderStageInfo.pNext = nullptr; // optional
    compShaderStageInfo.pSpecializationInfo = nullptr; // optional

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = settings.pipelineLayout;
    pipelineInfo.stage = compShaderStageInfo;

    if (vkCreateComputePipelines(m_device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline.");

    vkDestroyShaderModule(m_device->getDevice(), compShaderModule, nullptr);
}

VkShaderModule Pipeline::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device->getDevice(), &ci, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module.");

    return shaderModule;
}

std::vector<char> Pipeline::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Failed to open file.");

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void Pipeline::bindGraphics(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

void Pipeline::bindCompute(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
}

void Pipeline::defaultPipelineSettings(PipelineSettings& settings)
{
    settings.bindingDescription = Particle::getBindingDescription();
    settings.attributeDescriptions = Particle::getAttributeDescriptions();

    settings.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    settings.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    settings.inputAssembly.primitiveRestartEnable = VK_FALSE;

    settings.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    settings.viewportState.viewportCount = 1;
    settings.viewportState.pViewports = nullptr; // optional
    settings.viewportState.scissorCount = 1;
    settings.viewportState.pScissors = nullptr; // optional

    settings.rasteriser.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    settings.rasteriser.depthClampEnable = VK_FALSE;
    settings.rasteriser.rasterizerDiscardEnable = VK_FALSE;
    settings.rasteriser.polygonMode = VK_POLYGON_MODE_FILL; // fill the area of the polygon with fragments
    settings.rasteriser.lineWidth = 1.f;
    settings.rasteriser.cullMode = VK_CULL_MODE_NONE;
    settings.rasteriser.frontFace = VK_FRONT_FACE_CLOCKWISE;
    settings.rasteriser.depthBiasEnable = VK_FALSE;
    settings.rasteriser.depthBiasConstantFactor = 0.f; // optional
    settings.rasteriser.depthBiasClamp = 0.f; // optional
    settings.rasteriser.depthBiasSlopeFactor = 0.f; // optional

    settings.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    settings.multisampling.sampleShadingEnable = VK_FALSE;
    settings.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    settings.multisampling.minSampleShading = 1.f; // optional
    settings.multisampling.pSampleMask = nullptr; // optional
    settings.multisampling.alphaToCoverageEnable = VK_FALSE; // optional
    settings.multisampling.alphaToOneEnable = VK_FALSE; // optional

    settings.colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    settings.colourBlendAttachment.blendEnable = VK_FALSE;
    settings.colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // optional
    settings.colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // optional
    settings.colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    settings.colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // optional
    settings.colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
    settings.colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional

    settings.colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    settings.colourBlending.logicOpEnable = VK_FALSE; // VK_TRUE for alpha blending
    settings.colourBlending.logicOp = VK_LOGIC_OP_COPY; // optional; specify bitwise operation for alpha blending
    settings.colourBlending.attachmentCount = 1;
    settings.colourBlending.pAttachments = &settings.colourBlendAttachment;
    settings.colourBlending.blendConstants[0] = 0.f; // optional
    settings.colourBlending.blendConstants[1] = 0.f; // optional
    settings.colourBlending.blendConstants[2] = 0.f; // optional
    settings.colourBlending.blendConstants[3] = 0.f; // optional

    settings.dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    settings.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    settings.dynamicState.dynamicStateCount = static_cast<uint32_t>(settings.dynamicStates.size());
    settings.dynamicState.pDynamicStates = settings.dynamicStates.data();
    settings.dynamicState.flags = 0;
}

void Pipeline::enableAlphaBlending(PipelineSettings& settings)
{
    settings.colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    settings.colourBlendAttachment.blendEnable = VK_TRUE;
    settings.colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    settings.colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    settings.colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    settings.colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    settings.colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    settings.colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}
