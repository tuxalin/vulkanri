#pragma once

#include "Config.h"

#include <vector>

namespace ri
{
class ApplicationInstance
{
public:
    ApplicationInstance(const std::string& name, const std::string& engineName = "");
    ~ApplicationInstance();

private:
    std::vector<const char*> getRequiredExtensions();

private:
    VkInstance m_instance;
};
}
