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
