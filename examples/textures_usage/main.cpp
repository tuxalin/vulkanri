/**
 *
 * main.cpp textures_usage
 *
 * Covers the following:
 * -creating multiple textures
 * -loading multiple textures using a staging buffer
 * -mip-map generation for the textures
 * -setting up the depth buffer
 * -enabling MSAA and sample shading
 * -a simple PBR shader with multiple lights
 */
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include <ri/ApplicationInstance.h>
#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/CommandPool.h>
#include <ri/DescriptorPool.h>
#include <ri/DescriptorSet.h>
#include <ri/DeviceContext.h>
#include <ri/RenderPass.h>
#include <ri/RenderPipeline.h>
#include <ri/RenderTarget.h>
#include <ri/ShaderPipeline.h>
#include <ri/Surface.h>
#include <ri/Texture.h>
#include <ri/ValidationReport.h>
#include <ri/VertexDescription.h>

const int kWidth  = 800;
const int kHeight = 600;

struct Vertex
{
    struct UV
    {
        float s, t;
    };
    struct Vec
    {
        float x, y, z;
    };
    Vec pos;
    UV  uv;
    Vec normal;
};

struct Camera
{
    struct UBO
    {
        glm::vec4 worldPos;
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
    };
    UBO   ubo;
    float distance = 0.5f;
};

struct Material
{
    float roughness            = 0.5f;
    float metallic             = 0.f;
    float specular             = 1.f;
    float r                    = 1.f;
    float g                    = 1.f;
    float b                    = 1.f;
    float normalStrength       = 2.0f;
    float aoStrength           = 0.8f;
    float displacementStrength = 0.015f;
    float tessLevel            = 16.f;
};

struct LightParams
{
    glm::vec4 lights[4];
    float     ambient;
};

struct PlaneModel
{
    PlaneModel()
    {
        uint16_t faceCount = 32;
        int      halfCount = faceCount / 2;

        vertices.resize((faceCount + 1) * (faceCount + 1));
        for (int i = 0, y = -halfCount; y <= halfCount; y++)
        {
            float cy = y / float(faceCount);
            float ct = cy + 0.5f;
            for (int x = -halfCount; x <= halfCount; x++, i++)
            {
                Vertex& vertex = vertices[i];
                vertex.pos.x   = x / float(faceCount);
                vertex.pos.y   = cy;
                vertex.pos.z   = 0.f;
                vertex.normal  = Vertex::Vec{0.f, 0.f, 1.f};
                vertex.uv.s    = vertex.pos.x + 0.5f;
                vertex.uv.t    = ct;
            }
        }

        indices.resize(faceCount * faceCount * 6);
        for (uint16_t ti = 0, vi = 0, y = 0; y < faceCount; y++, vi++)
        {
            for (uint16_t x = 0; x < faceCount; x++, ti += 6, vi++)
            {
                indices[ti]     = vi;
                indices[ti + 3] = indices[ti + 2] = vi + 1;
                indices[ti + 4] = indices[ti + 1] = vi + faceCount + 1;
                indices[ti + 5]                   = vi + faceCount + 2;
            }
        }
    }

    std::vector<Vertex>   vertices = {{{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f}, {0.f, 0.f, 1.f}},
                                    {{0.5f, -0.5f, 0.f}, {0.0f, 0.0f}, {0.f, 0.f, 1.f}},
                                    {{0.5f, 0.5f, 0.f}, {0.0f, 1.0f}, {0.f, 0.f, 1.f}},
                                    {{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f}, {0.f, 0.f, 1.f}}};
    std::vector<uint16_t> indices  = {0, 1, 2, 2, 3, 0};
} planeModel;

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

        glfwWindowHint(GLFW_SAMPLES, 16);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, true);

        m_window = glfwCreateWindow(kWidth, kHeight, "Texture Usage", nullptr, nullptr);

        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, DemoApplication::onWindowResized);
        glfwSetKeyCallback(m_window, DemoApplication::onKeyEvent);
        glfwSetScrollCallback(m_window, DemoApplication::onScrollEvent);
    }

    static void onWindowResized(GLFWwindow* window, int width, int height)
    {
        if (width < 32 || height < 32)
            return;

        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        app->resizeWindow();
    }

    static void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        if (key == GLFW_KEY_P && action == GLFW_PRESS)
            app->m_paused = !app->m_paused;
        if (key == GLFW_KEY_L && action == GLFW_PRESS)
            app->m_lightsPaused = !app->m_lightsPaused;
        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            app->m_useWireframe = !app->m_useWireframe;
            app->record();
        }

        if (key == GLFW_KEY_R && action == GLFW_REPEAT)
            app->m_material.roughness += 0.05f;
        if (key == GLFW_KEY_E && action == GLFW_REPEAT)
            app->m_material.roughness -= 0.05f;
        if (key == GLFW_KEY_S && action == GLFW_REPEAT)
            app->m_material.specular += 0.05f;
        if (key == GLFW_KEY_A && action == GLFW_REPEAT)
            app->m_material.specular -= 0.05f;
        if (key == GLFW_KEY_O && action == GLFW_REPEAT)
            app->m_material.aoStrength += 0.1f;
        if (key == GLFW_KEY_I && action == GLFW_REPEAT)
            app->m_material.aoStrength -= 0.1f;
        if (key == GLFW_KEY_N && action == GLFW_REPEAT)
            app->m_material.normalStrength += 0.1f;
        if (key == GLFW_KEY_M && action == GLFW_REPEAT)
            app->m_material.normalStrength -= 0.1f;
        if (key == GLFW_KEY_D && action == GLFW_REPEAT)
            app->m_material.displacementStrength += 0.001f;
        if (key == GLFW_KEY_F && action == GLFW_REPEAT)
            app->m_material.displacementStrength -= 0.001f;
        if (key == GLFW_KEY_T && action == GLFW_REPEAT)
            app->m_material.tessLevel += 0.5f;
        if (key == GLFW_KEY_Y && action == GLFW_REPEAT)
            app->m_material.tessLevel -= 0.5f;

        app->m_material.roughness            = glm::clamp(app->m_material.roughness, 0.f, 1.f);
        app->m_material.specular             = glm::clamp(app->m_material.specular, 0.f, 1.f);
        app->m_material.aoStrength           = glm::clamp(app->m_material.aoStrength, 0.f, 1.f);
        app->m_material.normalStrength       = glm::clamp(app->m_material.normalStrength, -1.f, 10.f);
        app->m_material.displacementStrength = glm::clamp(app->m_material.displacementStrength, 0.f, 0.04f);
        app->m_material.tessLevel            = glm::clamp(app->m_material.tessLevel, 1.f, 64.f);
    }

    static void onScrollEvent(GLFWwindow* window, double xoffset, double yoffset)
    {
        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        app->m_camera.distance += (float)yoffset * 0.01f;
    }

    void initialize()
    {
        const std::string examplePath = "../textures_usage/";

        initWindow();

        m_instance.reset(new ri::ApplicationInstance("Texture Usage"));
        m_validation.reset(new ri::ValidationReport(*m_instance, ri::ReportLevel::eWarning));

        {
            ri::SurfaceCreateParams params;
            params.window          = m_window;
            params.depthBufferType = ri::SurfaceCreateParams::eDepth32;
            params.msaaSamples     = 16;
            m_surface.reset(  //
                new ri::Surface(*m_instance, ri::Sizei(kWidth, kHeight), params, ri::PresentMode::eMailbox));
            m_surface->setTagName("MainWindowSurface");
        }

        // create the device context
        {
            const std::vector<ri::DeviceFeature> requiredFeatures = {
                ri::DeviceFeature::eSwapchain, ri::DeviceFeature::eAnisotropy, ri::DeviceFeature::eSampleRateShading,
                ri::DeviceFeature::eTesselationShader, ri::DeviceFeature::eWireframe};
            const std::vector<ri::DeviceOperation> requiredOperations = {ri::DeviceOperation::eGraphics,
                                                                         // required for buffer transfer
                                                                         ri::DeviceOperation::eTransfer};

            // command buffers will be reset upon calling begin in render loop
            ri::DeviceContext::CommandPoolParam param = {ri::DeviceCommandHint::eTransient, true};

            m_context.reset(new ri::DeviceContext(*m_instance));
            m_context->initialize(*m_surface, requiredFeatures, requiredOperations, param);
            m_context->setTagName("MainContext");
        }

        // create a shader pipeline and let it own the shader modules
        {
            const std::string shadersPath = examplePath + "shaders/";
            m_shaderPipeline.reset(new ri::ShaderPipeline());
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.frag", ri::ShaderStage::eFragment));
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.vert", ri::ShaderStage::eVertex));
            m_shaderPipeline->addStage(new ri::ShaderModule(*m_context, shadersPath + "displacement.tesc",
                                                            ri::ShaderStage::eTessellationControl));
            m_shaderPipeline->addStage(new ri::ShaderModule(*m_context, shadersPath + "displacement.tese",
                                                            ri::ShaderStage::eTessellationEvaluation));
            m_shaderPipeline->setTagName("BasicShaderPipeline");
        }

        // create a vertex and index buffers
        {
            m_vertexBuffer.reset(new ri::Buffer(*m_context,
                                                ri::BufferUsageFlags::eVertex | ri::BufferUsageFlags::eDst,
                                                sizeof(Vertex) * planeModel.vertices.size()));
            m_vertexBuffer->setTagName("VertexBuffer");
            m_indexBuffer.reset(new ri::Buffer(*m_context,
                                               ri::BufferUsageFlags::eIndex | ri::BufferUsageFlags::eDst,
                                               sizeof(uint16_t) * planeModel.indices.size()));
            m_vertexBuffer->setTagName("IndexBuffer");

            std::unique_ptr<ri::Buffer> stagingBuffer(new ri::Buffer(
                *m_context, ri::BufferUsageFlags::eSrc, std::max(m_vertexBuffer->bytes(), m_indexBuffer->bytes())));
            stagingBuffer->setTagName("StagingBuffer");

            // create a transient pool for short lived buffer
            ri::DeviceContext::CommandPoolParam param = {ri::DeviceCommandHint::eTransient, false};
            auto& commandPool = m_context->addCommandPool(ri::DeviceOperation::eTransfer, param);

            /* update is equivalent to:
            auto dst = buffer->lock();
            memcpy(dst, someData.data(), buffer->bytes());
            buffer->unlock();
            */
            stagingBuffer->update(planeModel.vertices.data());
            m_vertexBuffer->copy(*stagingBuffer, commandPool);
            stagingBuffer->update(planeModel.indices.data());
            m_indexBuffer->copy(*stagingBuffer, commandPool);

            {
                ri::VertexBinding binding({{0, ri::AttributeFormat::eFloat3, offsetof(Vertex, pos)},
                                           {1, ri::AttributeFormat::eFloat3, offsetof(Vertex, normal)},
                                           {2, ri::AttributeFormat::eFloat2, offsetof(Vertex, uv)}});
                binding.bindingIndex = 0;
                binding.buffer       = m_vertexBuffer.get();
                binding.offset       = 0;
                binding.stride       = sizeof(Vertex);
                m_vertexDescription.create(binding);
                m_vertexDescription.setIndexBuffer(*m_indexBuffer, ri::IndexType::eInt16);
                m_vertexDescription.setTagName("InputLayout");
            }
        }

        // create and load textures
        {
            const std::string textureFilenames[] = {"Floor_Color.png", "Floor_Normal.png", "Floor_Roughness.png",
                                                    "Floor_AO.png", "Floor_Height.png"};

            // create a staging buffer
            const ri::Sizei             maxSize(4096, 4096);
            const size_t                imageSize = maxSize.pixelCount() * 4;
            std::unique_ptr<ri::Buffer> stagingBuffer(
                new ri::Buffer(*m_context, ri::BufferUsageFlags::eSrc, imageSize));
            stagingBuffer->setTagName("TextureStagingBuffer");

            // create a one time transfer command buffer and submit commands
            auto& commandPool =
                m_context->commandPool(ri::DeviceOperation::eTransfer, ri::DeviceCommandHint::eRecorded);

            for (size_t i = 0; i < 5; ++i)
            {
                const std::string path = "../resources/textures/" + textureFilenames[i];

                ri::Sizei size;
                int       texChannels;
                stbi_uc*  pixels =
                    stbi_load(path.c_str(), &(int&)size.width, &(int&)size.height, &texChannels, STBI_rgb_alpha);
                assert(size.pixelCount() < maxSize.pixelCount());
                if (pixels == nullptr)
                    continue;

                // load texture into the staging buffer
                void* buffer = stagingBuffer->lock(0, size.pixelCount() * 4);
                memcpy(buffer, pixels, size.pixelCount() * 4);
                stagingBuffer->unlock();
                stbi_image_free(pixels);

                // create texture
                ri::TextureParams params;
                params.type   = ri::TextureType::e2D;
                params.format = ri::ColorFormat::eRGBA;
                params.size   = size;
                params.flags =
                    ri::TextureUsageFlags::eDst | ri::TextureUsageFlags::eSrc | ri::TextureUsageFlags::eSampled;
                // sampler params
                params.samplerParams.magFilter = params.samplerParams.minFilter = ri::SamplerParams::eLinear;
                params.samplerParams.anisotropyEnable                           = true;
                params.samplerParams.maxAnisotropy                              = 16.f;
                // set mip levels to zero to generate all levels
                params.mipLevels = 0;
                m_textures.emplace_back(new ri::Texture(*m_context, params));
                m_textures.back()->setTagName(path);

                // issue copy commands
                ri::Texture::CopyParams copyParams;
                copyParams.layouts = {ri::TextureLayoutType::eUndefined,  //
                                      ri::TextureLayoutType::eTransferSrcOptimal};
                copyParams.size    = size;

                ri::CommandBuffer commandBuffer = commandPool.begin();
                m_textures.back()->copy(*stagingBuffer, copyParams, commandBuffer);
                m_textures.back()->generateMipMaps(commandBuffer);
                commandPool.end(commandBuffer);
            }
        }

        // create a uniform buffers
        {
            m_uniformBuffers[0].reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(Camera)));
            m_uniformBuffers[0]->setTagName("CameraUBO");

            m_uniformBuffers[1].reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(LightParams)));
            m_uniformBuffers[1]->setTagName("LightsUBO");

            m_uniformBuffers[2].reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(Material)));
            m_uniformBuffers[2]->setTagName("MaterialUBO");
        }

        ri::RenderPipeline::CreateParams params;
        // create a descriptor pool and descriptor for the shader
        {
            std::vector<ri::DescriptorPool::TypeSize> avaialbleTypes(
                {{ri::DescriptorType::eUniformBuffer, 3}, {ri::DescriptorType::eCombinedSampler, 5}});
            m_descriptorPool.reset(new ri::DescriptorPool(*m_context, 1, avaialbleTypes));

            // create descriptor layout
            const int materialStages = ri::ShaderStage::eTessellationControl |  //
                                       ri::ShaderStage::eTessellationEvaluation | ri::ShaderStage::eFragment;
            const int cameraStages = ri::ShaderStage::eVertex |  //
                                     ri::ShaderStage::eTessellationEvaluation | ri::ShaderStage::eFragment;

            ri::DescriptorLayoutParam layoutsParams({
                {0, cameraStages, ri::DescriptorType::eUniformBuffer},
                {5, ri::ShaderStage::eFragment, ri::DescriptorType::eUniformBuffer},
                {6, materialStages, ri::DescriptorType::eUniformBuffer},
                // pbr maps
                {1, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {2, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {3, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {4, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                // displacement map
                {7, ri::ShaderStage::eTessellationEvaluation, ri::DescriptorType::eCombinedSampler},
            });

            auto res = m_descriptorPool->createLayout(layoutsParams);
            params.descriptorLayouts.push_back(res.layout);

            // create descriptor
            ri::DescriptorSetParams descriptorParams;
            descriptorParams.add(0, m_uniformBuffers[0].get(), 0, sizeof(Camera));
            descriptorParams.add(5, m_uniformBuffers[1].get(), 0, sizeof(LightParams));
            descriptorParams.add(6, m_uniformBuffers[2].get(), 0, sizeof(Material));
            descriptorParams.add(1, m_textures[0].get());
            descriptorParams.add(2, m_textures[1].get());
            descriptorParams.add(3, m_textures[2].get());
            descriptorParams.add(4, m_textures[3].get());
            descriptorParams.add(7, m_textures[4].get());

            m_descriptor = m_descriptorPool->create(res.index, descriptorParams);
        }

        const auto surfaceAttachments = m_surface->attachments();
        // create the render/graphics pipeline
        {
            ri::RenderPass& pass = m_surface->renderPass();

            // needed to change viewport for resizing
            params.dynamicStates                 = {ri::DynamicState::eViewport, ri::DynamicState::eScissor};
            params.vertexDescription             = &m_vertexDescription;
            params.primitiveTopology             = ri::PrimitiveTopology::ePatchList;
            params.rasterizationSamples          = m_surface->msaaSamples();
            params.sampleShadingEnable           = true;
            params.depthTestEnable               = true;
            params.depthWriteEnable              = true;
            params.minSampleShading              = 0.5f;
            params.tesselationPatchControlPoints = 3;

            m_renderPipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
            m_renderPipeline->setTagName("SimplePipeline");

            params.polygonMode = ri::PolygonMode::eWireframe;

            m_renderWirePipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
            m_renderWirePipeline->setTagName("WirePipeline");
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
        auto& pipeline = m_useWireframe ? m_renderWirePipeline : m_renderPipeline;

        pipeline->dynamicState().setViewport(commandBuffer, target.size());
        pipeline->dynamicState().setScissor(commandBuffer, target.size());

        ri::RenderPipeline::ScopedEnable pipelineScope(*pipeline, target, commandBuffer);

        // bind the vertex and index buffers
        m_vertexDescription.bind(commandBuffer);
        // bind the uniform buffer/textures to the render pipeline
        m_descriptor.bind(commandBuffer, *pipeline);

        commandBuffer.drawIndexed(planeModel.indices.size());
    }

    void record()
    {
        // record the commands for all command buffers of the surface

        m_surface->renderPass().setRenderArea(m_surface->size());

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
        float timer       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        if (!m_paused)
            m_camera.ubo.model = glm::rotate(glm::mat4(1.f), timer * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.f));
        else
            m_camera.ubo.model = glm::rotate(glm::mat4(1.f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.f));

        m_camera.distance = std::max(0.18f, m_camera.distance);
        m_camera.ubo.view = glm::lookAt(glm::vec3(m_camera.distance), glm::vec3(0.f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_camera.ubo.proj = glm::perspective(glm::radians(45.0f),
                                             m_surface->size().width / (float)m_surface->size().height, 0.1f, 10.0f);
        // flip Y
        m_camera.ubo.proj[1][1] *= -1;

        m_camera.ubo.viewProj = m_camera.ubo.proj * m_camera.ubo.view;
        m_camera.ubo.worldPos = glm::vec4(glm::vec3(m_camera.distance), 1.0);

        LightParams lightParams;
        lightParams.ambient = 0.04f;

        const float lightPos  = 5.f;
        lightParams.lights[0] = glm::vec4(-lightPos, -lightPos * 0.5f, 2.f * lightPos, 0.5f);
        lightParams.lights[1] = glm::vec4(-lightPos, -lightPos * 0.5f, lightPos, 0.3f);
        lightParams.lights[2] = glm::vec4(lightPos * 0.05f, -lightPos * 0.15f, lightPos, 1.0f);
        lightParams.lights[3] = glm::vec4(lightPos, -lightPos * 0.5f, 5.f * lightPos, 0.33f);
        if (!m_lightsPaused)
        {
            const float a           = glm::radians(timer * 72.f);
            lightParams.lights[0].x = sin(a) * 1.0f;
            lightParams.lights[0].z = cos(a) * 1.5f;
            lightParams.lights[1].x = cos(a) * 3.0f;
            lightParams.lights[1].y = sin(a) * 1.5f;
        }

        m_uniformBuffers[0]->update(&m_camera.ubo);
        m_uniformBuffers[1]->update(&lightParams);
        m_uniformBuffers[2]->update(&m_material);
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
    GLFWwindow*                                m_window;
    std::unique_ptr<ri::ApplicationInstance>   m_instance;
    std::unique_ptr<ri::ValidationReport>      m_validation;
    std::unique_ptr<ri::DeviceContext>         m_context;
    std::unique_ptr<ri::Surface>               m_surface;
    std::unique_ptr<ri::ShaderPipeline>        m_shaderPipeline;
    std::unique_ptr<ri::RenderPipeline>        m_renderPipeline;
    std::unique_ptr<ri::RenderPipeline>        m_renderWirePipeline;
    std::unique_ptr<ri::DescriptorPool>        m_descriptorPool;
    std::unique_ptr<ri::Buffer>                m_vertexBuffer;
    std::unique_ptr<ri::Buffer>                m_indexBuffer;
    std::unique_ptr<ri::Buffer>                m_uniformBuffers[3];
    ri::IndexedVertexDescription               m_vertexDescription;
    ri::DescriptorSet                          m_descriptor;
    std::vector<std::shared_ptr<ri::Texture> > m_textures;
    std::unique_ptr<ri::RenderTarget>          m_msaaTarget;

    Camera   m_camera;
    Material m_material;
    bool     m_paused       = true;
    bool     m_lightsPaused = false;
    bool     m_useWireframe = false;
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
