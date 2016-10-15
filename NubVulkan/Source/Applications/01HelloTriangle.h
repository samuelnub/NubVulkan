#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Util/Constants.h>
#include <Util/VDeleter.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <cstring>
#include <set>
#include <algorithm>

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete();
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;
};

class HelloTriangleApp
{
public:
	void run();

protected:


private:
	void initWindow();
	void initVulkan();
	void setupDebugCallback();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	void createLogicalDevice();
	bool checkValidationLayerSupport();
	std::vector<const char *> getRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
	void createSwapChain();
	void createInstance();
	void createSurface();
	void createImageViews();
	void loop();

	GLFWwindow *window;
	
	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{instance, DestroyDebugReportCallbackEXT};
	VDeleter<VkDevice> device{ vkDestroyDevice };
	
	// Automatically deallocated/deleted upon VkInstance deletion, yay
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };
	VkQueue presentQueue;

	VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };
	// Also cleaned up by VkSwapchain deletion, yea buddy
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VDeleter<VkImageView>> swapChainImageViews;

};