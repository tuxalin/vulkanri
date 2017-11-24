#pragma once

#include "Config.h"
#include <vector>
#include <util/noncopyable.h>

namespace ri
{
class ApplicationInstance : util::noncopyable, public RenderObject<VkInstance>
{
public:
    ApplicationInstance(const std::string& name, const std::string& engineName = "");
    ~ApplicationInstance();

private:
    std::vector<const char*> getRequiredExtensions();
};
}  // namespace ri
