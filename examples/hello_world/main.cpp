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
#include <ri/RenderPass.h>
#include <ri/RenderPipeline.h>
#include <ri/ShaderPipeline.h>
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
        m_validation.reset(new ri::ValidationReport(*m_instance, ri::ReportLevel::eInfo));

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

        // create a shader pipeline and let it own the shader modules
        {
            const std::string shadersPath = "../hello_world/shaders/";
            m_shaderPipeline.reset(new ri::ShaderPipeline());
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.frag", ri::ShaderStage::eFragment));
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.vert", ri::ShaderStage::eVertex));
        }

        // create the render/graphics pipeline
        {
            ri::RenderPass::AttachmentParams params;
            params.format        = m_surface[0]->format();
            ri::RenderPass* pass = new ri::RenderPass(*m_context, params);

            m_renderPipeline.reset(new ri::RenderPipeline(
                *m_context, pass, *m_shaderPipeline, ri::RenderPipeline::CreateParams(), ri::Sizei(kWidth, kHeight)));
        }
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window[0]) && !glfwWindowShouldClose(m_window[1]))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        m_renderPipeline.reset();
        m_shaderPipeline.reset();
        m_surface[0].reset();
        m_surface[1].reset();
        m_context.reset();
        m_validation.reset();
        m_instance.reset();

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
    std::unique_ptr<ri::ShaderPipeline>      m_shaderPipeline;
    std::unique_ptr<ri::RenderPipeline>      m_renderPipeline;
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
