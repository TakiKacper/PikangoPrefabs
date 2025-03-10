#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace nl
{
    const glm::vec3 camera_world_up = {0, 1, 0};
    struct camera;
}

struct nl::camera
{
private:
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

private:
    inline void update_directions() noexcept
    {
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        right = glm::normalize(glm::cross(front, camera_world_up));
        up = glm::normalize(glm::cross(right, front));
    }

public:
    glm::vec3 world_position;
    float zoom;
    
    float yaw;
    float pitch;

    camera(
        glm::vec3 _world_position, 
        float _yaw, 
        float _pitch
    )
    {
        world_position = _world_position;
        yaw = yaw;
        pitch = pitch;
        update_directions();
    }

    inline glm::mat4 get_view_matrix() noexcept 
    {
        update_directions();
        return glm::lookAt(world_position, world_position + front, up);
    }

    inline const glm::vec3& get_front_vector() noexcept
    {
        update_directions();
        return front;
    }

    inline const glm::vec3& get_right_vector() noexcept
    {
        update_directions();
        return right;
    }

    inline const glm::vec3& get_up_vector() noexcept
    {
        update_directions();
        return up;
    }
};

