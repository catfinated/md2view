/**
 * @brief UniformBufferObject types and methods
 */
#pragma once

#include <glm/glm.hpp>

namespace VK {

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

} // namespace VK
