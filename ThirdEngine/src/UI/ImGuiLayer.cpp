#include "ImguiLayer.h"

#include "Graphics/Backend/Init.h"
#include "Util/util.h"

void ImGuiLayer::Init(VulkanContext* context, Swapchain* swapchain, Window* window)
{
    //  1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(context->GetDevice(), &pool_info, nullptr, &m_imguiPool));

    // initialize the core of imgui
    ImGui::CreateContext();

    // initialize imgui for SDL
    ImGui_ImplSDL2_InitForVulkan(window->GetWindow());

    // initialize imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context->GetInstance();
    init_info.PhysicalDevice = context->GetPhysicalDevice();
    init_info.Device = context->GetDevice();
    init_info.Queue = context->GetGraphicsQueue();
    init_info.DescriptorPool = m_imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    //dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat format = swapchain->GetSwapchainImageFormat();
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    // add the destroy the imgui created structures
    m_deletionQueue.PushFunction([=]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(context->GetDevice(), m_imguiPool, nullptr);
    });
}

void ImGuiLayer::BeginFrame(EngineStats* stats, glm::vec3 cameraPos)
{
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("frametime %f ms", stats->frameTime);
	ImGui::Text("fps %f", stats->fps);
	ImGui::Text("draw time %f ms", stats->meshDrawTime);
	ImGui::Text("update time %f ms", stats->sceneUpdateTime);
	ImGui::Text("triangles %i", stats->triangleCount);
	ImGui::Text("draws %i", stats->drawcallCount);

    ImGui::Separator();
    ImGui::Text("CameraPosition: X %.3f Y %.3f Z %.3f", cameraPos.x, cameraPos.y, cameraPos.z);

	ImGui::End();
    ImGui::Render();
}

void ImGuiLayer::Render(VkCommandBuffer cmd, VkImageView targetImageView, VkExtent2D extent)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::CreateColorAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::CreateRenderingInfo(extent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void ImGuiLayer::HandleEvent(SDL_Event* e)
{
	ImGui_ImplSDL2_ProcessEvent(e);
}

void ImGuiLayer::Cleanup()
{
    m_deletionQueue.Flush();
}