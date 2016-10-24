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

	this->window = glfwCreateWindow(WIDTH, HEIGHT, "NubVulkan", nullptr, nullptr);

	// Hooooo! i'm here from recreateSwapChain()!
	glfwSetWindowUserPointer(this->window, this);
	// woah, you can set your own custom pointer for your
	// specified window! let's give it the "this" context
	// (it's initially just NULL) we'll use this to point
	// to our static member func

	glfwSetWindowSizeCallback(this->window, HelloTriangleApp::onWindowResized);
}

void HelloTriangleApp::initVulkan()
{
	this->createInstance();
	this->setupDebugCallback();
	this->createSurface();
	this->pickPhysicalDevice();
	this->createLogicalDevice();
	this->createSwapChain();
	this->createImageViews();
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFrameBuffers();
	this->createCommandPool();
	this->createVertexBuffer();
	this->createCommandBuffers();
	this->createSemaphores();
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

	bool extensionsSupported = this->checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentMode.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
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

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}
		i++;
	}

	return indices;
}

VkSurfaceFormatKHR HelloTriangleApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto &availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	// or else... worst case scenario
	return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	for (const auto &availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	// Worst case
	return VK_PRESENT_MODE_FIFO_KHR;
}

void HelloTriangleApp::createLogicalDevice()
{
	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
		queueCreateInfo.queueCount = 1;
		// Most drivers only let you make a few/couple queues,
		// And handling more than 1 would just be cumbersome

		// VK lets you bribe the command buffer to how you
		// want your object prioritized, 0.0f to 1.0f
		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;

	// Validation layer debug output gave a really neat
	// Message as to why it wasn't working:
	// device extension for swapchain wasn't loaded!
	// it was set to 0 for the first part and i never
	// changed it
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create a logical VK device!");
	}

	vkGetDeviceQueue(this->device, indices.graphicsFamily, 0, &this->graphicsQueue);
	vkGetDeviceQueue(this->device, indices.presentFamily, 0, &this->presentQueue);
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

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApp::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) 
{
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

bool HelloTriangleApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);

	std::vector<VkExtensionProperties> availableExts(extCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, availableExts.data());

	std::set<std::string> requiredExts(deviceExtensions.begin(), deviceExtensions.end());

	std::cout << "Here's the available device extensions:\n";
	for (const auto &ext : availableExts)
	{
		std::cout << "\t" << ext.extensionName << "\n";
		requiredExts.erase(ext.extensionName);
	}

	return requiredExts.empty();
}

SwapChainSupportDetails HelloTriangleApp::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	// Takes in your physical device and the window 
	// Surface, and takes a good hard look at their
	// capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		// reset to fit all available formats
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentMode.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, details.presentMode.data());
	}
	
	return details;
}

VkExtent2D HelloTriangleApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	// the resolution of the swap chain images

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { WIDTH, HEIGHT };
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, capabilities.maxImageExtent.height));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, capabilities.maxImageExtent.height));

		return actualExtent;
	}
}

std::vector<char> HelloTriangleApp::readFile(const std::string & fileName)
{
	// generic file opener (as binary)

	// ::ate starts reading @ end of file
	// ::binary reads it as binary file
	// (our spir-v bytecode)
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file");
	}

	// .tellg() gives the index at where the "cursor" is,
	// which is at the EOF currently, which gives us size
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	// seek back to beginning since we have filesize
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer; // wonderful
}

uint32_t HelloTriangleApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	// first, lets query the available mem types
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);
	// this struct has 2 arrays, memoryTypes & memoryHeaps
	// heaps are distinct mem resources like ded. VRAM
	// and swap space in RAM (when your VRAM runs out!)
	// right now we're just gonna care about the type, not
	// theh heap it originates from
	
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) // sneaky bitwise ops!
		{
			// the second && checks is because
			// we're not just interested in a mem type
			// that is suitable for the vert buffer; we
			// need a mem type that'll write our vert data
			// to that memory!
			// the right memory type will have
			// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			// so our cpu can write to it
			// in the future we may need to && some more
			// args!
			
			return i;
		}
	}

	// wewlads
	throw std::runtime_error("Couldn't find a suitable memory type! What sort of freaking computer are you using???");
}

void HelloTriangleApp::onWindowResized(GLFWwindow * window, int width, int height)
{
	if (width == 0 || height == 0)
	{
		return;
	}

	HelloTriangleApp *app = reinterpret_cast<HelloTriangleApp *>(glfwGetWindowUserPointer(window));
	// so _that's_ how you reinterpret_cast... cool beans
	// man, that gave me a false sense of intelligence

	app->recreateSwapChain();
}

void HelloTriangleApp::createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule)
{
	// Making a shader module is "easy", just give it the
	// address of your code data buffer & its size
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t *)code.data();

	if (vkCreateShaderModule(device, &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create shader module!");
	}
}

void HelloTriangleApp::createSwapChain()
{
	// Tying it all together

	SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(this->physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentMode);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	// This part _could_ be put into its own func,
	// But keep it simple. Just setting how many images
	// in our swap chain, aka "image queue"
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	// A value of 0 means that there's no limit
	// (besides hardware VRAM limits, duh)
	// so it's best to limit it
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = this->surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; // Almost always 1 unless you're doing stereographic rendering
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = this->findQueueFamilies(this->physicalDevice);
	uint32_t queueFamilyIndices[] = {(uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily};

	// There's 2 ways to handle images that are accessed
	// from multiple queues, either ..._EXCLUSIVE
	// or ..._CONCURRENT, the former means that the image
	// is owned by only one queue family at a time
	// (like a mutex lock on the image) better performance
	// the latter is just as you'd expect; allowing
	// multiple queue families to use images without
	// explicitly declaring ownership
	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // optional
		createInfo.pQueueFamilyIndices = nullptr; // optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	// Ooh! allows you to blend with other windows
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	// We don't care about the pixels being obscured
	createInfo.clipped = VK_TRUE;
	// If your window's resized, you'd need to recreate
	// the entire swapchain, whoo boy, covered later
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// Hello higher order beings of the past! I have come
	// from the recreateSwapChain() function to bring you
	// new additions!
	VkSwapchainKHR oldSwapChain = this->swapChain;
	createInfo.oldSwapchain = oldSwapChain;

	VkSwapchainKHR newSwapChain;
	// This should have the newSwapChain as the last param
	if (vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create a swap chain!");
	}

	// Here too! this is new too
	this->swapChain = newSwapChain;
	// This will destroy the old swapchain and replace the
	// handle with the handle of the new one!

	vkGetSwapchainImagesKHR(this->device, this->swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(this->device, this->swapChain, &imageCount, swapChainImages.data());

	this->swapChainImageFormat = surfaceFormat.format;
	this->swapChainExtent = extent;
}

void HelloTriangleApp::createInstance()
{
	if (enableValidationLayers && !this->checkValidationLayerSupport()) 
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void HelloTriangleApp::createSurface()
{
	if (glfwCreateWindowSurface(this->instance, window, nullptr, surface.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create window surface!");
	}
}

void HelloTriangleApp::createImageViews()
{
	// Since we've got an array of these, gotta construct
	// them manually
	this->swapChainImageViews.resize(this->swapChainImages.size(), VDeleter<VkImageView>{this->device, vkDestroyImageView});

	for (uint32_t i = 0; i < this->swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = this->swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		// ooh, lets you map the colour channels to other
		// colours, eg red outputs will be mapped to
		// grayscale, etc., but we're sticking to the def.
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// subresourcerange specifies what this image's
		// purpose will be
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		
		if (vkCreateImageView(this->device, &createInfo, nullptr, this->swapChainImageViews[i].replace()) != VK_SUCCESS)
		{
			throw std::runtime_error("Couldn't create image view!");
		}
	}
}

void HelloTriangleApp::createRenderPass()
{
	// Hold up! before we can finish up that pipeline,
	// We gotta set this up! We need to tell vulkan about
	// the framebuffer attachments we'll be using for
	// rendering. eg no. of colour & depth buffers, how 
	// many samples, and how to handle their contents
	// throughout rendering

	VkAttachmentDescription colAtt = {};
	colAtt.format = this->swapChainImageFormat;
	colAtt.samples = VK_SAMPLE_COUNT_1_BIT;
	// stick to 1, as we're not doing anything for now

	colAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// these 2 params determine what to do with the data
	// in the attachment before & after rendering
	// op_clear clears the vals to a const at the start
	// op_load preserves the existing contents of the att
	// op_dont_care treats existing contents as undefined
	// storeop operations are as follows:
	// op_store - rendered contents will be stored in mem
	// and can be read later
	// op_dont_care - contents of the framebuffer will be
	// undefined after the rendering op

	colAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// textures and framebuffers in vk are represented by
	// the VkImage object, with certain pixel formats etc
	// specified. however, the layout of pixels can change
	// based on what you want to do with the image
	// vk_image_layout_color_attachment_optimal - images
	// used as colour att.
	// vk_image_layout_present_src_khr - images to be
	// presented in the swapchain
	// vk_image_layout_transfer_dst_optimal - images used
	// as destination for a mem copy operation
	// more info in the fuuuuture
	// initial layout specifies the format prior to the
	// render pass, and the final layout is the format to
	// transition into when the render pass finishes

	// Subpasses and attachment references
	// a single render pass can have multiple sub-passes
	// they're subsequent rendering operations that depend
	// on the contents of framebuffers in previous passes
	// eg. motion blur effects

	VkAttachmentReference colAttRef = {};
	colAttRef.attachment = 0;
	// our array consists of a single vkattachmentdesc,
	// so its index is just the first; 0
	colAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// just stick with this, it's optimal already!

	// just a struct to describe the subpass
	// ps: vk may support compute subpasses in the future!
	VkSubpassDescription subPass = {};
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colAttRef;
	// you can also ref these kinds of subpass att's:
	// pInputAttachments, pResolveAttachments,
	// pDepthStencilAttachment and pPreserveAttachments
	
	// Hey! I came from the drawFrame() function! go there
	// to see why I'm here
	VkSubpassDependency dep = {};
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	// the fields specify the indices of the dependencies
	// the VK_SUBPASS_EXTERNAL refers to the implicit
	// subpass before/after the renderpass depending on
	// whether it's specified in src or dst subpass
	// the index 0 refers to our subpass index, which is
	// the only one right now lol
	// dst must always be higher than src to prevent
	// cycles in the dependency graph

	dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// these params specify the ops to wait on and the
	// stages in which these ops occur. we need to wait
	// for the swap chain to finish reading from the image
	// before we can access it, duh. this read happens in
	// the last pipeline stage

	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// or'd. the operations that should wait on this are
	// in the color attachment stage and involve the read
	// and write of the color attachment. these settings
	// will prevent the transition from happening until it
	// is necessary & allowed; when we want to start
	// writing colours to it

	VkRenderPassCreateInfo rendPassInfo = {};
	rendPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rendPassInfo.attachmentCount = 1;
	rendPassInfo.pAttachments = &colAtt;
	rendPassInfo.subpassCount = 1;
	rendPassInfo.pSubpasses = &subPass;

	//added on!
	rendPassInfo.dependencyCount = 1;
	rendPassInfo.pDependencies = &dep;

	if (vkCreateRenderPass(this->device, &rendPassInfo, nullptr, this->renderPass.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create render pass!");
	}
}

void HelloTriangleApp::createGraphicsPipeline()
{
	// For now, we've just got these 2 cute lil shaders
	auto vertShaderCode = readFile("Shaders/shader.vert.spv");
	auto fragShaderCode = readFile("Shaders/shader.frag.spv");

	// Just like in opengl, we can discard the shaders
	// once we've got our program compiled and linked
	VDeleter<VkShaderModule> vertShaderModule{ this->device, vkDestroyShaderModule };
	VDeleter<VkShaderModule> fragShaderModule{ this->device, vkDestroyShaderModule };
	this->createShaderModule(vertShaderCode, vertShaderModule);
	this->createShaderModule(fragShaderCode, fragShaderModule);

	// Right now, the VkShaderModule object is just a
	// flat wrapper around the bytecode buffer
	// time to link them into a program!
	VkPipelineShaderStageCreateInfo vertCreateInfo = {};
	vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertCreateInfo.module = vertShaderModule;
	vertCreateInfo.pName = "main";
	// You can define your own entry point! interesting

	VkPipelineShaderStageCreateInfo fragCreateInfo = {};
	fragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragCreateInfo.module = fragShaderModule;
	fragCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertCreateInfo,
		fragCreateInfo
	};

	// Specifying vertex input

	VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
	vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindDesc = Vertex::getBindingDescription();
	auto attribDesc = Vertex::getAttributeDescriptions();

	vertInputInfo.vertexBindingDescriptionCount = 1;
	vertInputInfo.pVertexBindingDescriptions = &bindDesc;
	vertInputInfo.vertexAttributeDescriptionCount = attribDesc.size();
	vertInputInfo.pVertexAttributeDescriptions = attribDesc.data();

	// Input assembly, what kind of geometry is this?
	// fill? line? points?
	// should primitive restart be enabled?

	VkPipelineInputAssemblyStateCreateInfo inputAss = {};
	inputAss.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAss.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAss.primitiveRestartEnable = VK_FALSE;
	// If you set restarting to be enabled, youo could
	// have "breaks" between your rendered geometry,
	// by specifying something like 0xFFFF

	// Viewports and scissors

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)this->swapChainExtent.width;
	viewport.height = (float)this->swapChainExtent.height;
	viewport.minDepth = 1.0f;
	viewport.maxDepth = 0.0f;
	// Depth range for the framebuffer, 0 to 1.0 is normal

	// What region of pixels will be stored
	VkRect2D scissor = {};
	scissor.offset = { 0,0 }; // hoo, hoo, I'm an owl
	scissor.extent = this->swapChainExtent;

	// combine the viewport and scissor info
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	// Takes our stupid geometry, and fragments them
	// to be shaded later by the frag shader
	// It also performs depth testing, face culling and
	// the scissor test. neat

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	// if you clamp them, fragments outside the depth
	// bounds will be clamped instead of discarded

	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	// lol, if you set it to true, then geometry never
	// gets passed through the rasterizer, and just gets
	// discarded, giving you no results in the framebuffer

	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	// fill, line, point

	rasterizer.lineWidth = 1.0f;
	// width in terms of fragments, by the way, if you
	// want lines any thicker than 1.0f, you need to 
	// enable the "wideLines" gpu feature

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;
	// optional stuff if you set it to false
	// usually modified for shadow mapping

	// Multisampling

	VkPipelineMultisampleStateCreateInfo multiSample = {};
	multiSample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSample.sampleShadingEnable = VK_FALSE;
	multiSample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiSample.minSampleShading = 1.0f; //optional
	multiSample.pSampleMask = nullptr; //optional
	multiSample.alphaToCoverageEnable = VK_FALSE; //optional
	multiSample.alphaToOneEnable = VK_FALSE; //optional

	// Depth and stencil testing
	// Not covered at the moment, but it involves
	// VkPipeLineDepthStencilStateCreateInfo

	// Colour blending
	// after a frag shader has returned a colour, it needs
	// to be combined with the colour that's already in
	// the framebuffer. we specify how to do it, whether
	// to mix old and new, or combine using a bitwise op

	// Notice how this isn't the "createinfo" struct yet
	// this is the config attached to each framebuffer
	VkPipelineColorBlendAttachmentState colBlendAtt = {};
	colBlendAtt.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colBlendAtt.blendEnable = VK_TRUE;
	// the rest of this is optional
	colBlendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colBlendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colBlendAtt.colorBlendOp = VK_BLEND_OP_ADD;
	colBlendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colBlendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colBlendAtt.alphaBlendOp = VK_BLEND_OP_ADD;

	// Now this struct applies to the whole array of 
	// framebuffers, and lets you set blend consts that
	// you can use as blend factors
	VkPipelineColorBlendStateCreateInfo colBlend = {};
	colBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colBlend.logicOpEnable = VK_FALSE;
	colBlend.logicOp = VK_LOGIC_OP_COPY; // optional
	colBlend.attachmentCount = 1;
	colBlend.pAttachments = &colBlendAtt;
	colBlend.blendConstants[0] = 0.0f; //optional
	colBlend.blendConstants[1] = 0.0f;
	colBlend.blendConstants[2] = 0.0f;
	colBlend.blendConstants[3] = 0.0f;

	// Dynamic state
	// Here's a reliever: we don't have to recreate the
	// entire pipeline if we just want to modify certain
	// parts, with the blend ops being one!

	// Here's some
	VkDynamicState dyanamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};
	
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dyanamicStates;
	// This will cause the config of those specified
	// to be ignored at compiletime, and will need to be
	// specified at drawtime
	// Dynamic state variables are similar to uniform vars
	// - globals used within the shaders

	// Pipeline layout
	// uniform values need to be specified during pipeline
	// creation. aw man

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; //optional
	pipelineLayoutInfo.pPushConstantRanges = 0; //optional

	if (vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, this->pipelineLayout.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create pipeline layout!");
	}

	// whew, after all of that, we can combine all the 
	// structs and objects to actually make the pipeline
	// quick recap of the stuff we did up there ^
	// shader stages, fixed func states, pipeline layout,
	// and the render pass
	
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAss;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multiSample;
	pipelineInfo.pDepthStencilState = nullptr; // optional
	pipelineInfo.pColorBlendState = &colBlend;
	pipelineInfo.pDynamicState = nullptr; // optional

	// let's now ref all the structs describing the fixed
	// func stage
	pipelineInfo.layout = pipelineLayout;

	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0; // index of subpass

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; // optional
	// vulkan allows you to create a new graphics pipeline
	// by deriving from a previous existing one
	// much less expensive
	// for now we have nothing to switch back to, so leave
	// it as null

	if (vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, this->graphicsPipeline.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create graphics pipeline!");
	}
}

void HelloTriangleApp::createFrameBuffers()
{
	this->swapChainFramebuffers.resize(this->swapChainImageViews.size(), VDeleter<VkFramebuffer>{this->device, vkDestroyFramebuffer});

	for (size_t i = 0; i < this->swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = {
			this->swapChainImageViews[i]
		};

		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = this->renderPass;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = attachments;
		fbInfo.width = this->swapChainExtent.width;
		fbInfo.height = this->swapChainExtent.height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(this->device, &fbInfo, nullptr, this->swapChainFramebuffers[i].replace()) != VK_SUCCESS)
		{
			throw std::runtime_error("Couldn't create framebuffer!");
		}
	}
}

void HelloTriangleApp::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = this->findQueueFamilies(this->physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // optional
	// yep, just those 2 parameters
	// we're going to record commands for drawing, which
	// is why we've chosen our graphics queue family

	if (vkCreateCommandPool(this->device, &poolInfo, nullptr, this->commandPool.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create command pool!");
	}
}

void HelloTriangleApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VDeleter<VkBuffer>& buff, VDeleter<VkDeviceMemory>& buffMemory)
{
	// Ey! abstracting buffer creation! Optimally though,
	// you shouldn't be malloc'ing gpu memory in little
	// chunks, instead allocate a buttload first, and use
	// those handy buffer offsets to put your little buffs
	// inside! search up on vk memory management
	
	VkBufferCreateInfo buffInfo = {};
	buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffInfo.size = size;
	// size in bytes, remember! not just the size()!

	buffInfo.usage = usage;
	// what's its purpose? well lucky for you, we answered
	// that by specifying its gonna be used as a vert buff
	// you can also specify others, refer to the vulkan
	// registries and search up for the
	// "VkBufferUsageFlagBits" enum

	buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// similar to the images in the swapchain, buffers can
	// also be owned by a specific queue family, or be
	// shared between mul. simultaneously. The buffer will
	// only be used from the graphics queue, so we'll make
	// it exclusive
	// there's an optional flags param too

	if (vkCreateBuffer(this->device, &buffInfo, nullptr, buff.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create buffer!");
	}

	// alright, the buffer's been created... but there
	// aint actually any memory allocated/assigned to it
	// yet lol. first step to allocating a mem buffer is
	// to queery its memory requirements!

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(this->device, buff, &memReqs);
	// so this struct is going to have 3 data members,
	// size: memory size in bytes required for this buffer
	// alignment: the offset in bytes where the buffer
	// begins in the allocated region of memory
	// memoryTypeBits: bit field of the memory types that
	// are suitable for the buffer
	// GPU's can offer different types of mem to allocate
	// from. each mem type varies, ie allowed operations
	// as well as performance stats. let's get that info!
	// head over to this->findMemoryType();

	// physical memory allocation for our buffer

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->findMemoryType(memReqs.memoryTypeBits, properties);

	if (vkAllocateMemory(this->device, &allocInfo, nullptr, buffMemory.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create buffer memory!");
	}

	// cool! now we can associate this alloc'd memory with
	// the buffer
	vkBindBufferMemory(this->device, buff, buffMemory, 0);
	// ps: that last param is the offset within the region
	// of memory. since the mem is alloc'd specifically
	// for this vert buffer, we're havin it be 0, hurr...
	// but if you want it offsetted, it needs to be in
	// multiples of memReqs.alignment

	// filling the vertex buffer
	// time to copy that vert data over to the freshly
	// refurbished vram allocated memory!
	// aka Memory-Mapped I/O



}

void HelloTriangleApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// remember - mem transfer ops are exec. in command
	// buffers! just like drawing cmds. therefore we gotta
	// first allocate a temporary command buff... oh man
	// you might wanna create separate cmd pools for these
	// small/short operations, cause the implementation 
	// may be able to apply optimizations in terms of
	// alloc'ing. You could use
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT during
	// cmd pool creation, fyi

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = this->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuff;
	vkAllocateCommandBuffers(this->device, &allocInfo, &cmdBuff);

	// immediately start recording commands!

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	// that onetimesubmitbit flag just tells the driver
	// our intent

	vkBeginCommandBuffer(cmdBuff, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // optional
	copyRegion.dstOffset = 0; // optional
	copyRegion.size = size;
	vkCmdCopyBuffer(cmdBuff, srcBuffer, dstBuffer, 1, &copyRegion);
	// the contents of the buff are transferred via that
	// cmd. it takes the src and dest buffs as args, and
	// an array of regions to copy. the regions are
	// defined VkBufferCopy structs like this and consist
	// of the offsets of the src and dst and size. you
	// cant specify VK_WHOLE_SIZE here :(, unlike
	// vkMapMemory

	vkEndCommandBuffer(cmdBuff);
	// we're done recording here, so lets run it
	// immediately

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;

	vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(this->graphicsQueue);
	// unlike draw commands, there isnt a need for fences
	// or semaphores to wait on anything. we just gotta
	// execute! but we could use fences to schedule multi
	// simultaneous transfers! coolio

	vkFreeCommandBuffers(this->device, this->commandPool, 1, &cmdBuff);
	// dont forget to clean up after yourself!
}

void HelloTriangleApp::createVertexBuffer()
{
	VkDeviceSize buffSize = sizeof(vertices[0]) * vertices.size();

	// woosh! we're gonna only use a host-visible buffer
	// as a temporary buffer and use a device local one as
	// the actual vertex buffer
	VDeleter<VkBuffer> stagingBuff{ this->device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> stagingBuffMemory{ this->device, vkFreeMemory };

	// just using the staging buffer first!

	this->createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuff, stagingBuffMemory);

	void *data;
	vkMapMemory(this->device, stagingBuffMemory, 0, buffSize, 0, &data);
	// this func lets us access a region of the specified
	// memory resource (defined by the offset and size)
	// (which is 0 and buffInfo.size respectively). You
	// can also do VK_WHOLE_SIZE to map all the memory
	// the second last param is the flags, which dont
	// exist yet in this API version lol

	// let's simply memcpy it now!
	memcpy(data, vertices.data(), (size_t)buffSize);
	vkUnmapMemory(this->device, stagingBuffMemory);
	// we don't need to look at that disgrace anymore!
	// sadly, the driver might not copy it immediately
	// eg cause of caching. you can deal with this & other
	// problems by using a memory heap that is
	// host coherent, indicated with
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	// or, you could call vkFlushMappedMemoryRanges
	// after writing to the mapped memory, and then call
	// vkInvalidateMappedMemoryRanges before reading from
	// mapped memory
	// well, we went for the former option, which ensures
	// that the mapped memory always matches the contents
	// of the alloc'd memory. (may hit performance a bit)
	
	// Binding the vertex buffer!

	// ooh we're gonna record it in our command buffer!
	// head on over to this->createCommandBuffers();

	// ... after the staging buffer, here's the actual one

	this->createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->vertexBuffer, this->vertexBufferMemory);
	// we're now using a new staging buffer with staging
	// buffer memory for mapping & copying the vert data.
	// _TRANSFER_SRC_BIT - the buff can be used as source
	// in a mem transfer op
	// _TRANSFER_DST_BIT - the buff can be used as dest
	// in a mem transfer op

	// this->vertexBuffer is now allocated from a memory
	// type that is device local! but, this means that we
	// cant use vkMapMemory. but, we can copy data from
	// the stagingBuffer to this->vertexBuffer. we have to
	// indicate that we want to do this by specifying the
	// transfer source flag for the staging buffer and the
	// transfer dest flag for this->vertexBuffer, along w/
	// the vertex buffer usage flag

	this->copyBuffer(stagingBuff, this->vertexBuffer, buffSize);
}

void HelloTriangleApp::createCommandBuffers()
{
	// Howdy fellow kids! I'm here from recreateSwapChain,
	// and here i'm just checking if our cmd buff vec
	// already has previous buffers; if so, let's get rid/
	// free them first!
	if (this->commandBuffers.size() > 0)
	{
		vkFreeCommandBuffers(this->device, this->commandPool, this->commandBuffers.size(), this->commandBuffers.data());
	}

	this->commandBuffers.resize(this->swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)this->commandBuffers.size();
	// the level param specifies if the alloc'd buffs are
	// primary or secondary buffs.
	// primary: can be submitted to a queue for exec., but
	// cannot be called from other command buffs
	// secondary: cannot be submitted directly, but can be
	// called from primary command buffs

	if (vkAllocateCommandBuffers(this->device, &allocInfo, this->commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't allocate command buffers!");
	}

	// we begin recording a command buff by calling
	// vkBeginCommandBuffer.
	for (size_t i = 0; i < this->commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo begInfo = {};
		begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		begInfo.pInheritanceInfo = nullptr; // optional
		// for the flags param, it specifies how we're gon
		// use the command buff, you could do:
		// _one_time_submit_bit - the buff will be
		// re-recorded right after exec. it once
		// _render_pass_continue_bit - this is a secondary
		// command buff that will be entirely within a
		// single render pass
		// simultaneous_use_bit - the buff can be
		// re-submitted while it's also already pending an
		// execution

		// weeoooo weeeooooo
		vkBeginCommandBuffer(this->commandBuffers[i], &begInfo);

		VkRenderPassBeginInfo rendPassInfo = {};
		rendPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rendPassInfo.renderPass = this->renderPass;
		rendPassInfo.framebuffer = this->swapChainFramebuffers[i];

		rendPassInfo.renderArea.offset = { 0,0 }; // ey
		rendPassInfo.renderArea.extent = this->swapChainExtent;

		VkClearValue clearCol = { 0.5f, 0.5f, 0.5f, 1.0f };
		rendPassInfo.clearValueCount = 1;
		rendPassInfo.pClearValues = &clearCol;

		// ooh
		vkCmdBeginRenderPass(this->commandBuffers[i], &rendPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		// the render pass can now commence!
		// all of the funcs that record commands can be
		// recognised by their vkCmd prefix. they all
		// return void, so no error handling till we're
		// done! this is the risky life, my camaraderie
		// by the way, _inline means that the renderpass
		// commands will be embedded in the primary cmd
		// buffer itself and no secondary buff will be
		// executed. _secondary_command_buffers - the cmds
		// will be executed from secondary command buffs
		
		// Basic drawing commands:
		// I put it in this stupid scope block just to
		// let you know that you're an idiot, and this is
		// recording commands to the command buffer
		{
			// sticky!
			vkCmdBindPipeline(this->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);
			// that second enum param specifies whether
			// the pipeline object is a compute or
			// graphics pipeline

			// Heyo! I'm visiting from
			// this->createVertexBuffer();!
			VkBuffer vertBuffers[] = { this->vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(this->commandBuffers[i], 0, 1, vertBuffers, offsets);
			// that vkCmd func is used to bind vertex
			// buffers to bindings. Like the one we set up
			// previously. after that first param, the 
			// next 2 specify the offset & num of bindings
			// we're going to specify vertex buffers for.
			// the last 2 params specify the array of
			// vert buffers to bind and the byte offsets
			// to initially read from. also, down there,
			// vkCmdDraw should be changed by now to pass
			// the num of vertices in the buffer instead
			// of our magical 3 lol

			// the moment you've been waiting for,
			// duh, duh luh duh duh duh, duh luh duh duh
			// duh duh duh duh duh duh duh! duh dillie duh
			// duh dillie duh dillie duh di di duh
			// *breath*

			vkCmdDraw(this->commandBuffers[i], vertices.size(), 1, 0, 0);
			// oh
			// well, since we've done so much just now -
			// specifying all the parameters for the
			// pipeline, how to present it etc, this part
			// is just pure ease
			// those params by the way; are specifying the
			// vertex count, instance count, first vert
			// offset, and first instance offset
		}

		// just to remind you: we're not actually executing these yet, just
		// recording them, numbolini
		vkCmdEndRenderPass(this->commandBuffers[i]);

		if (vkEndCommandBuffer(this->commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
}

void HelloTriangleApp::createSemaphores()
{
	// heh, you gotta do the usual "creation struct args",
	// but at this point in time, the api just needs you
	// to specify its type lol
	
	VkSemaphoreCreateInfo semInfo = {};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(this->device, &semInfo, nullptr, this->imageAvailableSemaphore.replace()) != VK_SUCCESS ||
		vkCreateSemaphore(this->device, &semInfo, nullptr, this->renderFinishedSemaphore.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create semaphores!");
	}
}

void HelloTriangleApp::drawFrame()
{
	// Here's a brief overview of what this drawfunc will
	// do:
	// acquire an image in the swap chain
	// execute the command buffer with that image as the
	// attachment in the framebuffer
	// return the image to the swap chain for presentation
	// each of these events is used with only a single
	// function call, but are executed async.
	// the calls will return before the operations are
	// actually finished, and their order of operations is
	// also undefined. oh boy! this sucks because we need
	// them to be in order... so there's 2 ways to sync it
	// fences and semaphores. they're both objects that
	// can be used for coordinating operations by having
	// one operation signal, and another operation to wait
	// for the fence/semaphore to go from the unsignaled
	// to signaled state.
	// the diff between fences and semaphores is that
	// the state of fences can be seen/accessed from your
	// program with vkWaitForFences, and semaphores can't
	// Fences are mainly designed to sync your apps itself
	// with the rendering operation, whereas semaphores
	// are used to sync operations within or across
	// command queues. so it's more appropriate here.

	// Hey, I'm here from the future! (recreateSwapChain)
	// let's aquire the return value of vkAcNextImgKHR

	uint32_t imageIndex;
	auto result = vkAcquireNextImageKHR(
		this->device, 
		this->swapChain, 
		std::numeric_limits<uint64_t>::max(), 
		this->imageAvailableSemaphore, 
		VK_NULL_HANDLE, 
		&imageIndex);
	// third param specifies a timeout in ns for an image
	// to become available using the max value of a 64 bit
	// uint to disable the timeout
	// next 2 params specify sync objects that are to be
	// signaled when the presentation engine is finished
	// using the image. 
	// that's when we can start drawing to it! it's
	// possible to specify a semaphore; fence, or both
	// we're using our own semaphore
	// the last param specifies a var to output the index
	// of the swapchain image that has become available to
	// us. the index refers to a VkImage in this->
	// swapChainImages vector. we're gonna use that index
	// to pick the right command buffer.

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		this->recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Couldn't acquire a swapchain image!");
	}

	// submitting the command buffer

	// let's config our queue submission and syncing
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	// the three params specify which semaphores to wait
	// on before execution begins! and also in which
	// stage(s) of the pipeline to wait. we want to wait
	// with writing colours to the image until it's
	// available, so we're specifying that pipeline stage
	// here (the one that writes to the colour attachment)
	// that means that theoretically, the implementation
	// can already start executing our vertex shader etc.
	// while the image isn't available yet. each entry in
	// the waitStages array corresponds to the semaphore
	// with the same index in pWaitSemaphores

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->commandBuffers[imageIndex];
	// these params specify which command buffers to
	// actually submit for execution. we should submit
	// the command buffer that corresponds/binds to the
	// swapchain image

	VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	// these specify which semaphores to signal once the
	// command buffer(s) are done

	if (vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't submit draw command buffer!");
	}
	// the last optional arg specifies a fence that will
	// be signalled upon completion

	// subpass dependencies
	// hey idiot, remember that those subpasses in the
	// render pass automatically take care of image layout
	// transitions? yea, me neither. these transitions are
	// controlled by subpass dependencies, which specify
	// memory and exec. dependencies between subpasses
	// we've just got one subpass right now, but the ops
	// before and after this subpass are also implicit
	// subpasses. There's 2 builtin dependencies that take
	// care of the transition at the start of the render
	// pass & at the end. But the former doesnt happen at
	// the right time lol. that's why we have to override
	// those implicit dependencies with our own, using
	// vkSubpassDependency structs. head back over to
	// this->createRenderPass();
	// ...

	// Presentation
	// ey doe, this is the final step to getting our damn
	// triangle on the screen, i need a pay rise mr krabs

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	// these are the semaphores to wait on before
	// presentation can happen, just like VkSubmitInfo

	VkSwapchainKHR swapChains[] = { this->swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;
	// this is optional, as you can see. lets you specify
	// an array of VkResult to check each swap chain if
	// you really need that. but we live dangerously

	// Poof! I'm also here from recreateSwapChain,
	// lets get that return val too!

	result = vkQueuePresentKHR(this->presentQueue, &presentInfo);
	// oh man
	// submits the request to present an image
	// I ain't gonna accept your request unless you say
	// please, you nutbag

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		this->recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't present swapchain image!");
	}
}

void HelloTriangleApp::recreateSwapChain()
{
	vkDeviceWaitIdle(device);
	// we shouldnt touch resources that may still be used

	this->createSwapChain();
	this->createImageViews();
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFrameBuffers();
	this->createCommandBuffers();

	// the really handy VDeleter implements proper RAII
	// so, most of the funcs will work A-OK for re-
	// creation & will auto clean up older objects. But,
	// this->createSwapChain(); & 
	// this->createCommandBuffers(); need a bit of editing
	// woosh!
	// also, head over to our edited initWindow(), where
	// we set up our static onWindowResize() callback
}

bool QueueFamilyIndices::isComplete()
{
	return this->graphicsFamily >= 0 && this->presentFamily >= 0;
}

void HelloTriangleApp::loop()
{
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();

		// finally, some good hardcore action
		this->drawFrame();
	}

	// remember y'nubhead - all the operations in that
	// drawFrame() func are happening async'ly, so if we
	// shut down, we need to make sure everything's
	// cleaner than an abortion clinic
	vkDeviceWaitIdle(this->device);
}

// a thousand SLOC, and we just rendered a multi coloured triangle. amazing