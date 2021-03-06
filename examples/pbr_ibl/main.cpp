/**
 *
 * main.cpp pbr_ibl
 *
 * Covers the following:
 * - loading a GLTF model
 * - using multiple vertex binding for multiple meshes/primitives
 * - creating multiple descriptor set for multiple materials
 * - loading and using a texture cube for a skybox
 * - using a compute shaders
 * - using and compute pipelines to precompute maps (eg. irradiance, prefiltered, brdf lut) for IBL lighting
 * - changing the image view for a mipmap level of a texture/image target
 */
#define NOMINMAX

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include "../common/Camera.h"

#include <ri/ApplicationInstance.h>
#include <ri/Buffer.h>
#include <ri/CommandBuffer.h>
#include <ri/CommandPool.h>
#include <ri/ComputePipeline.h>
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

struct Material
{
    struct UBO
    {
        float roughness      = 0.3f;
        float metallic       = 0.f;
        float specular       = 1.f;
        float r              = 1.f;
        float g              = 1.f;
        float b              = 1.f;
        float normalStrength = 2.0f;
        float aoStrength     = 0.8f;
    };

    UBO    ubo;
    size_t albedoTexture            = 0;
    size_t metallicRoughnessTexture = 0;
    size_t normalTexture            = 1;
    size_t aoTexture                = 0;

    ri::DescriptorSet           descriptor;
    std::shared_ptr<ri::Buffer> buffer;
};

struct LightParams
{
    glm::vec4 lights[4];
    float     ambient;
    float     exposure = 1.5f;

    LightParams()
    {
        lightParams.lights[0].w = 0.5f;
        lightParams.lights[1].w = 0.3f;
        lightParams.lights[2].w = 1.0f;
        lightParams.lights[3].w = 0.33f;
    }
} lightParams;

enum
{
    eSkyboxMesh     = 0,
    eSkyboxMaterial = eSkyboxMesh,
    eMaterialOffset = eSkyboxMaterial + 1
};

namespace
{
bool loadImageData(tinygltf::Image* image, std::string* err, std::string* warn, int req_width, int req_height,
                   const unsigned char* bytes, int size, void*)
{
    // always from disk
    assert(image->uri.find_last_of(".png") != std::string::npos &&
           image->uri.find_last_of(".jpeg") != std::string::npos);
    return true;
}

bool openFile(tinygltf::Model& model, const char* filename)
{
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(&loadImageData, nullptr);

    std::string       err;
    std::string       warn;
    const std::string filepath = std::string("../resources/") + filename;
    bool              res      = loader.LoadASCIIFromFile(&model, &err, &warn, filepath.c_str());
    if (!warn.empty())
    {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cout << "ERR: " << err << std::endl;
    }

    if (!res)
        std::cout << "Failed to load glTF: " << filename << std::endl;
    else
        std::cout << "Loaded glTF: " << filename << std::endl;

    return res;
}

float getMaterialValue(const tinygltf::Material& mat, const char* propertyName, float defaultValue)
{
    auto found = mat.values.find(propertyName);
    if (found != mat.values.end())
        return (float)found->second.number_value;
    found = mat.additionalValues.find(propertyName);
    if (found != mat.additionalValues.end())
        return (float)found->second.number_value;
    return defaultValue;
}

template <typename T>
T getMaterialValue(const tinygltf::Material& mat, const char* propertyName, const char* subPropertyName, T defaultValue)
{
    auto found = mat.values.find(propertyName);
    if (found == mat.values.end())
    {
        found = mat.additionalValues.find(propertyName);
        if (found == mat.additionalValues.end())
            return defaultValue;
    }
    auto foundSub = found->second.json_double_value.find(subPropertyName);
    if (foundSub == found->second.json_double_value.end())
        return defaultValue;
    return (T)foundSub->second;
}
}

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

        m_window = glfwCreateWindow(kWidth, kHeight, "PBR IBL", nullptr, nullptr);

        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, DemoApplication::onWindowResized);
        glfwSetKeyCallback(m_window, DemoApplication::onKeyEvent);
        glfwSetScrollCallback(m_window, DemoApplication::onScrollEvent);
        glfwSetMouseButtonCallback(m_window, DemoApplication::onMouseKeyEvent);
        glfwSetCursorPosCallback(m_window, DemoApplication::onMouseMoveEvent);
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
        if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS)
            lightParams.exposure -= 0.1f;
        if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS)
            lightParams.exposure += 0.1f;

        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            app->m_useWireframe = !app->m_useWireframe;
            app->record();
        }

        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        {
            app->m_activeTexIndex++;
            if (app->m_activeTexIndex > app->m_prefilteredTexIndex)
                app->m_activeTexIndex = app->m_skyboxTexIndex;

            const ri::DescriptorSetParams descriptorParams = {
                {0, app->m_uniformBuffers[0].get(), ri::DescriptorType::eUniformBuffer},
                {1, app->m_uniformBuffers[1].get(), ri::DescriptorType::eUniformBuffer},
                {2, app->m_textures[app->m_activeTexIndex].get()}};

            app->m_materials[eSkyboxMaterial].descriptor.update<3>(descriptorParams);
            app->record();
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            app->m_camera.processKeyboard(MovementType::Forward, app->m_deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            app->m_camera.processKeyboard(MovementType::Backward, app->m_deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            app->m_camera.processKeyboard(MovementType::Left, app->m_deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            app->m_camera.processKeyboard(MovementType::Right, app->m_deltaTime);
    }

    static void onScrollEvent(GLFWwindow* window, double xoffset, double yoffset)
    {
        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        app->m_camera.processMouseScroll((float)yoffset);
    }

    static void onMouseKeyEvent(GLFWwindow* window, int key, int action, int)
    {
        if (key != GLFW_MOUSE_BUTTON_LEFT)
            return;

        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS)
            app->m_firstMouse = app->m_move = true;
        else if (action == GLFW_RELEASE)
            app->m_move = false;
    }

    static void onMouseMoveEvent(GLFWwindow* window, double xpos, double ypos)
    {
        static double lastX, lastY;

        DemoApplication* app = reinterpret_cast<DemoApplication*>(glfwGetWindowUserPointer(window));
        if (!app->m_move)
            return;

        if (app->m_firstMouse)
        {
            lastX             = xpos;
            lastY             = ypos;
            app->m_firstMouse = false;
        }

        double xoffset = xpos - lastX;
        double yoffset = lastY - ypos;  // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        app->m_camera.processMouseMovement((float)xoffset, (float)yoffset);
    }

    void initialize()
    {
        const std::string examplePath   = "../pbr_ibl/";
        const std::string resourcesPath = "../resources/";
        const std::string shadersPath   = examplePath + "shaders/";

        initWindow();

        m_instance.reset(new ri::ApplicationInstance("PBR IBL"));
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
                ri::DeviceFeature::eSwapchain, ri::DeviceFeature::eAnisotropy, ri::DeviceFeature::eWireframe};
            const std::vector<ri::DeviceOperation> requiredOperations = {ri::DeviceOperation::eGraphics,
                                                                         // required for buffer transfer
                                                                         ri::DeviceOperation::eTransfer,
                                                                         ri::DeviceOperation::eCompute};

            // command buffers will be reset upon calling begin in render loop
            ri::DeviceContext::CommandPoolParam param = {ri::DeviceCommandHint::eTransient, true};

            m_context.reset(new ri::DeviceContext(*m_instance));
            m_context->initialize(*m_surface, requiredFeatures, requiredOperations, param);
            m_context->setTagName("MainContext");
        }

        // create a shader pipeline and let it own the shader modules
        {
            m_shaderPipeline.reset(new ri::ShaderPipeline());
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.frag", ri::ShaderStage::eFragment));
            m_shaderPipeline->addStage(
                new ri::ShaderModule(*m_context, shadersPath + "shader.vert", ri::ShaderStage::eVertex));

            m_shaderPipeline->setTagName("BasicShaderPipeline");
        }

        // create a staging buffer
        {
            const size_t maxSize = 16 * 1024 * 1024 * sizeof(uint32_t);
            m_stagingBuffer.reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eSrc, maxSize));
            m_stagingBuffer->setTagName("StagingBuffer");
        }

        // default textures to load
        std::vector<std::string> textureFilePaths = {"WhiteTexture", "FlatNormalTexture"};

        using GltfTextureIndex = int;
        using TextureIndex     = size_t;
        std::map<GltfTextureIndex, TextureIndex> textureIndexMap;

        tinygltf::Model model;
        // load gltf models
        {
            tinygltf::Model skyboxModel;
            loadModel(skyboxModel, "models/Box.gltf");
            assert(m_buffers.size() == 2);
            assert(m_meshes.size() == 1);
            m_meshes[eSkyboxMesh].materialIndex = eSkyboxMaterial;

            loadModel(model, "models/MetalRoughSpheres.gltf");

            // load textures

            size_t i = 0;
            for (const tinygltf::Texture& tex : model.textures)
            {
                const tinygltf::Image& image = model.images[tex.source];
                textureFilePaths.push_back(resourcesPath + "/models/" + image.uri);
                textureIndexMap[i++] = textureFilePaths.size() - 1;
            }
        }

        // init camera
        {
            for (size_t i = 0; i < 4; ++i)
            {
                const double* data    = &model.nodes[0].matrix[i * 4];
                m_camera.ubo.model[i] = glm::vec4(data[0], data[1], data[2], data[3]);
            }

            for (int i = 0; i < 3; ++i)
            {
                m_bounds.maxSize = std::max(m_bounds.max[i] - m_bounds.min[i], m_bounds.maxSize);
            }
            m_camera.zoom = m_bounds.maxSize * 0.6f;
            m_camera.speed *= m_bounds.maxSize * 0.75f;

            m_camera.lookAt(m_camera.ubo.model * glm::vec4(m_bounds.center(), 1.f), m_camera.zoom);
            m_camera.update();
        }

        // create a one time transfer command buffer and submit commands
        auto& commandPool = m_context->commandPool(ri::DeviceOperation::eTransfer, ri::DeviceCommandHint::eRecorded);
        // create and load textures
        {
            std::array<uint32_t, 16> whiteTextureData;
            whiteTextureData.fill(0xFFFFFFFF);
            std::array<uint32_t, 16> flatNormalData;
            flatNormalData.fill(0x00FF8080);
            for (size_t i = 0; i < textureFilePaths.size(); ++i)
            {
                const std::string path = textureFilePaths[i];

                ri::Sizei size;
                int       texChannels;
                stbi_uc*  pixels;
                if (i == 0)
                {
                    pixels = (stbi_uc*)whiteTextureData.data();
                    size   = ri::Sizei(4);
                }
                else if (i == 1)
                {
                    pixels = (stbi_uc*)flatNormalData.data();
                    size   = ri::Sizei(4);
                }
                else
                    pixels =
                        stbi_load(path.c_str(), &(int&)size.width, &(int&)size.height, &texChannels, STBI_rgb_alpha);
                assert(size.pixelCount() * sizeof(uint32_t) < m_stagingBuffer->bytes());

                if (pixels == nullptr)
                    continue;

                // load texture into the staging buffer
                void* buffer = m_stagingBuffer->lock(0, size.pixelCount() * sizeof(uint32_t));
                memcpy(buffer, pixels, size.pixelCount() * sizeof(uint32_t));
                m_stagingBuffer->unlock();
                if (pixels != (const stbi_uc*)whiteTextureData.data() &&
                    pixels != (const stbi_uc*)flatNormalData.data())
                {
                    stbi_image_free(pixels);
                }

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
                copyParams.layouts = {ri::TextureLayoutType::eUndefined, ri::TextureLayoutType::eTransferSrcOptimal};
                copyParams.size    = size;

                ri::CommandBuffer commandBuffer = commandPool.begin();
                m_textures.back()->copy(*m_stagingBuffer, copyParams, commandBuffer);
                m_textures.back()->generateMipMaps(commandBuffer);
                commandPool.end(commandBuffer);
            }
        }

        ri::DescriptorSetLayout                descriptorLayouts[2];
        ri::DescriptorPool::CreateLayoutResult descriptorLayout;
        // create a descriptor pool and descriptor for the shader
        {
            std::vector<ri::DescriptorPool::TypeSize> avaialbleTypes({{ri::DescriptorType::eUniformBuffer, 30 + 1},
                                                                      {ri::DescriptorType::eCombinedSampler, 40 + 1},
                                                                      {ri::DescriptorType::eCombinedSampler, 40 + 1}});
            m_descriptorPool.reset(new ri::DescriptorPool(*m_context, 10, avaialbleTypes));

            // create descriptor layout

            ri::DescriptorLayoutParam layoutsParams({
                {0, ri::ShaderStage::eVertexFragment, ri::DescriptorType::eUniformBuffer},
                {5, ri::ShaderStage::eFragment, ri::DescriptorType::eUniformBuffer},
                {6, ri::ShaderStage::eFragment, ri::DescriptorType::eUniformBuffer},
                // pbr maps
                {1, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {2, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {3, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {4, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                // env maps
                {7, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {8, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
                {9, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
            });

            descriptorLayout     = m_descriptorPool->createLayout(layoutsParams);
            descriptorLayouts[0] = descriptorLayout.layout;
        }

        // create a uniform buffers
        {
            m_uniformBuffers[0].reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(Camera::UBO)));
            m_uniformBuffers[0]->setTagName("CameraUBO");

            m_uniformBuffers[1].reset(new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(LightParams)));
            m_uniformBuffers[1]->setTagName("LightsUBO");
        }
        size_t brfdLutTexIndex = 0;
        {
            ri::TextureParams params;
            params.type                       = ri::TextureType::e2D;
            params.size                       = ri::Sizei(512);
            params.samplerParams.addressModeU = ri::SamplerParams::eClampToEdge;
            params.samplerParams.addressModeV = ri::SamplerParams::eClampToEdge;
            params.samplerParams.minFilter    = ri::SamplerParams::eLinear;
            params.samplerParams.magFilter    = ri::SamplerParams::eLinear;
            params.format                     = ri::ColorFormat::eRG16f;
            params.mipLevels                  = 1;

            params.flags = ri::TextureUsageFlags::eDst | ri::TextureUsageFlags::eSrc |
                           // texture will be sampled in the fragment shader and used as storage in the compute shader
                           ri::TextureUsageFlags::eSampled | ri::TextureUsageFlags::eStorage;

            brfdLutTexIndex = m_textures.size();
            m_textures.emplace_back(new ri::Texture(*m_context, params));
            m_textures.back()->setTagName("BrdfLutTex");

            ri::CommandBuffer commandBuffer = commandPool.begin();
            m_textures[brfdLutTexIndex]->transitionImageLayout(ri::TextureLayoutType::eUndefined,
                                                               ri::TextureLayoutType::eGeneral, commandBuffer);
            commandPool.end(commandBuffer);
        }

        // create material for the skybox
        const unsigned int maxMipLevels = 5;
        {
            std::string filenames[6] = {"posx.png", "negx.png", "posy.png", "negy.png", "posz.png", "negz.png"};

            ri::Sizei size;
            int       texChannels;
            size_t    offset = 0;
            for (size_t i = 0; i < 6; ++i)
            {
                const std::string path = resourcesPath + "skybox/Yokohama3/" + filenames[i];
                stbi_uc*          pixels =
                    stbi_load(path.c_str(), &(int&)size.width, &(int&)size.height, &texChannels, STBI_rgb_alpha);
                assert(pixels);
                assert((offset + size.pixelCount() * sizeof(uint32_t)) < m_stagingBuffer->bytes());

                // load textures into the staging buffer
                void* buffer = m_stagingBuffer->lock(offset, size.pixelCount() * sizeof(uint32_t));
                memcpy(buffer, pixels, size.pixelCount() * sizeof(uint32_t));
                m_stagingBuffer->unlock();
                stbi_image_free(pixels);

                offset += size.pixelCount() * sizeof(uint32_t);
            }

            ri::TextureParams params;
            params.type = ri::TextureType::eCube;
            params.size = size;
            // use trilinear filtering
            params.samplerParams.minFilter  = ri::SamplerParams::eLinear;
            params.samplerParams.magFilter  = ri::SamplerParams::eLinear;
            params.samplerParams.mipmapMode = ri::SamplerParams::eLinear;

            params.flags = ri::TextureUsageFlags::eDst | ri::TextureUsageFlags::eSrc |
                           // textures will be sampled in the fragment shader and used as storage in the compute shader
                           ri::TextureUsageFlags::eSampled | ri::TextureUsageFlags::eStorage;

            m_skyboxTexIndex = m_textures.size();
            params.format    = ri::ColorFormat::eRGBA;
            params.mipLevels = maxMipLevels;
            m_textures.emplace_back(new ri::Texture(*m_context, params));
            m_textures.back()->setTagName("SkyboxCubeTex");

            m_irradianceTexIndex = m_textures.size();
            params.format        = ri::ColorFormat::eRGBA16f;
            params.mipLevels     = 1;
            m_textures.emplace_back(new ri::Texture(*m_context, params));
            m_textures.back()->setTagName("IrradianceCubeTex");

            m_prefilteredTexIndex = m_textures.size();
            params.format         = ri::ColorFormat::eRGBA16f;
            params.mipLevels      = maxMipLevels;
            m_textures.emplace_back(new ri::Texture(*m_context, params));
            m_textures.back()->setTagName("PrefilteredCubeTex");

            // issue copy and transition layout commands
            {
                ri::Texture::CopyParams copyParams;
                // using general as we will first run a compute shader to compute the irradiance map
                copyParams.layouts = {ri::TextureLayoutType::eUndefined, ri::TextureLayoutType::eGeneral};
                copyParams.size    = size;

                ri::CommandBuffer commandBuffer = commandPool.begin();

                m_textures[m_skyboxTexIndex]->copy(*m_stagingBuffer, copyParams, commandBuffer);
                m_textures[m_skyboxTexIndex]->generateMipMaps(commandBuffer);
                // use same layout
                m_textures[m_irradianceTexIndex]->transitionImageLayout(copyParams.layouts, commandBuffer);
                m_textures[m_prefilteredTexIndex]->transitionImageLayout(copyParams.layouts, commandBuffer);

                commandPool.end(commandBuffer);
            }

            ri::DescriptorLayoutParam layoutsParams({
                {0, ri::ShaderStage::eVertex, ri::DescriptorType::eUniformBuffer},
                {1, ri::ShaderStage::eFragment, ri::DescriptorType::eUniformBuffer},
                {2, ri::ShaderStage::eFragment, ri::DescriptorType::eCombinedSampler},
            });

            // create material
            assert(m_materials.size() == eSkyboxMaterial);
            m_materials.emplace_back();
            Material& material = m_materials.back();
            material.aoTexture = m_materials.size() - 1;

            m_activeTexIndex                               = m_skyboxTexIndex;
            const auto                    descriptorLayout = m_descriptorPool->createLayout(layoutsParams);
            const ri::DescriptorSetParams descriptorParams = {
                {0, m_uniformBuffers[0].get(), ri::DescriptorType::eUniformBuffer},
                {1, m_uniformBuffers[1].get(), ri::DescriptorType::eUniformBuffer},
                {2, m_textures[m_activeTexIndex].get()}};

            material.descriptor  = m_descriptorPool->create(descriptorLayout.index, descriptorParams);
            descriptorLayouts[1] = descriptorLayout.layout;
        }

        // create descriptors and materials
        {
            textureIndexMap[-1] = 0;
            textureIndexMap[-2] = 1;

            ri::DescriptorSetParams descriptorParams;
            descriptorParams.infos.reserve(16);
            descriptorParams.add(0, m_uniformBuffers[0].get(), ri::DescriptorType::eUniformBuffer);
            descriptorParams.add(5, m_uniformBuffers[1].get(), ri::DescriptorType::eUniformBuffer);
            descriptorParams.add(6, nullptr, ri::DescriptorType::eUniformBuffer);
            auto& materialUboInfo = descriptorParams.infos.back();
            descriptorParams.add(1, nullptr);
            auto& albedoMapInfo = descriptorParams.infos.back();
            descriptorParams.add(2, nullptr);
            auto& normalMapInfo = descriptorParams.infos.back();
            descriptorParams.add(3, nullptr);
            auto& metallicRoughnessMapInfo = descriptorParams.infos.back();
            descriptorParams.add(4, nullptr);
            auto& occlusionMapInfo = descriptorParams.infos.back();
            descriptorParams.add(7, m_textures[m_irradianceTexIndex].get());
            descriptorParams.add(8, m_textures[m_prefilteredTexIndex].get());
            descriptorParams.add(9, m_textures[brfdLutTexIndex].get());

            m_materials.reserve(model.materials.size());
            for (const tinygltf::Material& mat : model.materials)
            {
                m_materials.emplace_back();
                Material& material = m_materials.back();

                int index             = getMaterialValue<int>(mat, "baseColorTexture", "index", -1);
                albedoMapInfo.texture = m_textures[textureIndexMap[index]].get();

                index                       = getMaterialValue<int>(mat, "normalTexture", "index", -2);
                normalMapInfo.texture       = m_textures[textureIndexMap[index]].get();
                material.ubo.normalStrength = getMaterialValue<float>(mat, "normalTexture", "scale", 1.f);

                material.ubo.roughness           = getMaterialValue(mat, "roughnessFactor", 1.f);
                material.ubo.metallic            = getMaterialValue(mat, "metallicFactor", 1.f);
                index                            = getMaterialValue<int>(mat, "metallicRoughnessTexture", "index", -1);
                metallicRoughnessMapInfo.texture = m_textures[textureIndexMap[index]].get();

                index                    = getMaterialValue<int>(mat, "occlusionTexture", "index", -1);
                occlusionMapInfo.texture = m_textures[textureIndexMap[index]].get();
                material.ubo.aoStrength  = getMaterialValue<float>(mat, "occlusionTexture", "strength", 1.f);

                material.buffer.reset(
                    new ri::Buffer(*m_context, ri::BufferUsageFlags::eUniform, sizeof(Material::UBO)));
                material.buffer->setTagName(mat.name + "_UBO");
                materialUboInfo.buffer          = material.buffer.get();
                materialUboInfo.bufferInfo.size = material.buffer->bytes();

                material.descriptor = m_descriptorPool->create(descriptorLayout.index, descriptorParams);
            }
        }

        const auto surfaceAttachments = m_surface->attachments();
        // create the render/graphics pipeline
        {
            ri::RenderPass& pass = m_surface->renderPass();

            ri::RenderPipeline::CreateParams params;
            params.descriptorLayouts = {descriptorLayouts[0]};
            // needed to change viewport for resizing
            params.dynamicStates        = {ri::DynamicState::eViewport, ri::DynamicState::eScissor};
            params.vertexDescription    = &m_meshes[eSkyboxMesh + 1].vertexDescription;
            params.primitiveTopology    = ri::PrimitiveTopology::eTriangles;
            params.rasterizationSamples = m_surface->msaaSamples();
            params.depthCompareOp       = ri::CompareOperation::eLessOrEqual;
            params.frontFaceCW          = false;
            params.depthTestEnable      = true;
            params.depthWriteEnable     = true;

            m_renderPipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
            m_renderPipeline->setTagName("SimplePipeline");

            params.polygonMode = ri::PolygonMode::eWireframe;

            m_renderWirePipeline.reset(
                new ri::RenderPipeline(*m_context, pass, *m_shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
            m_renderWirePipeline->setTagName("WirePipeline");

            // create a render pipeline for the skybox
            {
                std::unique_ptr<ri::ShaderPipeline> shaderPipeline(new ri::ShaderPipeline());
                shaderPipeline->addStage(
                    new ri::ShaderModule(*m_context, shadersPath + "skybox.frag", ri::ShaderStage::eFragment));
                shaderPipeline->addStage(
                    new ri::ShaderModule(*m_context, shadersPath + "skybox.vert", ri::ShaderStage::eVertex));
                shaderPipeline->setTagName("SkyboxShaderPipeline");

                params.descriptorLayouts = {descriptorLayouts[1]};
                params.vertexDescription = &m_meshes[eSkyboxMesh].vertexDescription;
                params.polygonMode       = ri::PolygonMode::eNormal;
                params.frontFaceCW       = true;
                params.depthTestEnable   = true;
                params.depthWriteEnable  = false;
                m_skyboxPipeline.reset(
                    new ri::RenderPipeline(*m_context, pass, *shaderPipeline, params, ri::Sizei(kWidth, kHeight)));
                m_skyboxPipeline->setTagName("SkyboxPipeline");
            }
        }

        // create a compute pipelines for precomputing: irradiance map, prefiltered map and brdf LUT
        {
            std::unique_ptr<ri::DescriptorPool> descriptorPool;
            descriptorPool.reset(new ri::DescriptorPool(*m_context, 1 + 1 + maxMipLevels,
                                                        {{ri::DescriptorType::eImage, 2 + 1 * maxMipLevels + 1},
                                                         {ri::DescriptorType::eCombinedSampler, 1 * maxMipLevels}},
                                                        ri::DescriptorPool::eFreeDescriptorSet));

            const auto textureSize = m_textures[m_skyboxTexIndex]->size().width;

            ri::CommandBuffer                 commandBuffer = commandPool.begin();
            std::unique_ptr<ri::ShaderModule> shader;
            // execute the irradiance compute shader
            {
                ri::DescriptorLayoutParam              layoutsParams({
                    // skybox cubemap, readonly
                    {0, ri::ShaderStage::eCompute, ri::DescriptorType::eImage},
                    // irradiance cubemap, write only
                    {1, ri::ShaderStage::eCompute, ri::DescriptorType::eImage},
                });
                ri::DescriptorPool::CreateLayoutResult descriptorLayout = descriptorPool->createLayout(layoutsParams);

                shader.reset(
                    new ri::ShaderModule(*m_context, shadersPath + "irradiance.comp", ri::ShaderStage::eCompute));
                m_computePipelines[0].reset(new ri::ComputePipeline(*m_context, descriptorLayout.layout, *shader));
                m_computePipelines[0]->setTagName("IrradianceComputePipeline");

                const ri::DescriptorSetParams descriptorParams = {
                    {0, m_textures[m_skyboxTexIndex].get(), ri::DescriptorSetParams::eImage},
                    {1, m_textures[m_irradianceTexIndex].get(), ri::DescriptorSetParams::eImage}};
                ri::DescriptorSet descriptor = descriptorPool->create(descriptorLayout.index, descriptorParams);

                m_computePipelines[0]->bind(commandBuffer, descriptor);
                m_computePipelines[0]->dispatch(commandBuffer, textureSize / 16, textureSize / 16, 6);
            }

            // execute the prefiltered GGX compute shader
            {
                ri::DescriptorLayoutParam              layoutsParams({
                    // skybox cubemap, readonly
                    {0, ri::ShaderStage::eCompute, ri::DescriptorType::eCombinedSampler},
                    // prefiltered cubemap, write only
                    {1, ri::ShaderStage::eCompute, ri::DescriptorType::eImage},
                });
                ri::DescriptorPool::CreateLayoutResult descriptorLayout = descriptorPool->createLayout(layoutsParams);

                shader.reset(
                    new ri::ShaderModule(*m_context, shadersPath + "prefilterGGX.comp", ri::ShaderStage::eCompute));
                m_computePipelines[1].reset(
                    new ri::ComputePipeline(*m_context, descriptorLayout.layout, *shader,
                                            ri::ComputePipeline::PushParams(0, 3 * sizeof(float))));
                m_computePipelines[1]->setTagName("PrefilterComputePipeline");

                ri::DescriptorSetParams descriptorParams = {
                    {0, m_textures[m_skyboxTexIndex].get(), ri::DescriptorSetParams::eCombinedSampler},
                    {1, m_textures[m_prefilteredTexIndex].get(), ri::DescriptorSetParams::eImage}};

                // create the prefiltered maps based on roughness
                m_computePipelines[1]->bind(commandBuffer);
                for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
                {
                    float              params[3];
                    const unsigned int mipmapSize = textureSize >> mip;
                    const float        roughness  = (float)mip / (float)(maxMipLevels - 1);

                    params[0] = params[1] = (float)mipmapSize;
                    params[2]             = roughness;
                    m_computePipelines[1]->pushConstants(params, commandBuffer);

                    descriptorParams.infos[1].textureInfo.level = mip;  // set the mip target
                    ri::DescriptorSet descriptor = descriptorPool->create(descriptorLayout.index, descriptorParams);
                    descriptor.bind(commandBuffer, *m_computePipelines[1]);
                    m_computePipelines[1]->dispatch(commandBuffer, mipmapSize / 16, mipmapSize / 16, 6);
                }
            }

            // execute the GGX BRDF LUT compute shader
            {
                ri::DescriptorLayoutParam              layoutsParams({
                    // brdf LUT, write only
                    {0, ri::ShaderStage::eCompute, ri::DescriptorType::eImage},
                });
                ri::DescriptorPool::CreateLayoutResult descriptorLayout = descriptorPool->createLayout(layoutsParams);

                shader.reset(
                    new ri::ShaderModule(*m_context, shadersPath + "integrateGGX.comp", ri::ShaderStage::eCompute));
                m_computePipelines[2].reset(new ri::ComputePipeline(*m_context, descriptorLayout.layout, *shader));
                m_computePipelines[2]->setTagName("IntegrateBrdfComputePipeline");

                const ri::DescriptorSetParams descriptorParams = {
                    {0, m_textures[brfdLutTexIndex].get(), ri::DescriptorSetParams::eImage}};
                ri::DescriptorSet descriptor = descriptorPool->create(descriptorLayout.index, descriptorParams);

                const auto textureSize = m_textures[brfdLutTexIndex]->size().width;
                m_computePipelines[2]->bind(commandBuffer, descriptor);
                m_computePipelines[2]->dispatch(commandBuffer, textureSize / 16, textureSize / 16, 1);
            }

            // transition the cubemaps to an optimized format
            std::array<ri::TextureLayoutType, 2> layouts = {ri::TextureLayoutType::eGeneral,
                                                            ri::TextureLayoutType::eShaderReadOnly};
            m_textures[m_skyboxTexIndex]->transitionImageLayout(layouts, commandBuffer);
            m_textures[m_irradianceTexIndex]->transitionImageLayout(layouts, commandBuffer);
            m_textures[m_prefilteredTexIndex]->transitionImageLayout(layouts, commandBuffer);
            m_textures[brfdLutTexIndex]->transitionImageLayout(layouts, commandBuffer);

            commandPool.end(commandBuffer);
        }
    }

    void loadModel(tinygltf::Model& model, const char* filename)
    {
        const bool success = openFile(model, filename);
        assert(success);

        // create a transient pool for short lived buffers
        ri::DeviceContext::CommandPoolParam param = {ri::DeviceCommandHint::eTransient, false};
        auto& commandPool                         = m_context->addCommandPool(ri::DeviceOperation::eTransfer, param);

        const size_t bufferStartIndex = m_buffers.size();
        m_buffers.resize(m_buffers.size() + model.bufferViews.size());

        // create asset buffers
        for (size_t i = 0; i < model.bufferViews.size(); ++i)
        {
            const tinygltf::BufferView& bufferView = model.bufferViews[i];
            if (bufferView.target == 0)
            {
                std::cout << "WARN: bufferView.target is zero" << std::endl;
                continue;
            }

            const size_t currentIndex = bufferStartIndex + i;
            if (bufferView.target == TINYGLTF_TARGET_ARRAY_BUFFER)
            {
                m_buffers[currentIndex].reset(new ri::Buffer(
                    *m_context, ri::BufferUsageFlags::eVertex | ri::BufferUsageFlags::eDst, bufferView.byteLength));
            }
            else if (bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
            {
                m_buffers[currentIndex].reset(new ri::Buffer(
                    *m_context, ri::BufferUsageFlags::eIndex | ri::BufferUsageFlags::eDst, bufferView.byteLength));
            }
            else
                assert(false);
            assert(m_stagingBuffer->bytes() > bufferView.byteLength);

            const tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
            m_stagingBuffer->write(&buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
            m_buffers[currentIndex]->copy(*m_stagingBuffer, commandPool);
            m_buffers[currentIndex]->setTagName(buffer.name);
        }

        for (size_t i = 0; i < model.meshes.size(); ++i)
        {
            createMesh(model, model.meshes[i], bufferStartIndex);
        }
    }

    void createMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, size_t bufferStartIndex)
    {
        enum AttributeLocation
        {
            ePosition = 0,
            eNormal,
            eUV
        };

        std::map<int, ri::VertexBinding> bindings;
        std::vector<ri::VertexBinding>   bindingList;
        for (size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            const tinygltf::Primitive& primitive = mesh.primitives[i];

            bindings.clear();
            uint32_t bindingIndex = 0;
            for (auto& attrib : primitive.attributes)
            {
                const tinygltf::Accessor& accessor = model.accessors[attrib.second];

                auto& binding        = bindings[attrib.second];
                binding.bindingIndex = bindingIndex++;
                binding.buffer       = m_buffers[bufferStartIndex + accessor.bufferView].get();
                binding.offset       = accessor.byteOffset;
                binding.stride       = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

                binding.attributes.emplace_back();
                auto& bindingAttr  = binding.attributes.back();
                bindingAttr.offset = 0;  // never interleaved
                if (attrib.first.compare("POSITION") == 0)
                {
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);
                    bindingAttr.location = ePosition;
                    bindingAttr.format   = ri::AttributeFormat::eFloat3;
                    for (int i = 0; i < 3; ++i)
                    {
                        m_bounds.min[i] = std::min(m_bounds.min[i], (float)accessor.minValues[i]);
                        m_bounds.max[i] = std::max(m_bounds.max[i], (float)accessor.maxValues[i]);
                    }
                }
                else if (attrib.first.compare("NORMAL") == 0)
                {
                    assert(accessor.type == TINYGLTF_TYPE_VEC3);
                    bindingAttr.location = eNormal;
                    bindingAttr.format   = ri::AttributeFormat::eFloat3;
                }
                else if (attrib.first.compare("TEXCOORD_0") == 0)
                {
                    assert(accessor.type == TINYGLTF_TYPE_VEC2);
                    bindingAttr.location = eUV;
                    bindingAttr.format   = ri::AttributeFormat::eFloat2;
                }
                else
                    assert(false);
            }
            if (m_buffers.size() != 2)  // ignore for skybox
                assert(bindings.size() == 3);

            bindingList.clear();
            for (auto& bindingPair : bindings)
            {
                bindingList.push_back(bindingPair.second);
            }

            m_meshes.emplace_back();
            m_meshes.back().vertexDescription.create(bindingList);
            m_meshes.back().materialIndex = eMaterialOffset + primitive.material;

            tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
            const ri::Buffer&  indexBuffer   = *m_buffers[bufferStartIndex + indexAccessor.bufferView];

            ri::IndexType indexType;
            assert(indexAccessor.type == TINYGLTF_TYPE_SCALAR);
            switch (indexAccessor.componentType)
            {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    indexType = ri::IndexType::eInt16;
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    indexType = ri::IndexType::eInt32;
                    break;
                default:
                    assert(false);
                    break;
            }
            m_meshes.back().vertexDescription.setIndexBuffer(indexBuffer, indexType, indexAccessor.byteOffset,
                                                             indexAccessor.count);
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
        m_skyboxPipeline->dynamicState().setViewport(commandBuffer, target.size());
        m_skyboxPipeline->dynamicState().setScissor(commandBuffer, target.size());

        ri::RenderPass::ScopedEnable passScope(pipeline->defaultPass(), target, commandBuffer);

        pipeline->bind(commandBuffer);

        // bind the vertex and index buffers
        size_t lastMaterialIndex = m_materials.size();
        for (size_t i = eSkyboxMesh + 1; i < m_meshes.size(); ++i)
        {
            const auto& mesh = m_meshes[i];
            mesh.vertexDescription.bind(commandBuffer);

            assert(mesh.materialIndex != eSkyboxMaterial);
            if (lastMaterialIndex != mesh.materialIndex)
            {
                const Material& material = m_materials[mesh.materialIndex];
                // bind the uniform buffer/textures to the render pipeline
                material.descriptor.bind(commandBuffer, *pipeline);
                material.buffer->update(material.ubo);
                lastMaterialIndex = mesh.materialIndex;
            }

            commandBuffer.drawIndexed(mesh.vertexDescription.count());
        }

        // render skybox
        {
            m_skyboxPipeline->bind(commandBuffer);

            const auto& mesh = m_meshes[eSkyboxMesh];
            mesh.vertexDescription.bind(commandBuffer);
            m_materials[mesh.materialIndex].descriptor.bind(commandBuffer, *m_skyboxPipeline);

            commandBuffer.drawIndexed(mesh.vertexDescription.count());
        }
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

        auto        currentTime = std::chrono::high_resolution_clock::now();
        const float timer = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        m_deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_lastTime).count();
        m_lastTime  = currentTime;

        const glm::mat4 model = m_camera.ubo.model;
        if (!m_paused)
            m_camera.ubo.model = glm::rotate(model, timer, glm::vec3(0.0f, 0.0f, 1.f));

        const float zfar  = m_bounds.maxSize * 100.f;
        m_camera.ubo.proj = glm::perspective(glm::radians(45.0f),
                                             m_surface->size().width / (float)m_surface->size().height, 0.1f, zfar);
        // flip Y
        m_camera.ubo.proj[1][1] *= -1;
        m_camera.update();

        lightParams.ambient   = 0.01f;
        const float lightPos  = m_bounds.maxSize * 5.f;
        lightParams.lights[0] = glm::vec4(-lightPos, -lightPos * 0.5f, -lightPos, lightParams.lights[0].w);
        lightParams.lights[1] = glm::vec4(-lightPos, -lightPos * 0.5f, lightPos, lightParams.lights[1].w);
        lightParams.lights[2] = glm::vec4(lightPos * 0.05f, -lightPos * 0.15f, lightPos, lightParams.lights[2].w);
        lightParams.lights[3] = glm::vec4(lightPos, -lightPos * 0.5f, -lightPos, lightParams.lights[3].w);
        if (!m_lightsPaused)
        {
            const float angleDelta  = lightPos * 0.1f;
            const float a           = glm::radians(timer * 72.f);
            lightParams.lights[0].x = sin(a) * 1.0f * angleDelta;
            lightParams.lights[0].z = cos(a) * 1.5f * angleDelta;
            lightParams.lights[1].x = cos(a) * 3.0f * angleDelta;
            lightParams.lights[1].y = sin(a) * 1.5f * angleDelta;
        }

        m_uniformBuffers[0]->update(&m_camera.ubo);
        m_uniformBuffers[1]->update(&lightParams);

        m_camera.ubo.model = model;
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
    struct Mesh
    {
        ri::IndexedVertexDescription vertexDescription;
        size_t                       materialIndex = 0;
    };

    GLFWwindow*                                m_window;
    std::unique_ptr<ri::ApplicationInstance>   m_instance;
    std::unique_ptr<ri::ValidationReport>      m_validation;
    std::unique_ptr<ri::DeviceContext>         m_context;
    std::unique_ptr<ri::Surface>               m_surface;
    std::unique_ptr<ri::ShaderPipeline>        m_shaderPipeline;
    std::unique_ptr<ri::RenderPipeline>        m_renderPipeline;
    std::unique_ptr<ri::RenderPipeline>        m_renderWirePipeline;
    std::unique_ptr<ri::RenderPipeline>        m_skyboxPipeline;
    std::unique_ptr<ri::ComputePipeline>       m_computePipelines[5];
    std::unique_ptr<ri::DescriptorPool>        m_descriptorPool;
    std::unique_ptr<ri::Buffer>                m_stagingBuffer;
    std::vector<std::shared_ptr<ri::Buffer> >  m_buffers;
    std::unique_ptr<ri::Buffer>                m_uniformBuffers[2];
    std::vector<Mesh>                          m_meshes;
    std::vector<Material>                      m_materials;
    std::vector<std::shared_ptr<ri::Texture> > m_textures;
    std::unique_ptr<ri::RenderTarget>          m_msaaTarget;

    using time_t = std::chrono::time_point<std::chrono::steady_clock>;
    struct Bounds
    {
        glm::vec3 min     = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max     = glm::vec3(std::numeric_limits<float>::min());
        float     maxSize = 0.f;

        glm::vec3 center() const
        {
            glm::vec3 res = max + min;
            res *= 0.5f;
            return res;
        }
    } m_bounds;

    size_t m_skyboxTexIndex, m_irradianceTexIndex, m_prefilteredTexIndex, m_brdfTexIndex, m_activeTexIndex;
    Camera m_camera;
    float  m_deltaTime    = 0.0f;
    time_t m_lastTime     = std::chrono::high_resolution_clock::now();
    bool   m_paused       = true;
    bool   m_lightsPaused = false;
    bool   m_useWireframe = false;
    bool   m_move         = false;
    bool   m_firstMouse   = false;
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
