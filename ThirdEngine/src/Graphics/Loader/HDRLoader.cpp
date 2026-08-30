#include "HDRLoader.h"

std::optional<HDRImageData> HDR::LoadHDRImage(const std::filesystem::path& path)
{
	int width = 0;
	int height = 0;
	int sourceChannels = 0;

	stbi_set_flip_vertically_on_load(false);

	float* pixels = stbi_loadf(path.string().c_str(), &width, &height, &sourceChannels, STBI_rgb_alpha);

	if (!pixels) {
		std::cerr << "Failed to load HDR: " << path.string() << "\nReason: " << stbi_failure_reason() << std::endl;
	}

	HDRImageData result{};
	result.width = static_cast<uint32_t>(width);
	result.height = static_cast<uint32_t>(height);

	const size_t floatCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
	result.pixels.assign(pixels, pixels + floatCount);

	stbi_image_free(pixels);

	return result;
}