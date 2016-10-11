#pragma once

#include <vulkan/vulkan.h>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback);

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);