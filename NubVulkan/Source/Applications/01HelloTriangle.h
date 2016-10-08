#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>

#include <Util/Constants.h>

class HelloTriangleApp
{
public:
	void run();

protected:


private:
	void initWindow();
	void initVulkan();
	void loop();

	GLFWwindow *window;
	
};