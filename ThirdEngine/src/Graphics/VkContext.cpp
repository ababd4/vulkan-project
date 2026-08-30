#include "VkContext.h"

#include "../Util/Util.h"

void VulkanContext::Init(Window* window)
{
	CreateContext(window);
	CreateAllocator();
}
    
void VulkanContext::Cleanup()
{
    /*
    char* statsString = nullptr;

    vmaBuildStatsString(
        m_allocator,
        &statsString,
        VK_TRUE
    );

    std::cout << statsString << std::endl;

    vmaFreeStatsString(
        m_allocator,
        statsString
    );

    PrintAllocationCount("shutdown");
    */

	vmaDestroyAllocator(m_allocator);

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
    vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
	vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::PrintAllocationCount(std::string str)
{
    VmaTotalStatistics stats{};
    vmaCalculateStatistics(m_allocator, &stats);
    std::cout << str << ": " << stats.total.statistics.allocationCount << std::endl;
}

void VulkanContext::CreateContext(Window* window)
{
    vkb::InstanceBuilder builder;

    //make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Third Engine")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    vkb_inst = inst_ret.value();

    // grab the instance
    m_instance = vkb_inst.instance;
    m_debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(window->GetWindow(), m_instance, &m_surface);

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // use vkbootstrap to select a gpu.
    // select a gpu that can write to the sdl surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_surface(m_surface)
        .select()
        .value();

    // Craete the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the vulkan device handle used in the rest of a vulkan application
    m_device = vkbDevice.device;
    m_physicalDevice = physicalDevice.physical_device;

    // get a graphics queue
    m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    m_transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    m_presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    m_graphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    m_transferQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
}

void VulkanContext::CreateAllocator()
{
	VmaAllocatorCreateInfo allocator_create_info{};
	allocator_create_info.instance = m_instance;
	allocator_create_info.physicalDevice = m_physicalDevice;
	allocator_create_info.device = m_device;
	//allocator_create_info.vulkanApiVersion = 0;
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	VK_CHECK( vmaCreateAllocator(&allocator_create_info, &m_allocator) );
}