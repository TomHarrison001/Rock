#pragma once

#include <lvk/LVK.h>
#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"
#include <ktx.h>
#include <ktx-software/lib/gl_format.h>

class Compression
{
private:
public:
	static void CompressTextureToBC7(const char* sourceFilename, const char* destFilename);
};
