#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <cstring>
#include <set>

#include <Util/Constants.h>
#include <Util/VDeleter.h>

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete();
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
	void createLogicalDevice();
	bool checkValidationLayerSupport();
	std::vector<const char *> getRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	void createInstance();
	void createSurface();
	void loop();

	GLFWwindow *window;
	
	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{instance, DestroyDebugReportCallbackEXT};
	VDeleter<VkDevice> device{ vkDestroyDevice };

	// An app isn't global, you can make your own little apps
	VkApplicationInfo appInfo = {};
	// These creation params apply globally
	VkInstanceCreateInfo createInfo = {};
	// Automatically deallocated/deleted upon VkInstance deletion, yay
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };
	VkQueue presentQueue;

};