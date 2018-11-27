
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum class MovementType
{
    Forward,
    Backward,
    Left,
    Right
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class Camera
{
    // Default camera values
    static constexpr float cYaw         = 0.0f;
    static constexpr float cPitch       = 0.0f;
    static constexpr float cSpeed       = 2.5f;
    static constexpr float cSensitivity = 0.01f;
    static constexpr float cZoom        = 45.0f;

public:
    struct UBO
    {
        glm::vec4 worldPos;
        glm::mat4 model = glm::mat4(1.f);
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
    };

    // Precomputed Camera Attributes
    UBO ubo;
    // Camera Attributes
    glm::vec3 position;
    glm::vec3 center;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    // Euler Angles
    float yaw;
    float pitch;
    // Camera options
    float speed;
    float sensitivity;
    float zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = cYaw, float pitch = cPitch)
        : center(glm::vec3(0.0f))
        , front(glm::vec3(0.0f, 0.0f, -1.0f))
        , speed(cSpeed)
        , sensitivity(cSensitivity)
        , zoom(cZoom)
    {
        this->position = position;
        worldUp        = up;
        this->yaw      = yaw;
        this->pitch    = pitch;
        updateCameraVectors();
    }

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
        : center(glm::vec3(0.0f))
        , front(glm::vec3(0.0f, 0.0f, -1.0f))
        , speed(cSpeed)
        , sensitivity(cSensitivity)
        , zoom(cZoom)
    {
        position    = glm::vec3(posX, posY, posZ);
        worldUp     = glm::vec3(upX, upY, upZ);
        this->yaw   = yaw;
        this->pitch = pitch;
        updateCameraVectors();
    }

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera
    // defined ENUM (to abstract it from windowing systems)
    void processKeyboard(MovementType direction, float deltaTime)
    {
        float velocity = speed * deltaTime;
        if (direction == MovementType::Forward)
            position += front * velocity;
        if (direction == MovementType::Backward)
            position -= front * velocity;
        if (direction == MovementType::Left)
            position -= right * velocity;
        if (direction == MovementType::Right)
            position += right * velocity;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void processMouseScroll(float yoffset)
    {
        zoom += yoffset * sensitivity * 10.f;
        lookAt(center, zoom);
    }

    void lookAt(const glm::vec3& center, float distance)
    {
        front        = glm::vec3(0.0f, 0.0f, -1.0f);
        position     = center + front * -distance;
        this->center = center;
        updateCameraAngles();
        updateCameraVectors();
    }

    void update()
    {
        updateCameraVectors();

        zoom = std::max(0.1f, zoom);

        ubo.view = glm::lookAt(position, position + front, up);

        ubo.viewProj = ubo.proj * ubo.view;
        ubo.worldPos = glm::vec4(position, 1.0);
    }

private:
    void updateCameraAngles()
    {
        pitch = glm::degrees(std::asin(-front.y));
        yaw   = glm::degrees(std::asin(front.z));
    }

    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x     = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y     = sin(glm::radians(pitch));
        front.z     = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        right = glm::normalize(
            glm::cross(front, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you
                                          // look up or down which results in slower movement.
        up = glm::normalize(glm::cross(right, front));
    }
};
