// VulkanTemplate.cpp : Defines the entry point for the console application.
//

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include <ri/ApplicationInstance.h>
#include <ri/DeviceContext.h>
#include <ri/ShaderModule.h>
#include <ri/Surface.h>
#include <ri/ValidationReport.h>

const int kWidth  = 800;
const int kHeight = 600;

class HelloTriangleApplication
{
public:
    HelloTriangleApplication()
        : m_validation(nullptr)
    {
    }

    void run()
    {
        initialize();
        mainLoop();
        cleanup();
    }

private:
    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, false);

        m_window[0] = glfwCreateWindow(kWidth, kHeight, "Hello world 1", nullptr, nullptr);
        m_window[1] = glfwCreateWindow(kWidth / 2, kHeight / 2, "Hello world 2", nullptr, nullptr);
    }

    void initialize()
    {
        initWindow();

        m_instance.reset(new ri::ApplicationInstance("Hello Triangle"));
        m_validation.reset(new ri::ValidationReport(*m_instance));

        m_context.reset(new ri::DeviceContext(*m_instance));
        m_surface[0].reset(  //
            new ri::Surface(*m_instance, ri::Sizei(kWidth, kHeight), m_window[0], ri::PresentMode::eMailbox));
        m_surface[1].reset(  //
            new ri::Surface(*m_instance, ri::Sizei(kWidth / 2, kHeight / 2), m_window[1], ri::PresentMode::eNormal));

        const std::vector<ri::DeviceFeatures>   requiredFeatures   = {ri::DeviceFeatures::eGeometryShader,
                                                                  ri::DeviceFeatures::eSwapchain};
        const std::vector<ri::DeviceOperations> requiredOperations = {ri::DeviceOperations::eGraphics,
                                                                      ri::DeviceOperations::eTransfer};

        const std::vector<ri::Surface*> surfaces = {m_surface[0].get(), m_surface[1].get()};
        m_context->create(surfaces, requiredFeatures, requiredOperations);

        const std::string shadersPath = "../hello_world/shaders/";
        m_fragShader.reset(new ri::ShaderModule(*m_context, shadersPath + "shader.frag", ri::ShaderStage::eFragment));
        m_vertShader.reset(new ri::ShaderModule(*m_context, shadersPath + "shader.vert", ri::ShaderStage::eVertex));
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window[0]))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        m_context.release();
        m_surface[0].release();
        m_surface[1].release();
        m_validation.release();
        m_validation.release();

        glfwDestroyWindow(m_window[0]);
        glfwDestroyWindow(m_window[1]);
        glfwTerminate();
    }

private:
    GLFWwindow*                              m_window[2] = {nullptr, nullptr};
    std::unique_ptr<ri::ApplicationInstance> m_instance;
    std::unique_ptr<ri::ValidationReport>    m_validation;
    std::unique_ptr<ri::DeviceContext>       m_context;
    std::unique_ptr<ri::Surface>             m_surface[2];
    std::unique_ptr<ri::ShaderModule>        m_fragShader;
    std::unique_ptr<ri::ShaderModule>        m_vertShader;
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
