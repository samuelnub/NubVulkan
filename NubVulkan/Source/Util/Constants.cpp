#include <Util/Constants.h>

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) 
	{
		func(instance, callback, pAllocator);
	}
}

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	// A vertex binding description describes (duh) at
	// which rate to load data from memory. it specifies
	// the number of bytes between entries and whether to
	// move the next data entry after each vertex oooor
	// after each instance
	
	VkVertexInputBindingDescription bindDesc = {};
	bindDesc.binding = 0;
	bindDesc.stride = sizeof(Vertex);
	bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	// _vertex will move to the next data entry after each
	// vertex, whereas
	// _instance will move to the next data entry after
	// each instance
	// right now we're not doing instanced rendering, so
	// we can stick to per-vertex data

	return bindDesc;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()
{
	// the magic number for the array size of 2 is 'cause
	// of the amount of attributes we have, which is just
	// the pos and norm (shh, colour...)

	std::array<VkVertexInputAttributeDescription, 2> attribDescs = {};
	
	// this part should make sense
	// heh, regarding the format, its a vector 3, aka 3
	// components, so RGB, thanks vulkan lol
	attribDescs[0].binding = 0;
	attribDescs[0].location = 0;
	attribDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribDescs[0].offset = offsetof(Vertex, pos);

	attribDescs[1].binding = 0;
	attribDescs[1].location = 1;
	attribDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribDescs[1].offset = offsetof(Vertex, norm);

	return attribDescs;
}
