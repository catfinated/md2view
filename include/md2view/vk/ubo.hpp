/**
 * @brief UniformBufferObject types and methods
 */
#pragma once

#include <glm/glm.hpp>

namespace VK {

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

} // namespace VK
