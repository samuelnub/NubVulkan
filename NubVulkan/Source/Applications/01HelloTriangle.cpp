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

void HelloTriangleApp::createInstance()
{
	this->appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	this->appInfo.pApplicationName = "Ey doe";
	this->appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	this->appInfo.pEngineName = "no engine :(";
	this->appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	this->appInfo.apiVersion = VK_API_VERSION_1_0;

	this->createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	this->createInfo.pApplicationInfo = &this->appInfo;

	unsigned int glfwExtCount = 0;
	const char **glfwExts;

	glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

	this->createInfo.enabledExtensionCount = glfwExtCount;
	this->createInfo.ppEnabledExtensionNames = glfwExts;

	// Empty for now
	this->createInfo.enabledLayerCount = 0;

	VkResult result = vkCreateInstance(&this->createInfo, nullptr, this->instance.replace());

	if (vkCreateInstance(&this->createInfo, nullptr, this->instance.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create Vulkan instance :(");
	}
	else
	{
		std::cout << "Created a Vulkan instance!\n";
	}

	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> exts(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

	std::cout << "Available VK extensions:\n";
	for (const auto &ext : exts)
	{
		std::cout << "\t" << ext.extensionName << "\n";
	}
}

void HelloTriangleApp::loop()
{
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
	}
}
