#include <Applications/01HelloTriangle.h>

// Throws errors like crazy if we have this define symbol
// in the header
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

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
	this->createDescriptorSetLayout();
	this->createGraphicsPipeline();
	this->createCommandPool();
	this->createDepthResources();
	this->createFrameBuffers();
	this->createTextureImage();
	this->createTextureImageView();
	this->createTextureSampler();
	this->loadModel();
	this->createVertexBuffer();
	this->createIndexBuffer();
	this->createUniformBuffer();
	this->createDescriptorPool();
	this->createDescriptorSet();
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
		// I'm here from the future! the fifth of november
		// to be exact. just removing redundancy. 
		this->createImageView(this->swapChainImages[i], this->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, this->swapChainImageViews[i]);
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

	// Hey! here from the future! adding the depth attach!
	VkAttachmentDescription depthAtt = {};
	depthAtt.format = this->findDepthFormat();
	depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// format should be same as our depth image (should!)
	// we dont really need storing depth data ATM, so
	// just ignore it. Initial and final layout remain.

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

	// Also from the future! up there! the depthAtt, this 
	// is the proper ref to it!
	VkAttachmentReference depthAttRef = {};
	depthAttRef.attachment = 1;
	depthAttRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// edit the subpassdescription too for this!
	// and the renderPassInfo below too! its an array now

	// just a struct to describe the subpass
	// ps: vk may support compute subpasses in the future!
	VkSubpassDescription subPass = {};
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colAttRef;
	subPass.pDepthStencilAttachment = &depthAttRef;
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
	dep.srcAccessMask = 0;
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

	std::array<VkAttachmentDescription, 2> atts = { colAtt, depthAtt };
	VkRenderPassCreateInfo rendPassInfo = {};
	rendPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rendPassInfo.attachmentCount = atts.size();
	rendPassInfo.pAttachments = atts.data();
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

void HelloTriangleApp::createDescriptorSetLayout()
{
	// This is just the _set_ of descriptors!
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	// first 2 params specify the binding used in shader
	// just a single struct object, so 1

	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	// we need to specify whcich shader stages the desc.
	// is gonna be referrenced. you could | combos or use
	// VK_SHADER_STAGE_ALL_GRAPHICS

	uboLayoutBinding.pImmutableSamplers = nullptr; // optional
	// Only used for image sampling related descriptors.

	// Hey! here from the future! I'm here to add the
	// combined image sampler descriptor to this set
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	// the stageFlags lets vulkan know that we intend to
	// use our combined img sampler desc. inside the frag
	// shader. That's where the ultimate colour of the
	// frag will be determined
	// Oooh, if you wanted to implement something like a 
	// heightmap, you could put the vertex shader as being
	// one of the shaders to also sample from images
	// Head over to this->createDescriptorPool and change
	// the descriptor pool size to accomodate this new
	// comb. img. sampler descriptor!

	// Future me still here! remember to do this too!
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, this->descriptorSetLayout.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create descriptor set layout!");
	}

	// TODO: uniform buff in another func
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
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	// haha oh my god, depth buffering didnt work because
	// these two little shits were switched around
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

	// Hey! from the future (when making descriptor sets)
	// just a little tweak
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
	// SYKE!
	// Hey! from the future here, with depth testing, pah,
	// and you thought we'd never cover it
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // optional
	depthStencil.maxDepthBounds = 1.0f; // optional
	depthStencil.stencilTestEnable = VK_FALSE; // for now
	depthStencil.front = {}; // optional
	depthStencil.back = {}; // optional
	// the compare operation is just configured here to be
	// lower depth = closer to us.
	// Dont forget to head down and specify pipelineInfo
	// .pDepthStencilState!

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

	// Here from the future to specify the descriptor set
	// layouts! dont forget this!
	VkDescriptorSetLayout setLayouts[] = { this->descriptorSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // optional
	pipelineLayoutInfo.pSetLayouts = setLayouts; // optional
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
	pipelineInfo.pDepthStencilState = &depthStencil;
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
		// Here from the future! depth testin'
		std::array<VkImageView, 2> attachments = {
			this->swapChainImageViews[i],
			this->depthImageView
		};

		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = this->renderPass;
		fbInfo.attachmentCount = attachments.size();
		fbInfo.pAttachments = attachments.data();
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

VkCommandBuffer HelloTriangleApp::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = this->commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuff;
	vkAllocateCommandBuffers(this->device, &allocInfo, &cmdBuff);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuff, &beginInfo);

	return cmdBuff;
}

void HelloTriangleApp::endSingleTimeCommands(VkCommandBuffer cmdBuff)
{
	vkEndCommandBuffer(cmdBuff);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;

	vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(this->graphicsQueue);

	vkFreeCommandBuffers(this->device, this->commandPool, 1, &cmdBuff);
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
	/* we've abstracted this to 
	this->begin/endSingleTimeCommands, so this is just
	for comment reference!

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
	*/

	// the magic of abstraction: stuffing junk away

	auto cmdBuff = this->beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;

	vkCmdCopyBuffer(cmdBuff, srcBuffer, dstBuffer, 1, &copyRegion);

	this->endSingleTimeCommands(cmdBuff);
}

void HelloTriangleApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	// This guy handles layout transitions! in order to
	// finish the job of making the images the correct
	// format from our inferior input one. to a totally
	// different one for gpu speeeeeeeeeeeeeeeeeeeeeeeeed

	auto cmdBuff = this->beginSingleTimeCommands();

	// a common way to perform a layout transition is to
	// use an Image Memory Barrier. pipeline barriers like
	// these are generally used to safely sync access to
	// shared resources, ie making sure a write command
	// is done before reading from it. but it can also be
	// used for this - transitioning layouts
	// (ps: theres a similar Buffer Memory Barrier too)
	
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// you gotta set this to ignored if you want to
	// explciitly say you're not going to transfer queue
	// family ownership in this process. leaving this
	// to its constructor-given-value isn't this!

	barrier.image = image;
	
	// Hey! here from the future, so that depth can be
	// used!
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (this->hasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	// so that sub struct specifies the details of the img
	// that's attached. nothing special, just magic nums

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else 
	{
		throw std::invalid_argument("Couldn't get supported layout transition!");
	}
	// barriers are usually used for sync'ing purposes, so
	// you gotta specify which types of operations that
	// involve the must happen before the barrier, and
	// operations that involve the resource must wait on
	// the barrier. we gotta do this even though
	// vkQueueWaitIdle already manually sync's for us.
	// the right values depend on the old and new layout
	// let's come back here once we've sorted out what
	// transitions we're gonna use.
	// so uh, here's what the conditionals are doing:
	// Preinitialized -> Transfer src, trans reads should
	// wait on host writes
	// Preinitialized -> Transfer dst, trans writes should
	// wait on host writes
	// Transfer Destination -> Shader reading, shader read
	// should wait on transfer writes
	// if we need more options in the future, we could
	// expand these

	vkCmdPipelineBarrier(
		cmdBuff,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);
	// first 2 params specify the pipeline stage where
	// the operations will happen that should happen
	// before/after the barrier.
	// next one can either be 0 or VK_DEP_BY_REGION_BIT,
	// which turns the barrier into a per-region
	// condition, which you can use to implement something
	// to read from certain parts of the resource that's
	// written already, for example.
	// the last 3 pairs of parameters reference arrays of
	// pipeline barriers of the 3 avaiable types:
	// - Memory barriers, - Buffer memory barriers, 
	// - Image memory barriers <- our needs!

	this->endSingleTimeCommands(cmdBuff);
}

void HelloTriangleApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VDeleter<VkImage>& image, VDeleter<VkDeviceMemory>& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1; // we'll just assume so
	imageInfo.mipLevels = 1; // yea, assume too
	imageInfo.arrayLayers = 1; // yea lol
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // assume
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// we're making lots of assumptions as we just need
	// to get a textured piece of geometry lol

	if (vkCreateImage(this->device, &imageInfo, nullptr, image.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create image!");
	}

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(this->device, image, &memReqs);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->findMemoryType(memReqs.memoryTypeBits, properties);

	if (vkAllocateMemory(this->device, &allocInfo, nullptr, imageMemory.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't allocate image memory!");
	}

	// dont forget this!
	vkBindImageMemory(this->device, image, imageMemory, 0);
}

void HelloTriangleApp::copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height)
{
	auto cmdBuff = this->beginSingleTimeCommands();
	
	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	// Just like with buffers, we gotta specify which part
	// of the image needs to be copied over.

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = { 0,0,0 }; // AGH! the 3 eye'd bug!
	region.dstOffset = { 0,0,0 }; // THERES TWO??!! NOOOO
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
		cmdBuff,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region);
	// first 2 pairs of params specify the src/dst
	// image & layout. this assumes that they've been
	// transferred to their optimal layout by now

	this->endSingleTimeCommands(cmdBuff);
}

VkFormat HelloTriangleApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	// just a bit of extra modularity/flexibility!

	for (VkFormat format : candidates)
	{
		VkFormatProperties properties;
		// for this, you could have linearTilingFeatures,
		// optimalTilingFeatures, and bufferFeatures

		vkGetPhysicalDeviceFormatProperties(this->physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	// if nothing's right lol
	// help lol
	throw std::runtime_error("Couldn't find supported format!");
}

VkFormat HelloTriangleApp::findDepthFormat()
{
	return this->findSupportedFormat(
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
}

bool HelloTriangleApp::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void HelloTriangleApp::createDepthResources()
{
	// making a depth image is actually pretty straight-
	// forward lol. keep same resolution as color attach.
	// defined by the swapchain extent, an image usage
	// that's appropriate for a depth attachment, optimal
	// tiling and device local memory. hm, what's the
	// format for a depth image tho? A: it's indicated by
	// the _D??_ format (VK_FORMAT_D??_...)
	// VK_FORMAT_D32_SFLOAT for 32 bit float depth
	// VK_FORMAT_D32_SFLOAT_S8_UINT plus 8 bit stencil!
	// VK_FORMAT_D32_UNORM_S8_UINT 24 bit float for depth
	// and 8 bit for stencil

	auto depthFormat = this->findDepthFormat();

	// now we've got sufficient info to just invoke our
	// already-made createImage and createImageView funcs!

	this->createImage(this->swapChainExtent.width, this->swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->depthImage, this->depthImageMemory);
	this->createImageView(this->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, this->depthImageView);
	// head over to that func for a second, we need to
	// pass it our aspectMask type too, because prevously
	// we just assumed we'd be using it for colour

	// wow! look at the beauty of encapsulation! now I've
	// completely forgotten how these functions work! it
	// seems to be working!

	// That's all for creating the depth image! no mapping
	// or copying to another image (staging one, nope!)
	// because we're gonna clear it at the start of the
	// renderpass like the colour attachment. But! it
	// still needs to be Transitioned to a depth-suited
	// layout. We could do this in the render pass like
	// the colour attachment, but we're doing it here,
	// suing a pipeline barrier, cause this just needs to
	// happen once every pass!

	this->transitionImageLayout(this->depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	// the undefined layout can be used as initial one, 
	// cause there aren't any existing depth img contents
	// that actually matter (me irl)

	// head over to this->transitionImageLayout, we need
	// to update it to use the right subresource aspect!
}

void HelloTriangleApp::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	// TODO: magical strings!
	stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	// 4 bytes as its got the alpha component too!

	if (!pixels)
	{
		throw std::runtime_error("Couldn't load texture image file!");
	}

	// This is our staging image, so its just in this
	// function scope
	VDeleter<VkImage> stagingImage{ this->device, vkDestroyImage };
	VDeleter<VkDeviceMemory> stagingImageMemory{ this->device, vkFreeMemory };

	/* Since we abstracted this to this->createImage(),
	we don't really need this here! just for reference

	// Here's the params to create a VkImage:
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = texWidth;
	imageInfo.extent.height = texHeight;
	imageInfo.extent.depth = 1; // its a 2d image, so just
	// one "layer" of images

	imageInfo.mipLevels = 1; // not used for now
	imageInfo.arrayLayers = 1; // not used for now
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	// make sure it matches!
	
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	// you could have _LINEAR or _OPTIMAL - impl. defined

	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	// you could have _UNDEFINED, or that.
	// undefined means its not usable by the gpu lol and
	// the first transition will discard these texels
	// _PREINITIALIZED is also not used by the gpu, but
	// the first transition won't get rid of the texels

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	// staging, so it should be the source transfer part

	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// this image will only be used by one queue family
	// if its exclusive. the family that supports
	// transfer operations
	// (yea, i need a family which supports trans 
	// operations)

	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // optional
	// samples aka multisampling. only relavent for
	// attachment images.
	// some optional flags include sparse images, and
	// doing cool stuff like telling the gpu not to alloc
	// useless memory for eg "air" texels in a 3d terrain

	if (vkCreateImage(this->device, &imageInfo, nullptr, stagingImage.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create image!");
	}

	// okie, we've created an image object, so lets alloc
	// the pixel data

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(this->device, stagingImage, &memReqs);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// very similar to a regular buffer

	if (vkAllocateMemory(this->device, &allocInfo, nullptr, stagingImageMemory.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't allocate image memory!");
	}

	// don't forget this!
	vkBindImageMemory(this->device, stagingImage, stagingImageMemory, 0);
	*/
	
	// poof! abstraction

	this->createImage(
		texWidth, 
		texHeight, 
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_TILING_LINEAR, 
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingImage, 
		stagingImageMemory);

	// let's copy the pixel data to the staging image!
	void *data;
	vkMapMemory(this->device, stagingImageMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, (size_t)imageSize);
	vkUnmapMemory(this->device, stagingImageMemory);

	// free the image data!
	stbi_image_free(pixels);

	// alright, that was the staging image, now onto the
	// real mothercucker!

	this->createImage(
		texWidth,
		texHeight,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->textureImage,
		this->textureImageMemory);
	// ooh! the USAGE_SAMPLED_BIT lets us sample the texel
	// data from the gpu shader-side!

	// Hey! I'm here from the future! we're now done with
	// the following helper functions to do the layout
	// transitioning and image copying!

	this->transitionImageLayout(
		stagingImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	this->transitionImageLayout(
		this->textureImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	this->copyImage(stagingImage, this->textureImage, texWidth, texHeight);

	// remember this! make it _SHADER_READ_ONLY_OPTIMAL
	// to allow our shader to sample it!
	this->transitionImageLayout(
		this->textureImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void HelloTriangleApp::createTextureImageView()
{
	this->createImageView(this->textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, this->textureImageView);

	// Let's head over to our abstracted createImageView
	// func. stay DRY, pupper

}

void HelloTriangleApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VDeleter<VkImageView>& imageView)
{
	// This is pretty similar to createImageViews for our
	// swapchain actually! just a few minor differences
	// with the format and image

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	// we've omitted our explicit viewInfo.components part
	// cause VK_COMPONENT_SWIZZLE_IDENTITY is defined as
	// 0 anyway.

	// note to self: debugging is a fun and rewarding
	// process. gave it this->textureImageView instead
	// lol.
	if (vkCreateImageView(this->device, &viewInfo, nullptr, imageView.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create texture image view!");
	}

	// Head over to this->createImageViews (yes, the one
	// we did several weeks ago for the swapchain!) to see
	// this used in action too
}

void HelloTriangleApp::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	// mag and min filter specify how to interpolate
	// texels that are magnified or minified. mag is over-
	// sampling and min is undersampling.

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	// can be Mirrored, Repeat, Mirrored repeat, Clamp to
	// edge, Mirror clamp to edge, Clamp to border.
	// U,V,W instead of X,Y,Z!

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	// if we've got a border, what should the border be?
	// sadly, no abritrary colour of your choice :(

	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	// ok, so this field specifies which coord system you
	// want to use to address texels in an image. if it's
	// true, you can use coords within [0,texWidth) and
	// [0,texHeight) range. if its false, then texels are
	// addressed using the [0,1) range on all axes (float)
	// usually always normalized to 0-1, as you see with
	// UV mapping!

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	// they're all mipmap related (discussed next time!)

	if (vkCreateSampler(this->device, &samplerInfo, nullptr, this->textureSampler.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create texture sampler!");
	}
}

void HelloTriangleApp::loadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	// as you know, and .obj file has pos, norm, uv's and
	// faces. faces consist of an arbitrary amount of 
	// verts, where each vert refers to a pos/norm/uv by
	// its index. reuse, reduce, recycle!
	// the attrib_t type contains all of the vectors of 
	// this data (go peek its definition, just pure 
	// vectors lol)
	// our shape vector will just hold all the individual
	// "object" shapes with face info and the pos/norm/uv
	// it uses.

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str()))
	{
		throw std::runtime_error(err);
	}

	// Let's now combine all of these faces into a single
	// model, let's concatenate all the shapes

	std::unordered_map<Vertex, int> uniqueVerts;

	for (const auto &shape : shapes)
	{
		for (const auto &index : shape.mesh.indices)
		{
			Vertex vertex = {};
			
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			// sadly, tinyobjloader's attrib.vertices 
			// array is in floats, so it isnt a glm::vec3,
			// which results in us having to multiply the
			// index by 3. likewise for the tex coords
			// below

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.norm = { 1.0f, 1.0f, 1.0f };
			// placeholder for now lol

			if (uniqueVerts.count(vertex) == 0)
			{
				uniqueVerts[vertex] = this->vertices.size();
				this->vertices.push_back(vertex);
			}
			this->indices.push_back(uniqueVerts[vertex]);
		}
	}
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

	this->createBuffer(
		buffSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuff, 
		stagingBuffMemory);

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

	this->createBuffer(
		buffSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		this->vertexBuffer, 
		this->vertexBufferMemory);
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

void HelloTriangleApp::createIndexBuffer()
{
	VkDeviceSize buffSize = sizeof(indices[0]) * indices.size();

	// Pretty similar to making the vert buffer!
	// Let's make a staging buff first

	VDeleter<VkBuffer> stagingBuff{ this->device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> stagingBuffMem{ this->device, vkFreeMemory };
	this->createBuffer(
		buffSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuff, 
		stagingBuffMem);

	void *data;
	vkMapMemory(this->device, stagingBuffMem, 0, buffSize, 0, &data);
	memcpy(data, indices.data(), (size_t)buffSize);
	vkUnmapMemory(this->device, stagingBuffMem);

	// just like the vert buffer this is the real gpu buff
	this->createBuffer(
		buffSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		this->indexBuffer, 
		this->indexBufferMemory);
	// note: look at the INDEX_BUFFER_BIT property
	// instead of vert! coolio

	this->copyBuffer(stagingBuff, this->indexBuffer, buffSize);
	// dont forget to actually copy the staging buff over!

	// now just head over to this->createCommandBuffers();
	// to bind this newly alloc'd index buffer!
}

void HelloTriangleApp::createUniformBuffer()
{
	VkDeviceSize buffSize = sizeof(UniformBufferObject);

	this->createBuffer(
		buffSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		this->uniformStagingBuffer, 
		this->uniformStagingBufferMemory);

	this->createBuffer(
		buffSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		this->uniformBuffer, 
		this->uniformBufferMemory);
	//Standard stuff!

	// Head off to this->updateUniformBuffer() which is
	// gonna be called in the main loop!
}

void HelloTriangleApp::createDescriptorPool()
{
	// This sets up the descriptors that'll be bound
	// we're gonna make a descriptor set too (not here!)
	// Descriptor sets cant be made directly, they have to
	// be allocated from a pool (like command buffers!)

	// Hey! here from the future from the creation of our
	// combined image sampler descriptor!

	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;
	// PS: when you're done looking at this, head over to
	// our also-modified createDescriptorSet func!
	
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;
	
	if (vkCreateDescriptorPool(this->device, &poolInfo, nullptr, this->descriptorPool.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create descriptor pool!");
	}
}

void HelloTriangleApp::createDescriptorSet()
{
	// You need to specify the pool to allocate from!

	VkDescriptorSetLayout layouts[] = { this->descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(this->device, &allocInfo, &this->descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("Couldn't create descriptor set!");
	}

	// Alright! so the descriptor set is allocated now!
	// but the descriptors inside themselves still need to
	// be configured!

	VkDescriptorBufferInfo buffInfo = {};
	buffInfo.buffer = this->uniformBuffer;
	buffInfo.offset = 0;
	buffInfo.range = sizeof(UniformBufferObject);
	
	// You update these descriptors using vkUpdateDescSets
	// which takes an array of vkWriteDescSet as a param

	// Hey! here from the future with combined image
	// sampler descriptor creation!
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = this->textureImageView;
	imageInfo.sampler = this->textureSampler;

	
	// Future me is also editing this to hold an array
	std::array<VkWriteDescriptorSet, 2> descWrites = {};

	// Doing the one regarding the uniform buff first
	descWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrites[0].dstSet = this->descriptorSet;
	descWrites[0].dstBinding = 0;
	descWrites[0].dstArrayElement = 0;
	descWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descWrites[0].descriptorCount = 1;
	descWrites[0].pBufferInfo = &buffInfo;
	// first 2 params set it to our descSet, and set the
	// binding to 0, cause our uniform buff binding's 0
	// the array element is just the 1st item, which is 0

	descWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrites[1].dstSet = this->descriptorSet;
	descWrites[1].dstBinding = 1;
	descWrites[1].dstArrayElement = 0;
	descWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descWrites[1].descriptorCount = 1;
	descWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(this->device, descWrites.size(), descWrites.data(), 0, nullptr);

	// Head over to this->createCommandBuffers() to add
	// vkCmdBindDescriptorSets();
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

		// Hey! here from the future with depth testing
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		// the range for the depth buffer is 0.0 to 1.0!
		// dont forget! 1.0f is at the far plane.
		// the initial value should be the furthest
		// possible value

		rendPassInfo.clearValueCount = clearValues.size();
		rendPassInfo.pClearValues = clearValues.data();

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

			// Hey! I'm from the future! here to bind the
			// index buffer as well!
			vkCmdBindIndexBuffer(this->commandBuffers[i], this->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Ey mang, I'm from this->createDescriptorSet
			// to actually bind the desc. set to the 
			// descriptors in the shader!
			vkCmdBindDescriptorSets(this->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 0, 1, &this->descriptorSet, 0, nullptr);
			// But! unlike shaders (vert, frag etc.),
			// desc sets are not unique to the pipeline!
			// so we need to specify if we want to bind
			// the desc sets to the graphics or compute
			// pipeline!
			// the next param after that is the layout
			// that the descs. are based on. the next
			// three params specify the index of the first
			// desc set, the number of sets to bind, and
			// the array of sets to bind.
			// PS: head over to createGraphicsPipeline
			// to fix a little thing we did when we 
			// flipped the clip-Y coords for MVP matrices

			// the moment you've been waiting for,
			// duh, duh luh duh duh duh, duh luh duh duh
			// duh duh duh duh duh duh duh! duh dillie duh
			// duh dillie duh dillie duh di di duh
			// *breath*

			//vkCmdDraw(this->commandBuffers[i], vertices.size(), 1, 0, 0);
			// oh
			// well, since we've done so much just now -
			// specifying all the parameters for the
			// pipeline, how to present it etc, this part
			// is just pure ease
			// those params by the way; are specifying the
			// vertex count, instance count, first vert
			// offset, and first instance offset

			// we're drawing an indexed version now!
			vkCmdDrawIndexed(this->commandBuffers[i], indices.size(), 1, 0, 0, 0);
			// just 1 instance, offset of 0 to begin with,
			// an offset of 0 per index, and an offset of
			// 0 for instancing, which wont be used atm
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

void HelloTriangleApp::updateUniformBuffer()
{
	// Updated each frame!
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	// oh, so that's how to duration cast

	// This is where we define our MVP matrices!
	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(1.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.25f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(25.0f), this->swapChainExtent.width / (float)this->swapChainExtent.height, 0.1f, 1000.0f);

	// right then. GLM was designed for opengl, and VK has
	// inverted Y clip coords
	ubo.proj[1][1] *= -1;

	// Passing it to vulkan! not very efficient though!
	void *data;
	vkMapMemory(this->device, this->uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(this->device, this->uniformStagingBufferMemory);

	this->copyBuffer(this->uniformStagingBuffer, this->uniformBuffer, sizeof(ubo));
	// If you want per-frame uploading of data, try using
	// Push Constants. may cover later :(
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
	this->createDepthResources();
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

		this->updateUniformBuffer();
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