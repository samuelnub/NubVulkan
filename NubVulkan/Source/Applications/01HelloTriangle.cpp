#include <Applications/01HelloTriangle.h>

void HelloTriangleApp::run()
{
	this->initWindow();
	this->initVulkan();
	this->loop();
}

void HelloTriangleApp::initWindow()
{
	glfwInit();

	// Since glfw was built for OGL (kinda in the name)
	// We gotta specify to not create an OGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(WIDTH, HEIGHT, "NubVulkan", nullptr, nullptr);
}

void HelloTriangleApp::initVulkan()
{
	this->createInstance();
	this->setupDebugCallback();
	this->pickPhysicalDevice();
}

void HelloTriangleApp::setupDebugCallback()
{
	if (!enableValidationLayers)
	{
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = this->debugCallback;

	if (CreateDebugReportCallbackEXT(this->instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't set up debug callback!");
	}
}

void HelloTriangleApp::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// oh man, gl with that, eh?
	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find any graphics cards with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto &device : devices)
	{
		if (this->isDeviceSuitable(device))
		{
			this->physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a usable GPU!");
	}
}

bool HelloTriangleApp::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = this->findQueueFamilies(device);

	return indices.isComplete();
}

QueueFamilyIndices HelloTriangleApp::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}
		if (indices.isComplete())
		{
			break;
		}
		i++;
	}

	return indices;
}

bool HelloTriangleApp::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto &layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char *> HelloTriangleApp::getRequiredExtensions()
{
	std::vector<const char *> exts;

	unsigned int glfwExtCount = 0;
	const char **glfwExts;
	glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

	for (unsigned int i = 0; i < glfwExtCount; i++)
	{
		exts.push_back(glfwExts[i]);
	}

	if (enableValidationLayers)
	{
		exts.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return exts;
}

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApp::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData)
{
	std::cerr << "Validation layer: " << msg << "\n";
	return VK_FALSE;
}

void HelloTriangleApp::createInstance()
{
	if (enableValidationLayers && !this->checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers ain't existing :(");
	}

	this->appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	this->appInfo.pApplicationName = "Ey doe";
	this->appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	this->appInfo.pEngineName = "no engine :(";
	this->appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	this->appInfo.apiVersion = VK_API_VERSION_1_0;

	this->createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	this->createInfo.pApplicationInfo = &this->appInfo;

	auto exts = this->getRequiredExtensions();

	this->createInfo.enabledExtensionCount = exts.size();
	this->createInfo.ppEnabledExtensionNames = exts.data();

	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateInstance(&this->createInfo, nullptr, this->instance.replace());

	if (vkCreateInstance(&this->createInfo, nullptr, this->instance.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create Vulkan instance :(");
	}
	else
	{
		std::cout << "Created a Vulkan instance!\n";
	}
}

void HelloTriangleApp::loop()
{
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
	}
}

bool QueueFamilyIndices::isComplete()
{
	return this->graphicsFamily >= 0;
}
