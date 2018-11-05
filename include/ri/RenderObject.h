#pragma once

#include <string>

namespace ri
{
class TagableObject
{
public:
    void               setTagName(const std::string& name);
    void               setTagName(const char* name);
    const std::string& tagName() const;

protected:
    TagableObject()
        : m_tag("unknown")
    {
    }

    TagableObject(const char* tagName)
        : m_tag(tagName)
    {
    }

    std::string m_tag;
};

template <typename HandleClass>
class RenderObject : public TagableObject
{
protected:
    RenderObject()
        : m_handle(VK_NULL_HANDLE)
    {
    }
    RenderObject(HandleClass handle, const char* tagName = "unknown")
#ifdef NDEBUG
        : m_handle(handle)
#else
        : TagableObject(tagName)
        , m_handle(handle)

#endif  // NDEBUG
    {
    }

    HandleClass m_handle;

    template <typename HandleClass>
    friend HandleClass detail::getVkHandle(const RenderObject<HandleClass>& obj);
};

inline void TagableObject::setTagName(const std::string& name)
{
    m_tag = name;
}

inline void TagableObject::setTagName(const char* name)
{
    m_tag = name;
}

inline const std::string& TagableObject::tagName() const
{
    return m_tag;
}
}  // namespace ri
