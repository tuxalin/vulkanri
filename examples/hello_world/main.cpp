/**
 *
 * main.cpp hello_world
 *
 * Covers the following:
 * - how to initialize the render interface
 * - setup the validation report layer
 * - create two windows and their corresponding surfaces
 * - initialize a device context with the swapchain extension/feature
 * - construct the shader pipeline with a simple vertex and fragment shader
 * - construct render pipeline with a single pass
 * - use dynamic state for viewport changes for each window on the render pipeline
 * - surface acquire and presentation
 * - surface recreation upon resizing
 * - two presentation modes, recorded and transient modes
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include <ri/ApplicationInstance.h>
#include <ri/CommandBuffer.h>
#include <ri/DeviceContext.h>
#include <ri/RenderPass.h>
#include <ri/RenderPipeline.h>
#include <ri/RenderTarget.h>
#include <ri/ShaderPipeline.h>
#include <ri/Surface.h>
#include <ri/ValidationReport.h>

const int kWidth  = 800;
const int kHeight = 600;

#define RECORDED_MODE 1

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
        glfwWindowHint(GLFW_RESIZABLE, true);

        m_windows[0] = glfwCreateWindow(kWidth, kHeight, "Hello world 1", nullptr, nullptr);
        m_windows[1] = glfwCreateWindow(kWidth / 2, kHeight / 2, "Hello world 2", nullptr, nullptr);

        for (auto window : m_windows)
        {
            glfwSetWindowUserPointer(window, this);
            glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);
        }
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        if (width < 32 || height < 32)
            return;

        HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->resizeWindows();
    }

    void initialize()
    {
        initWindow();

        m_instance.reset(new ri::ApplicationInstance("Hello Triangle"));
        m_validation.reset(new ri::ValidationReport(*m_instance, ri::ReportLevel::eInfo));

        m_surfaces[0].reset(  //
            new ri::Surface(*m_instance, ri::Sizei(kWidth, kHeight), m_windows[0], ri::PresentMode::eMailbox));
        m_surfaces[1].reset(  //
            new ri::Surface(*m_instance, ri::Sizei(kWidth / 2, kHeight / 2), m_windows[1], ri::PresentMode::eNormal));

        // create the device context
        {
            const std::vector<ri::DeviceFeature>   requiredFeatures   = {ri::DeviceFeature::eSwapchain};
            const std::vector<ri::DeviceOperation> requiredOperations = {ri::DeviceOperation::eGraphics};
            const std::vector<ri::Surface*>        surfaces           = {m_surfaces[0].get(), m_surfaces[1].get()};

#if RECORDED_MODE == 1
            // commands will be recorded in the command buffers before render loop
            bool                  resetCommand = false;
            ri::DeviceCommandHint commandHints = ri::DeviceCommandHint::eRecorded;
#else
            // command buffers will be reset upon calling begin in render loop
            bool                  resetCommand = true;
            ri::DeviceCommandHint commandHints = ri::DeviceCommandHint::eTransient;
#endif  //  RECORDED_MODE == 1

            m_context.reset(new ri::DeviceContext(*m_instance, resetCommand, commandHints));
            m_context->initialize(surfaces, requiredFeatures, requiredOperations);
        }

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
            ri::RenderPass::AttachmentParams passParams;
            passParams.format    = m_surfaces[0]->format();
            ri::RenderPass* pass = new ri::RenderPass(*m_context, passParams);

            ri::RenderPipeline::CreateParams params;
            // neded to change viewport for multiple windows
            params.dynamicStates = {ri::DynamicState::eViewport, ri::DynamicState::eScissor};

            m_renderPipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
        }
    }

    void dispatchCommands(const ri::RenderTarget& target, ri::CommandBuffer& commandBuffer)
    {
        /* is equivalant to:
        auto& pass = m_renderPipeline->defaultPass();
        pass.begin(commandBuffer, target);
        m_renderPipeline->bind(commandBuffer);
        commandBuffer.draw(3, 1);
        pass.end(commandBuffer);
        */

        m_renderPipeline->dynamicState().setViewport(commandBuffer, target.size());
        m_renderPipeline->dynamicState().setScissor(commandBuffer, target.size());

        m_renderPipeline->begin(commandBuffer, target);
        commandBuffer.draw(3, 1);
        m_renderPipeline->end(commandBuffer);
    }

    void render(ri::Surface* surface)
    {
#if RECORDED_MODE == 1
        surface->acquire();
#else
        surface->waitIdle();
        uint32_t activeIndex = surface->acquire();

        auto& commandBuffer = surface->commandBuffer(activeIndex);

        // must update before binding the render pipeline
        m_renderPipeline->defaultPass().setRenderArea(surface->size());

        commandBuffer.begin(ri::RecordFlags::eResubmit);
        dispatchCommands(surface->renderTarget(activeIndex), surface->commandBuffer(activeIndex));
        commandBuffer.end();
#endif

        surface->present(*m_context);

        //  valdidation layer requires to be synched each frame
        if (ri::ValidationReport::kEnabled)
            surface->waitIdle();
    }

    void record(ri::Surface* surface)
    {
        // record the commands for all command buffers of the surface

        m_renderPipeline->defaultPass().setRenderArea(surface->size());

        for (uint32_t index = 0; index < surface->swapCount(); ++index)
        {
            auto& commandBuffer = surface->commandBuffer(index);

            commandBuffer.begin(ri::RecordFlags::eResubmit);
            dispatchCommands(surface->renderTarget(index), commandBuffer);
            commandBuffer.end();
        }
    }

    void mainLoop()
    {
        // pre-record all of the surface's command buffers
        for (auto& surface : m_surfaces)
        {
            record(surface.get());
        }

        while (!glfwWindowShouldClose(m_windows[0]) && !glfwWindowShouldClose(m_windows[1]))
        {
            glfwPollEvents();
            for (auto& surface : m_surfaces)
            {
                render(surface.get());
            }
        }

        // wait for device to finish any ongoing commands for safe cleanup
        m_context->waitIdle();
    }

    void resizeWindows()
    {
        size_t i = 0;
        for (auto window : m_windows)
        {
            ri::Sizei size;
            glfwGetWindowSize(window, (int*)&size.width, (int*)&size.height);

            auto surface = m_surfaces[i].get();
            if (size && (size != surface->size()))
            {
                surface->recreate(*m_context, size);
#if RECORDED_MODE == 1
                // must record again as buffers were reconstructed
                record(surface);
#endif
                render(surface);
            }
            ++i;
        }
    }

    void cleanup()
    {
        m_renderPipeline.reset();
        m_shaderPipeline.reset();
        m_surfaces[0].reset();
        m_surfaces[1].reset();
        m_context.reset();
        m_validation.reset();
        m_instance.reset();

        glfwDestroyWindow(m_windows[0]);
        glfwDestroyWindow(m_windows[1]);
        glfwTerminate();
    }

private:
    GLFWwindow*                              m_windows[2] = {nullptr, nullptr};
    std::unique_ptr<ri::ApplicationInstance> m_instance;
    std::unique_ptr<ri::ValidationReport>    m_validation;
    std::unique_ptr<ri::DeviceContext>       m_context;
    std::unique_ptr<ri::Surface>             m_surfaces[2];
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
