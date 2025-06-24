#include "vk_DescriptorManager.h"

void DescriptorSetManager::init(VulkanContext& context)
{
    _context = &context;

    //create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };

    //_allocator.init(context.getDevice(), 10, sizes);
}

void DescriptorSetManager::cleanup()
{

}