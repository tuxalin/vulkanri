/**
 *
 * main.cpp buffer_usage
 *
 * Covers the following:
 * -creating vertex, uniform and index buffers
 * -creating and using an input layout
 * -seting an indexed input layout, it's vertex binding and attributes
 * -use indexed draw commands
 * -using transfer operations with a staging buffer
 * -adding debug tags to resources
 * -how to create and set uniform buffers
 * -creating descriptor set and layouts via a descriptor pool
 * -using push constants
 */
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include <ri/ApplicationInstance.h>
#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/DescriptorPool.h>
#include <ri/DescriptorSet.h>
#include <ri/DeviceContext.h>
#include <ri/RenderPass.h>
#include <ri/RenderPipeline.h>
#include <ri/RenderTarget.h>
#include <ri/ShaderPipeline.h>
#include <ri/Surface.h>
#include <ri/ValidationReport.h>
#include <ri/VertexDescription.h>

const int kWidth  = 800;
const int kHeight = 600;

struct Vertex
{
    struct Pos
    {
        float x, y;
    };
    struct Color
    {
        float r, g, b;
    };
    Pos   pos;
    Color color;
};

struct Matrices
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const glm::vec3             kTintColor(0.0, 0.3, 0.15);
const std::vector<Vertex>   kVertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                       {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                       {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                       {{-0.5f, 0.5f}, {0.0f, 0.0f, 0.0f}}};
const std::vector<uint16_t> kIndices  = {0, 1, 2, 2, 3, 0};

class DemoApplication
{
public:
    DemoApplication()
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
        glfwSetWindowSizeCallback(m_window, DemoApplication::onWindowResized);
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        if (width < 32 || height < 32)
            return;

        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        app->resizeWindow();
    }

    void initialize()
    {
        initWindow();

        m_instance.reset(new ri::ApplicationInstance("Vertex Buffers"));
        m_validation.reset(new ri::ValidationReport(*m_instance, ri::ReportLevel::eWarning));

        m_surface.reset(  //
            new ri::Surface(*m_instance, ri::Sizei(kWidth, kHeight), m_window, ri::PresentMode::eMailbox));
        m_surface->setTagName("MainWindowSurface");

        // create the device context
        {
            const std::vector<ri::DeviceFeature>   requiredFeatures   = {ri::DeviceFeature::eSwapchain};
            const std::vector<ri::DeviceOperation> requiredOperations = {ri::DeviceOperation::eGraphics,
                                                                         // required for buffer transfer
                                                                         ri::DeviceOperation::eTransfer};

            // commands will be recorded in the command buffers before render loop
            bool                  resetCommand = false;
            ri::DeviceCommandHint commandHints = ri::DeviceCommandHint::eRecorded;

            m_context.reset(new ri::DeviceContext(*m_instance));
            m_context->initialize(*m_surface, requiredFeatures, requiredOperations);
            m_context->setTagName("MainContext");
        }

        // create a shader pipeline and let it own the shader modules
        {
            const std::string shadersPath = "../buffer_usage/shaders/";
            m_shaderPipeline.reset(new ri::ShaderPipeline());
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.frag", ri::ShaderStage::eFragment));
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.vert", ri::ShaderStage::eVertex));
            m_shaderPipeline->setTagName("BasicShaderPipeline");
        }

        // create a vertex and index buffers
        {
            m_vertexBuffer.reset(new ri::Buffer(*m_context,
                                                ri::BufferUsageFlags::eVertex | ri::BufferUsageFlags::eDst,
                                                sizeof(kVertices[0]) * kVertices.size()));
            m_vertexBuffer->setTagName("VertexBuffer");
            m_indexBuffer.reset(new ri::Buffer(*m_context,
                                               ri::BufferUsageFlags::eIndex | ri::BufferUsageFlags::eDst,
                                               sizeof(kIndices[0]) * kIndices.size()));
            m_vertexBuffer->setTagName("IndexBuffer");

            std::unique_ptr<ri::Buffer> stagingBuffer(new ri::Buffer(
                *m_context, ri::BufferUsageFlags::eSrc, std::max(m_vertexBuffer->bytes(), m_indexBuffer->bytes())));
            stagingBuffer->setTagName("StagingBuffer");

            // create a transient pool for short lived buffer
            ri::DeviceContext::CommandPoolParam param = {ri::DeviceCommandHint::eTransient, false};
            m_context->addCommandPool(ri::DeviceOperation::eTransfer, param);

            /* update is equivalant to:
            auto dst = buffer->lock();
            memcpy(dst, someData.data(), buffer->bytes());
            buffer->unlock();
            */
            auto& commandPool =
                m_context->commandPool(ri::DeviceOperation::eTransfer, ri::DeviceCommandHint::eTransient);
            stagingBuffer->update(kVertices.data());
            m_vertexBuffer->copy(*stagingBuffer, commandPool);
            stagingBuffer->update(kIndices.data());
            m_indexBuffer->copy(*stagingBuffer, commandPool);

            {
                ri::VertexBinding binding({{0, ri::AttributeFormat::eFloat2, offsetof(Vertex, pos)},
                                           {1, ri::AttributeFormat::eFloat4, offsetof(Vertex, color)}});
                binding.bindingIndex = 0;
                binding.buffer       = m_vertexBuffer.get();
                binding.offset       = 0;
                binding.stride       = sizeof(Vertex);
                m_vertexDescription.create(binding);
                m_vertexDescription.setIndexBuffer(*m_indexBuffer, ri::IndexType::eInt16);
                m_vertexDescription.setTagName("InputLayout");
            }
        }

        // create a uniform buffer
        {
            m_uniformBuffer.reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(Matrices)));
            m_uniformBuffer->setTagName("UniformBuffer");
        }

        ri::DescriptorSetLayout descriptorLayout;
        // create a descriptor pool
        {
            m_descriptorPool.reset(new ri::DescriptorPool(*m_context, 1, ri::DescriptorType::eUniformBuffer, 1));
            auto res = m_descriptorPool->createLayout(
                ri::DescriptorLayoutParam({0, ri::ShaderStage::eVertex, ri::DescriptorType::eUniformBuffer}));
            descriptorLayout = res.layout;

            m_descriptor = m_descriptorPool->create(
                res.index, ri::DescriptorSetParams(0, m_uniformBuffer.get(), 0, sizeof(Matrices)));
        }

        // create the render/graphics pipeline
        {
            ri::RenderPass::AttachmentParams passParams;
            passParams.format    = m_surface->format();
            ri::RenderPass* pass = new ri::RenderPass(*m_context, passParams);
            pass->setTagName("SimplePass");

            ri::RenderPipeline::CreateParams params;
            // neded to change viewport for multiple windows
            params.dynamicStates     = {ri::DynamicState::eViewport, ri::DynamicState::eScissor};
            params.vertexDescription = &m_vertexDescription;
            params.frontFaceCW       = false;  // since we inverted the Y axis
            // add a descriptor layout
            params.descriptorLayouts.push_back(descriptorLayout);
            // add a push constant for the tint color
            params.pushConstants.emplace_back(ri::ShaderStage::eVertex, 0, sizeof(kTintColor));

            m_renderPipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
            m_renderPipeline->setTagName("SimplePipeline");
        }
    }

    void render()
    {
        m_surface->acquire();
        m_surface->present(*m_context);

        //  validation layer requires to be synced each frame
        if (ri::ValidationReport::kEnabled)
            m_surface->waitIdle();
    }

    void dispatchCommands(const ri::RenderTarget& target, ri::CommandBuffer& commandBuffer)
    {
        m_renderPipeline->dynamicState().setViewport(commandBuffer, target.size());
        m_renderPipeline->dynamicState().setScissor(commandBuffer, target.size());

        m_renderPipeline->begin(commandBuffer, target);

        // bind the vertex and index buffers
        m_vertexDescription.bind(commandBuffer);
        // bind the uniform buffer to the render pipeline
        m_descriptor.bind(commandBuffer, *m_renderPipeline);
        m_renderPipeline->pushConstants(&kTintColor[0], ri::ShaderStage::eVertex, 0, sizeof(glm::vec3), commandBuffer);
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

    void update()
    {
        // update the uniform buffer

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto  currentTime = std::chrono::high_resolution_clock::now();
        float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Matrices matrices;
        matrices.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        matrices.view =
            glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        matrices.proj = glm::perspective(
            glm::radians(45.0f), m_surface->size().width / (float)m_surface->size().height, 0.1f, 10.0f);
        // flip Y
        matrices.proj[1][1] *= -1;

        m_uniformBuffer->update(&matrices);
    }

    void mainLoop()
    {
        // pre-record all of the surface's command buffers
        record();

        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            update();
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
    std::unique_ptr<ri::DescriptorPool>      m_descriptorPool;
    std::unique_ptr<ri::Buffer>              m_vertexBuffer;
    std::unique_ptr<ri::Buffer>              m_indexBuffer;
    std::unique_ptr<ri::Buffer>              m_uniformBuffer;
    ri::IndexedVertexDescription             m_vertexDescription;
    ri::DescriptorSet                        m_descriptor;
};

int main()
{
    DemoApplication app;

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
