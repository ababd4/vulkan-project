#include "vk_context.h"

#include "../Util/util.h"

void VulkanContext::init(Window* window)
{
	create_context(window);
	create_allocator();
}

void VulkanContext::cleanup()
{
	vmaDestroyAllocator(m_allocator);

	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void VulkanContext::create_context(Window* window)
{
    vkb::InstanceBuilder builder;

    //make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Vulkan Engine")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    m_instance = vkb_inst;
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
    m_queue_family_index = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanContext::create_allocator()
{
	VmaAllocatorCreateInfo allocator_create_info{};
	allocator_create_info.instance = m_instance;
	allocator_create_info.physicalDevice = m_physicalDevice;
	allocator_create_info.device = m_device;
	//allocator_create_info.vulkanApiVersion = 0;

	VK_CHECK( vmaCreateAllocator(&allocator_create_info, &m_allocator) );
}