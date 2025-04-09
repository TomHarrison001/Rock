#include <vector>
#include <volk.h>
#include <vulkan/VulkanUtils.h>
#include <lvk/LVK.h>
#include "shared/Utils.h"
//#include <shared/HelpersGLFW.h>
#include "glslang/Public/resource_limits_c.h"

VkDevice vkDevice_ = VK_NULL_HANDLE;

lvk::Result compileShader(glslang_stage_t stage, const char* code, std::vector<uint8_t>* outSPIRV, const glslang_resource_t* glslLangResource)
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

void saveSPIRVBinaryFile(const char* filename, const uint8_t* code, size_t size)
{
	FILE* f = fopen(filename, "wb");
	if (!f) {  // parent directory doesn't exist
		LLOGW("Path: %s doesn't exist.\n", filename);
		return;
	}
	fwrite(code, sizeof(uint8_t), size, f);
	fclose(f);
}

void testShaderCompilation(const char* sourceFilename, const char* destFilename)
{
	std::string shaderSource = readShaderFile(sourceFilename);
	std::vector<uint8_t> spirv;
	lvk::Result res = compileShader(
		glslangShaderStageFromFileName(sourceFilename),
		shaderSource.c_str(),
		&spirv,
		glslang_default_resource());
	saveSPIRVBinaryFile(destFilename, spirv.data(), spirv.size());
}

int main()
{
	glslang_initialize_process();

	testShaderCompilation("application/res/main.vert", "application/.cache/application.vert.bin");
	testShaderCompilation("application/res/main.frag", "application/.cache/application.frag.bin");

	glslang_finalize_process();

	return 0;
}
