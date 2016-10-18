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
	this->createSurface();
	this->pickPhysicalDevice();
	this->createLogicalDevice();
	this->createSwapChain();
	this->createImageViews();
	this->createRenderPass();
	this->createGraphicsPipeline();
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

	if (vkCreateSwapchainKHR(this->device, &createInfo, nullptr, this->swapChain.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create a swap chain!");
	}

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
		createInfo.subresourceRange.levelCount = 2;
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

	VkRenderPassCreateInfo rendPassInfo = {};
	rendPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rendPassInfo.attachmentCount = 1;
	rendPassInfo.pAttachments = &colAtt;
	rendPassInfo.subpassCount = 1;
	rendPassInfo.pSubpasses = &subPass;

	if (vkCreateRenderPass(this->device, &rendPassInfo, nullptr, this->renderPass.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create render pass!");
	}
}

void HelloTriangleApp::createGraphicsPipeline()
{
	// For now, we've just got these 2 cute lil shaders
	auto vertShaderCode = readFile("Shaders/vert.spv");
	auto fragShaderCode = readFile("Shaders/frag.spv");

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
	// Since we're idiots at the moment, we've put the
	// vert data within the shader itself, so we can omit
	// specifying the vertex data for now

	VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
	vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertInputInfo.vertexBindingDescriptionCount = 0;
	vertInputInfo.pVertexBindingDescriptions = nullptr;
	vertInputInfo.vertexAttributeDescriptionCount = 0;
	vertInputInfo.pVertexAttributeDescriptions = nullptr;

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

bool QueueFamilyIndices::isComplete()
{
	return this->graphicsFamily >= 0 && this->presentFamily >= 0;
}

void HelloTriangleApp::loop()
{
	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
	}
}
