#pragma once

#include "../Util/types.h"

namespace vkinit {
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
	VkPipelineLayoutCreateInfo pipeline_layout_create_info();
}

