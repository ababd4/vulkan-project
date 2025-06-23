#include "vk_context.h"

#include "../Util/util.h"

void VulkanContext::init()
{
	create_instance();
	create_device();
	create_allocator();
}

void VulkanContext::cleanup()
{
	vmaDestroyAllocator(_allocator);

	vkDestroyDevice(_device, nullptr);
	vkDestroyInstance(_instance, nullptr);
}

void VulkanContext::create_instance()
{
	VkApplicationInfo application_info{};
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pNext = nullptr;

	// application name
	application_info.pApplicationName = "Third Engine";

	// applicaton version
	application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

	// engine name
	application_info.pEngineName = "Third Engine";

	// engine version
	application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	// set vulkan's version 1.2
	application_info.apiVersion = VK_API_VERSION_1_2;

	// use validation layer
	const std::vector< const char* > layers{
		"VK_LAYER_KHRONOS_validation"
	};

	// create instance
	VkInstanceCreateInfo create_instance_info;
	create_instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_instance_info.pNext = nullptr;
	create_instance_info.flags = 0;

	// specify the application information
	create_instance_info.pApplicationInfo = &application_info;

	// specify use layer
	create_instance_info.enabledLayerCount = layers.size();
	create_instance_info.ppEnabledLayerNames = layers.data();

	create_instance_info.enabledExtensionCount = 0;
	create_instance_info.ppEnabledExtensionNames = nullptr;

	VK_CHECK( vkCreateInstance(&create_instance_info, nullptr, &_instance) );
}

void VulkanContext::create_device()
{
	// get the number of devices
	uint32_t device_count = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(_instance, &device_count, nullptr)); 
	
	// get the infomation of device
	std::vector<VkPhysicalDevice> devices(device_count);
	VK_CHECK( vkEnumeratePhysicalDevices(_instance, &device_count, devices.data()) );

	// optional: show infomation of physical device

	for (const auto& device : devices) {
		// TODO: choose the best GPU
		_physicalDevice = device;
	}
	
	if (_physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU");
	}

	// get the queues that are available on the physical device
	uint32_t queue_props_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queue_props_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_props(queue_props_count);
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queue_props_count, queue_props.data());

	for (uint32_t i = 0; i < queue_props.size(); i++) {
		if (queue_props[i].queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT) {
			_queue_family_index = i;
			break;
		}
	}

	const float priority = 0.0f;

	VkDeviceQueueCreateInfo queue_create_info{};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.pNext = nullptr;
	queue_create_info.flags = 0;
	queue_create_info.queueFamilyIndex = _queue_family_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &priority;

	//// use VK_EXT_pipeline_creation_feedback extention
	//std::vector< const char* > extension{
	//	VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME
	//};

	// create logical device
	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext = nullptr;
	device_create_info.flags = 0;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &queue_create_info;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = nullptr;
	device_create_info.enabledExtensionCount = 0; // extension.size();
	device_create_info.ppEnabledExtensionNames = nullptr; // extension.data();
	device_create_info.pEnabledFeatures = nullptr;

	VK_CHECK( vkCreateDevice(_physicalDevice, &device_create_info, nullptr, &_device) );
	
	// get queue from device
	vkGetDeviceQueue(_device, _queue_family_index, 0, &_graphicsQueue);

}

void VulkanContext::create_allocator()
{
	VmaAllocatorCreateInfo allocator_create_info{};
	allocator_create_info.instance = _instance;
	allocator_create_info.physicalDevice = _physicalDevice;
	allocator_create_info.device = _device;
	//allocator_create_info.vulkanApiVersion = 0;

	VK_CHECK( vmaCreateAllocator(&allocator_create_info, &_allocator) );
}