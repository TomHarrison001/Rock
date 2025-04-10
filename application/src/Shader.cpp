#include "Shader.h"

Shader::Shader(const char* sourceFilename, const char* destFilename)
	: m_sourceFilename(sourceFilename), m_destFilename(destFilename)
{
	m_shaderSource = readShaderFile(m_sourceFilename.c_str());
	CompileShader(glslangShaderStageFromFileName(m_sourceFilename.c_str()), m_shaderSource.c_str(), &m_spirv, glslang_default_resource());
	SaveSPIRVBinaryFile(m_destFilename.c_str(), m_spirv.data(), m_spirv.size());
}

Shader::~Shader()
{
	m_spirv = std::vector<uint8_t>();
}

lvk::Result Shader::CompileShader(glslang_stage_t stage, const char* code, std::vector<uint8_t>* outSPIRV, const glslang_resource_t* glslLangResource)
{
	const glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_3,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_6,
		.code = code,
		.default_version = 100,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = glslLangResource
	};

	glslang_shader_t* shader = glslang_shader_create(&input);
	SCOPE_EXIT{
		glslang_shader_delete(shader);
	};

	if (!glslang_shader_preprocess(shader, &input)) {
		LLOGW("Shader preprocessing failed:\n");
		LLOGW("    %s\n", glslang_shader_get_info_log(shader));
		LLOGW("    %s\n", glslang_shader_get_info_debug_log(shader));
		lvk::logShaderSource(code);
		return lvk::Result(lvk::Result::Code::RuntimeError);
	}

	if (!glslang_shader_parse(shader, &input)) {
		LLOGW("Shader parsing failed:\n");
		LLOGW("    %s\n", glslang_shader_get_info_log(shader));
		LLOGW("    %s\n", glslang_shader_get_info_debug_log(shader));
		lvk::logShaderSource(glslang_shader_get_preprocessed_code(shader));
		return lvk::Result(lvk::Result::Code::RuntimeError);
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);
	SCOPE_EXIT{
		glslang_program_delete(program);
	};

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		LLOGW("Shader linking failed:\n");
		LLOGW("    %s\n", glslang_program_get_info_log(program));
		LLOGW("    %s\n", glslang_program_get_info_debug_log(program));
		return lvk::Result(lvk::Result::Code::RuntimeError);
	}

	glslang_spv_options_t options = {
		.generate_debug_info = true,
		.strip_debug_info = false,
		.disable_optimizer = false,
		.optimize_size = true,
		.disassemble = false,
		.validate = true,
		.emit_nonsemantic_shader_debug_info = false,
		.emit_nonsemantic_shader_debug_source = false
	};

	glslang_program_SPIRV_generate_with_options(program, input.stage, &options);

	if (glslang_program_SPIRV_get_messages(program)) {
		LLOGW("%s\n", glslang_program_SPIRV_get_messages(program));
	}

	const uint8_t* spirv = reinterpret_cast<const uint8_t*>(glslang_program_SPIRV_get_ptr(program));
	const size_t numBytes = glslang_program_SPIRV_get_size(program) * sizeof(uint32_t);
	*outSPIRV = std::vector(spirv, spirv + numBytes);

	return lvk::Result();
}

void Shader::SaveSPIRVBinaryFile(const char* filename, const uint8_t* code, size_t size)
{
	LLOGL("Saving shader to SPIRV binary file...\n");
	FILE* f = fopen(filename, "wb");
	if (!f) {  // parent directory doesn't exist
		LLOGW("Path: %s doesn't exist.\n", filename);
		return;
	}
	fwrite(code, sizeof(uint8_t), size, f);
	fclose(f);
	LLOGL("Saved shader: %s\n", filename);
}

VkShaderModule Shader::CreateShaderModuleFromSPIRV(const void* spirv, size_t numBytes, const char* debugName, lvk::Result* outResult) const
{
	VkShaderModule vkShaderModule = VK_NULL_HANDLE;

	const VkShaderModuleCreateInfo ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = numBytes,
		.pCode = (const uint32_t*)spirv
	};

	const VkResult result = vkCreateShaderModule(vkDevice_, &ci, nullptr, &vkShaderModule);
	lvk::setResultFrom(outResult, result);
	if (result != VK_SUCCESS) return VK_NULL_HANDLE;

	VK_ASSERT(lvk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vkShaderModule, debugName));
	return vkShaderModule;
}
