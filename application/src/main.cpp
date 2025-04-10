#include <vector>
#include <volk.h>
#include <vulkan/VulkanUtils.h>
#include <lvk/LVK.h>
#include "shared/Utils.h"
//#include "shared/HelpersGLFW.h"
#include "glslang/Public/resource_limits_c.h"
#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"
#include <ktx.h>
#include <ktx-software/lib/gl_format.h>

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

VkShaderModule createShaderModuleFromSPIRV(const void* spirv, size_t numBytes, const char* debugName, lvk::Result* outResult)
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

void compressTextureToBC7(const char* sourceFilename, const char* destFilename)
{
	const int numChannels = 4;
	int origW, origH;
	uint8_t* pixels = stbi_load(sourceFilename, &origW, &origH, nullptr, numChannels);
	const uint32_t numMipLevels = lvk::calcNumMipLevels(origW, origH);

	LLOGL("Compressing texture to BC7...\n");

	// create a KTX2 texture for RBGA data
	ktxTextureCreateInfo ciKTX2 = {
		.glInternalformat = GL_RGBA8,
		.vkFormat = VK_FORMAT_R8G8B8A8_UNORM,
		.baseWidth = (uint32_t)origW,
		.baseHeight = (uint32_t)origH,
		.baseDepth = 1u,
		.numDimensions = 2u,
		.numLevels = numMipLevels,
		.numLayers = 1u,
		.numFaces = 1u,
		.generateMipmaps = KTX_FALSE
	};
	ktxTexture2* textureKTX2 = nullptr;
	ktxTexture2_Create(&ciKTX2, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX2);

	int w = origW;
	int h = origH;

	for (uint32_t i = 0; i != numMipLevels; ++i) {
		size_t offset = 0;
		ktxTexture_GetImageOffset(ktxTexture(textureKTX2), i, 0, 0, &offset);
		stbir_resize_uint8_linear((const unsigned char*)pixels, origW, origH, 0, ktxTexture_GetData(ktxTexture(textureKTX2)) + offset, w, h, 0, STBIR_RGBA);
		h = h > 1 ? h >> 1 : 1;
		w = w > 1 ? w >> 1 : 1;
	}

	// compress to Basis and transcode to BC7
	ktxTexture2_CompressBasis(textureKTX2, 255);
	ktxTexture2_TranscodeBasis(textureKTX2, KTX_TTF_BC7_RGBA, 0);

	// convert to KTX1
	ktxTextureCreateInfo ciKTX1 = {
		.glInternalformat = GL_COMPRESSED_RGBA_BPTC_UNORM,
		.vkFormat = VK_FORMAT_BC7_UNORM_BLOCK,
		.baseWidth = (uint32_t)origW,
		.baseHeight = (uint32_t)origH,
		.baseDepth = 1u,
		.numDimensions = 2u,
		.numLevels = numMipLevels,
		.numLayers = 1u,
		.numFaces = 1u,
		.generateMipmaps = KTX_FALSE
	};
	ktxTexture1* textureKTX1 = nullptr;
	ktxTexture1_Create(&ciKTX1, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX1);

	for (uint32_t i = 0; i != numMipLevels; ++i) {
		size_t offset1 = 0;
		ktxTexture_GetImageOffset(ktxTexture(textureKTX1), i, 0, 0, &offset1);
		size_t offset2 = 0;
		ktxTexture_GetImageOffset(ktxTexture(textureKTX2), i, 0, 0, &offset2);
		memcpy(
			ktxTexture_GetData(ktxTexture(textureKTX1)) + offset1,
			ktxTexture_GetData(ktxTexture(textureKTX2)) + offset2,
			ktxTexture_GetImageSize(ktxTexture(textureKTX1), i));
	}
	
	if (ktxTexture_WriteToNamedFile(ktxTexture(textureKTX1), destFilename) == KTX_SUCCESS)
		LLOGL("Saved compressed image: %s\n", destFilename);
	else
		LLOGW("Path: %s doesn't exist.\n", destFilename);
	ktxTexture_Destroy(ktxTexture(textureKTX1));
	ktxTexture_Destroy(ktxTexture(textureKTX2));
	
	if (pixels)
		stbi_image_free(pixels);
}

int main()
{
	glslang_initialize_process();

	testShaderCompilation("application/res/main.vert", "application/res/.cache/application.vert.bin");
	testShaderCompilation("application/res/main.frag", "application/res/.cache/application.frag.bin");

	glslang_finalize_process();

	compressTextureToBC7("data/wood.jpg", "data/.cache/wood.ktx");
	return 0;
}
