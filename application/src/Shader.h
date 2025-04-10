#pragma once

#include <vector>
#include <vulkan/VulkanUtils.h>
#include "shared/Utils.h"
#include "glslang/Public/resource_limits_c.h"

class Shader
{
private:
	VkDevice vkDevice_ = VK_NULL_HANDLE;

	std::string m_sourceFilename;
	std::string m_destFilename;
	std::string m_shaderSource;
	std::vector<uint8_t> m_spirv;
public:
	Shader(const char* sourceFilename, const char* destFilename);
	~Shader();

	lvk::Result CompileShader(glslang_stage_t stage, const char* code, std::vector<uint8_t>* outSPIRV, const glslang_resource_t* glslLangResource);
	void SaveSPIRVBinaryFile(const char* filename, const uint8_t* code, size_t size);
	VkShaderModule CreateShaderModuleFromSPIRV(const void* spirv, size_t numBytes, const char* debugName, lvk::Result* outResult) const;
};
