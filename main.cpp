#include "stdafx.h"
#include "Application.h"

int main() {
	try {
		Application particlesApp;
		particlesApp.Run();
	}
	catch (std::exception exception) {
		std::cerr << "Exception thrown: " << exception.what() << std::endl;
		return 100;
	}
	return 0;
}