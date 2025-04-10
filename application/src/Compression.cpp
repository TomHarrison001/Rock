#include "Compression.h"

void Compression::CompressTextureToBC7(const char* sourceFilename, const char* destFilename)
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
