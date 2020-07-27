#pragma once
#include <system_error>
#include <string>

class VulkanParticlesException : public std::system_error
{
public:
	enum class ExceptionType
	{
		SWAPCHAIN_OUT_OF_DATE,
		PRESENT_SWAPCHAIN_UNKNOWN_ERROR
	};
	
	VulkanParticlesException(const ExceptionType type, const std::string& message)
		: system_error(static_cast<int>(type), ErrorCategoryImpl::GetCategory(), message)
	{
	}

private:
	class ErrorCategoryImpl final : public std::error_category
	{
	public:
		const char* name() const noexcept override
		{
			return "VulkanParticles::Exception";
		}
		
		std::string message(int errorValue) const override
		{
			return std::to_string(errorValue);
		}

		static const std::error_category& GetCategory()
		{
			static ErrorCategoryImpl instance;
			return instance;
		}
	};
};
