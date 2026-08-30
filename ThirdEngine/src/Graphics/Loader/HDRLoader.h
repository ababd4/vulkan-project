#pragma once

#include "../Types.h"
#include <stb_image/stb_image.h>

#include <optional>
#include <filesystem>

namespace HDR 
{
	std::optional<HDRImageData> LoadHDRImage(const std::filesystem::path& path);
}