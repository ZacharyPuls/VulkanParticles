#pragma once

#include "GraphicsHeaders.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <fstream>

#ifdef NDEBUG
const bool ENABLE_VK_VALIDATION_LAYERS = false;
static void DebugMessage(const std::string& message) {
}
#else
const bool ENABLE_VK_VALIDATION_LAYERS = true;

static void DebugMessage(const std::string& message)
{
	auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm nowTm;
	localtime_s(&nowTm, &now);
	const auto timeString = std::put_time(&nowTm, "%Y-%m-%d %X");
	std::cerr << timeString << " [DEBUG]: " << message << std::endl;
}
#endif