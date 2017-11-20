#pragma once

namespace ri
{
namespace detail
{
    template <typename HandleClass>
    struct RenderObject
    {
    protected:
        RenderObject()
            : m_handle(VK_NULL_HANDLE)
        {
        }
        RenderObject(HandleClass handle)
            : m_handle(handle)
        {
        }

        HandleClass m_handle;

        template <typename HandleClass>
        friend HandleClass getVkHandle(const RenderObject<HandleClass>& obj);
    };
}  // namespace detail
}  // namespace ri
