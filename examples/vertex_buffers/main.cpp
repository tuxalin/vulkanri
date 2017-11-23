/**
 *
 * main.cpp vertex_buffer
 *
 * Covers the following:
 * -creating vertex and index buffers
 * -creating and using an input layout
 * -seting an indexed input layout, it's vertex binding and attributes
 * -use indexed draw commands
 * -using transfer operations with a staging buffer
 */
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include <ri/ApplicationInstance.h>
#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/DeviceContext.h>
#include <ri/InputLayout.h>
#include <ri/RenderPass.h>
#include <ri/RenderPipeline.h>
#include <ri/RenderTarget.h>
#include <ri/ShaderPipeline.h>
#include <ri/Surface.h>
#include <ri/ValidationReport.h>

const int kWidth  = 800;
const int kHeight = 600;

struct Pos
{
    float x, y;
};
struct Color
{
    float r, g, b;
};
struct Vertex
{
    Pos   pos;
    Color color;
};

const std::vector<Vertex>   kVertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                       {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                       {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                       {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
const std::vector<uint16_t> kIndices  = {0, 1, 2, 2, 3, 0};

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

        m_window = glfwCreateWindow(kWidth, kHeight, "Vertex Buffers", nullptr, nullptr);

        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, HelloTriangleApplication::onWindowResized);
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        if (width < 32 || height < 32)
            return;

        HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->resizeWindow();
    }

    void initialize()
    {
        initWindow();

        m_instance.reset(new ri::ApplicationInstance("Vertex Buffers"));
        m_validation.reset(new ri::ValidationReport(*m_instance, ri::ReportLevel::eWarning));

        m_surface.reset(  //
            new ri::Surface(*m_instance, ri::Sizei(kWidth, kHeight), m_window, ri::PresentMode::eMailbox));

        // create the device context
        {
            const std::vector<ri::DeviceFeature>   requiredFeatures   = {ri::DeviceFeature::eSwapchain};
            const std::vector<ri::DeviceOperation> requiredOperations = {ri::DeviceOperation::eGraphics,
                                                                         // required for buffer transfer
                                                                         ri::DeviceOperation::eTransfer};

            // commands will be recorded in the command buffers before render loop
            bool                  resetCommand = false;
            ri::DeviceCommandHint commandHints = ri::DeviceCommandHint::eRecorded;

            m_context.reset(new ri::DeviceContext(*m_instance, resetCommand, commandHints));
            m_context->initialize(*m_surface, requiredFeatures, requiredOperations);
        }

        // create a shader pipeline and let it own the shader modules
        {
            const std::string shadersPath = "../vertex_buffers/shaders/";
            m_shaderPipeline.reset(new ri::ShaderPipeline());
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.frag", ri::ShaderStage::eFragment));
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.vert", ri::ShaderStage::eVertex));
        }

        // create a vertex and index buffers
        {
            m_vertexBuffer.reset(new ri::Buffer(*m_context,
                                                ri::BufferUsageFlags::eVertex | ri::BufferUsageFlags::eDst,
                                                sizeof(kVertices[0]) * kVertices.size()));
            m_indexBuffer.reset(new ri::Buffer(*m_context,
                                               ri::BufferUsageFlags::eIndex | ri::BufferUsageFlags::eDst,
                                               sizeof(kIndices[0]) * kIndices.size()));

            std::unique_ptr<ri::Buffer> stagingBuffer(new ri::Buffer(
                *m_context, ri::BufferUsageFlags::eSrc, std::max(m_vertexBuffer->bytes(), m_indexBuffer->bytes())));

            /* update is equivalant to:
            auto dst = buffer->lock();
            memcpy(dst, someData.data(), buffer->bytes());
            buffer->unlock();
            */
            stagingBuffer->update(kVertices.data());
            m_vertexBuffer->copy(*stagingBuffer, m_context->commandPool());
            stagingBuffer->update(kIndices.data());
            m_indexBuffer->copy(*stagingBuffer, m_context->commandPool());

            {
                ri::InputLayout::VertexBinding binding({{0, ri::AttributeFormat::eFloat2, offsetof(Vertex, pos)},
                                                        {1, ri::AttributeFormat::eFloat4, offsetof(Vertex, color)}});
                binding.bindingIndex = 0;
                binding.buffer       = m_vertexBuffer.get();
                binding.offset       = 0;
                binding.stride       = sizeof(Vertex);
                m_inputLayout.create(binding);
                m_inputLayout.setIndexBuffer(*m_indexBuffer, ri::IndexType::eInt16);
            }
        }

        // create the render/graphics pipeline
        {
            ri::RenderPass::AttachmentParams passParams;
            passParams.format    = m_surface->format();
            ri::RenderPass* pass = new ri::RenderPass(*m_context, passParams);

            ri::RenderPipeline::CreateParams params;
            // neded to change viewport for multiple windows
            params.dynamicStates = {ri::DynamicState::eViewport, ri::DynamicState::eScissor};
            params.inputLayout   = &m_inputLayout;

            m_renderPipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
        }
    }

    void render()
    {
        m_surface->acquire();
        m_surface->present(*m_context);

        //  valdidation layer requires to be synched each frame
        if (ri::ValidationReport::kEnabled)
            m_surface->waitIdle();
    }

    void dispatchCommands(const ri::RenderTarget& target, ri::CommandBuffer& commandBuffer)
    {
        m_renderPipeline->dynamicState().setViewport(commandBuffer, target.size());
        m_renderPipeline->dynamicState().setScissor(commandBuffer, target.size());

        m_renderPipeline->begin(commandBuffer, target);
        m_inputLayout.bind(commandBuffer);
        commandBuffer.drawIndexed(kIndices.size());
        m_renderPipeline->end(commandBuffer);
    }

    void record()
    {
        // record the commands for all command buffers of the surface

        m_renderPipeline->defaultPass().setRenderArea(m_surface->size());

        for (uint32_t index = 0; index < m_surface->swapCount(); ++index)
        {
            auto& commandBuffer = m_surface->commandBuffer(index);

            commandBuffer.begin(ri::RecordFlags::eResubmit);
            dispatchCommands(m_surface->renderTarget(index), commandBuffer);
            commandBuffer.end();
        }
    }

    void mainLoop()
    {
        // pre-record all of the surface's command buffers
        record();

        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            render();
        }

        // wait for device to finish any ongoing commands for safe cleanup
        m_context->waitIdle();
    }

    void resizeWindow()
    {
        ri::Sizei size;
        glfwGetWindowSize(m_window, (int*)&size.width, (int*)&size.height);

        if (size && (size != m_surface->size()))
        {
            m_surface->recreate(*m_context, size);
            record();
            render();
        }
    }

    void cleanup()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

private:
    GLFWwindow*                              m_window;
    std::unique_ptr<ri::ApplicationInstance> m_instance;
    std::unique_ptr<ri::ValidationReport>    m_validation;
    std::unique_ptr<ri::DeviceContext>       m_context;
    std::unique_ptr<ri::Surface>             m_surface;
    std::unique_ptr<ri::ShaderPipeline>      m_shaderPipeline;
    std::unique_ptr<ri::RenderPipeline>      m_renderPipeline;
    std::unique_ptr<ri::Buffer>              m_vertexBuffer;
    std::unique_ptr<ri::Buffer>              m_indexBuffer;
    ri::IndexedInputLayout                   m_inputLayout;
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
