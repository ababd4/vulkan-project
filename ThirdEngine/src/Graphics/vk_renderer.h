#pragma once

#include "../Graphics/vk_Context.h"
#include "../Graphics/Pipeline/vk_PipelineManager.h"
#include "../Graphics/Mesh/vk_MeshManager.h"
#include "../Graphics/vk_Descriptors.h"
#include "../Graphics/vk_Swapchain.h"
#include "../Graphics/vk_Init.h"
#include "../Window/Window.h"
#include "../Camera/camera.h"
#include "vk_Types.h"

#include <../../vendor/include/SDL/SDL.h>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void PushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void Flush()
	{
		// reverse itrate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); // call functors
		}

		deletors.clear();
	}

	void PrintContents() {
		fmt::print("Deletion Queue Contents:\n");
		fmt::print("Total number of deletion functions: {}\n", deletors.size());

		for (size_t i = 0; i < deletors.size(); ++i) {
			fmt::print("Deletion Function [{}]: {}\n",
				i,
				typeid(deletors[i]).name()
			);
		}
	}
};

struct FrameResource {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore swapchainSemaphore, renderSemaphore;
	VkFence renderFence;

	DeletionQueue deletionQueue;
	DescriptorAllocatorGrowable frameDescriptor;
};

constexpr int MAX_FRAME = 2;

class Renderer
{
public:

	void Init(VulkanContext* context, Window& window, PipelineManager* pipelineManager, MeshManager* meshManager, BufferManager* bufferManager);
	void Cleanup();

	void UpdateScene();
	void Render();

	void HandleSDLEvents(const SDL_Event& e);

private:
	VulkanContext* m_pContext;
	PipelineManager* m_pPipelineManager;
	MeshManager* m_pMeshManager;
	BufferManager* m_pBufferManager;

	// Push Constants
	GPUDrawPushConstants m_pushConstants;

	// Descriptor
	DescriptorAllocatorGrowable m_descriptorAllocator;
	VkDescriptorSetLayout m_layout;

	// Drawing image
	VkDescriptorSet m_drawImageDescriptors;
	VkDescriptorSetLayout m_drawImageDescriptorLayout;

	// Pipeline Description
	std::vector<PipelineDesc> m_pipelineDesc;

	// Swapchain
	Swapchain m_swapchain;
		
	// Render Pass
	VkRenderPass m_renderPass;
	uint32_t subpass;

	// Frame
	std::vector<VkFramebuffer> m_framebuffers;
	uint32_t currentFrameIndex = 0;
	FrameResource m_frameResources[MAX_FRAME];
	FrameResource& GetCurrentFrame() { return m_frameResources[currentFrameIndex % MAX_FRAME]; }

	// Camera
	Camera m_mainCamera;
	float zNear = 0.1f;
	float zFar = 1000.0f;

	// Scene Data
	GPUSceneData m_sceneData;
	VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout;

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateRenderPass();
	void CreateFramebuffer();
	void CreateDescriptorAllocator();
	void CreatePipeline();
	void CreateSyncObjects();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void RecreateSwapchain();
};

