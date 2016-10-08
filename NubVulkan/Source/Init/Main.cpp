
#include <Applications/01HelloTriangle.h>

int main() {
	HelloTriangleApp app;

	try
	{
		app.run();
	}
	catch (const std::runtime_error &err)
	{
		std::cerr << err.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}