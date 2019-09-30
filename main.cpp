#include "Application.h"

int main() {
	try {
		Application* particlesApp = new Application();
		particlesApp->Run();
		delete particlesApp;
	}
	catch (std::exception exception) {
		std::cerr << "Exception thrown: " << exception.what() << std::endl;
		return 100;
	}
	return 0;
}