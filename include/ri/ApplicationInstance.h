#pragma once

#include "Config.h"
#include <vector>
#include <util/noncopyable.h>

namespace ri
{
class ApplicationInstance : util::noncopyable
{
public:
    ApplicationInstance(const std::string& name, const std::string& engineName = "");
    ~ApplicationInstance();

private:
    std::vector<const char*> getRequiredExtensions();

private:
    VkInstance m_handle;
};
}
