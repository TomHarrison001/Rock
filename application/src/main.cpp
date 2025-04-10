#include "Shader.h"
#include "Compression.h"

int main()
{
	glslang_initialize_process();

	Shader vertShader("application/res/main.vert", "application/res/.cache/application.vert.bin");
	Shader fragShader("application/res/main.frag", "application/res/.cache/application.frag.bin");

	glslang_finalize_process();

	Compression::CompressTextureToBC7("data/wood.jpg", "data/.cache/wood.ktx");

	return 0;
}
