#pragma once
#include "../Util/Types.h"
#include "../Util/Util.h"
#include "vk_Init.h"
#include <fstream>
#include <optional>

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    PipelineBuilder() { Clear(); }

    void Clear();

    VkPipeline BuildPipeline(VkDevice device);

    void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode mode);
    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void set_multisampling_none();
    void disable_blending();
    void set_color_attachment_format(std::optional<VkFormat> format);
    void set_depth_format(VkFormat format);
    void disable_depthtest();
    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
    void enable_blending_additive();
    void enable_blending_alphablend();
    void disable_rasterizer();
};

namespace vkutil {
    bool LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}
