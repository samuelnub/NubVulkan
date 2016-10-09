#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>

#include <Util/Constants.h>
#include <Util/VDeleter.h>

class HelloTriangleApp
{
public:
	void run();

protected:


private:
	void initWindow();
	void initVulkan();
	void createInstance();
	void loop();

	GLFWwindow *window;
	
	VDeleter<VkInstance> instance{ vkDestroyInstance };

	// An app isn't global, you can make your own little apps
	VkApplicationInfo appInfo = {};
	// These creation params apply globally
	VkInstanceCreateInfo createInfo = {};

};