#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 norm; // i swear its the colour data... for now

	// Helper functions for this vertex class; oops, sorry, struct...

	// Binding descriptions - yep! you gotta tell vulkan, so why not here?
	static VkVertexInputBindingDescription getBindingDescription();

	// Attribute descriptions - VAO's pretty much
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

const std::vector<Vertex> vertices = {
	{ { 0.0f, -0.5f, 0.5f },{ 1.0f, 0.0f, 0.0f } },
	{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f } },
	{ { -0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } }
};

const int WIDTH = 1280;
const int HEIGHT = 720;

const std::vector<const char *> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback);

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);