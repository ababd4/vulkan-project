#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_sdl2.h>

#include "../Types.h"
#include "../VkContext.h"
#include "../Swapchain.h"
#include "../../Window/Window.h"

class ImGuiLayer 
{		
public:
	void Init(VulkanContext* context, Swapchain* swapchain, Window* window);
	void Cleanup();
	void BeginFrame(EngineStats* stats, glm::vec3 cameraPos);
	void Render(VkCommandBuffer cmd, VkImageView targetImageView, VkExtent2D extent);
	void HandleEvent(SDL_Event* e);

private:
	DeletionQueue m_deletionQueue;
	VkDescriptorPool m_imguiPool;
};