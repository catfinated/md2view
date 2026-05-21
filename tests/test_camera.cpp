#include "md2view/camera.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

// Default camera: pos=(0,0,0), yaw=-90, pitch=0
// → front=(0,0,-1), right=(1,0,0), up=(0,1,0)
// → view_matrix() == glm::mat4(1.0f)
//
// glm::lookAt stores the translation in column 3 (column-major):
//   view[3][0] = -dot(right, pos)
//   view[3][1] = -dot(up,    pos)
//   view[3][2] =  dot(front, pos)
//
// Movement speed is 3.0f, so move(dir, dt=1.0f) displaces by 3 units.

TEST_CASE("camera default ctor", "[cam]") {
    Camera camera;
    REQUIRE(camera.fov() == Camera::FOV);
    REQUIRE(camera.view_dirty());
    REQUIRE(camera.fov_dirty());
}

TEST_CASE("camera default accessors", "[cam]") {
    using Catch::Approx;
    Camera camera;
    // Default: pos=origin, yaw=-90, pitch=0 → front=(0,0,-1), right=(1,0,0),
    // up=(0,1,0)
    REQUIRE(camera.position() == glm::vec3(0.0f));
    REQUIRE(camera.front().x == Approx(0.0f).margin(1e-5f));
    REQUIRE(camera.front().y == Approx(0.0f).margin(1e-5f));
    REQUIRE(camera.front().z == Approx(-1.0f).margin(1e-5f));
    REQUIRE(camera.right().x == Approx(1.0f).margin(1e-5f));
    REQUIRE(camera.right().y == Approx(0.0f).margin(1e-5f));
    REQUIRE(camera.right().z == Approx(0.0f).margin(1e-5f));
    REQUIRE(camera.up().x == Approx(0.0f).margin(1e-5f));
    REQUIRE(camera.up().y == Approx(1.0f).margin(1e-5f));
    REQUIRE(camera.up().z == Approx(0.0f).margin(1e-5f));
    REQUIRE(camera.pitch() == Approx(Camera::PITCH));
    REQUIRE(camera.yaw() == Approx(Camera::YAW));
}

TEST_CASE("camera set_fov marks fov dirty", "[cam]") {
    Camera camera;
    camera.set_fov_clean();
    camera.set_fov(30.0f);
    REQUIRE(camera.fov() == Catch::Approx(30.0f));
    REQUIRE(camera.fov_dirty());
}

TEST_CASE("camera position updated after move", "[cam]") {
    Camera camera;
    camera.move(Camera::Direction::FORWARD, 1.0f);
    REQUIRE(camera.position().z == Catch::Approx(-3.0f).margin(1e-5f));
}

TEST_CASE("camera pitch and yaw updated after mouse movement", "[cam]") {
    using Catch::Approx;
    Camera camera;
    // sensitivity=0.25; xoffset=4 → yaw += 1.0; yoffset=4 → pitch += 1.0
    camera.on_mouse_movement(4.0f, 4.0f);
    REQUIRE(camera.yaw() == Approx(Camera::YAW + 1.0f).margin(1e-5f));
    REQUIRE(camera.pitch() == Approx(Camera::PITCH + 1.0f).margin(1e-5f));
}

TEST_CASE("camera set_view_clean clears flag", "[cam]") {
    Camera camera;
    camera.set_view_clean();
    REQUIRE_FALSE(camera.view_dirty());
}

TEST_CASE("camera set_fov_clean clears flag", "[cam]") {
    Camera camera;
    camera.set_fov_clean();
    REQUIRE_FALSE(camera.fov_dirty());
}

TEST_CASE("camera view matrix at default is identity", "[cam]") {
    using Catch::Approx;
    Camera camera;
    auto const view = camera.view_matrix();
    auto const expected = glm::mat4(1.0f);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            REQUIRE(view[c][r] == Approx(expected[c][r]).margin(1e-5f));
}

TEST_CASE("camera move forward", "[cam]") {
    using Catch::Approx;
    Camera camera;
    camera.move(Camera::Direction::FORWARD, 1.0f);
    REQUIRE(camera.view_dirty());
    // pos=(0,0,-3): dot(front=(0,0,-1), pos) = 3
    REQUIRE(camera.view_matrix()[3][2] == Approx(3.0f).margin(1e-5f));
}

TEST_CASE("camera move backward", "[cam]") {
    using Catch::Approx;
    Camera camera;
    camera.move(Camera::Direction::BACKWARD, 1.0f);
    REQUIRE(camera.view_dirty());
    // pos=(0,0,3): dot(front=(0,0,-1), pos) = -3
    REQUIRE(camera.view_matrix()[3][2] == Approx(-3.0f).margin(1e-5f));
}

TEST_CASE("camera move left", "[cam]") {
    using Catch::Approx;
    Camera camera;
    camera.move(Camera::Direction::LEFT, 1.0f);
    REQUIRE(camera.view_dirty());
    // pos=(-3,0,0): -dot(right=(1,0,0), pos) = 3
    REQUIRE(camera.view_matrix()[3][0] == Approx(3.0f).margin(1e-5f));
}

TEST_CASE("camera move right", "[cam]") {
    using Catch::Approx;
    Camera camera;
    camera.move(Camera::Direction::RIGHT, 1.0f);
    REQUIRE(camera.view_dirty());
    // pos=(3,0,0): -dot(right=(1,0,0), pos) = -3
    REQUIRE(camera.view_matrix()[3][0] == Approx(-3.0f).margin(1e-5f));
}

TEST_CASE("camera mouse movement sets view dirty", "[cam]") {
    Camera camera;
    camera.set_view_clean();
    camera.on_mouse_movement(10.0f, 0.0f);
    REQUIRE(camera.view_dirty());
}

TEST_CASE("camera mouse movement changes view matrix", "[cam]") {
    Camera camera;
    auto const initial = camera.view_matrix();
    camera.on_mouse_movement(90.0f, 0.0f);
    REQUIRE(camera.view_matrix() != initial);
}

TEST_CASE("camera pitch clamped at +89 degrees", "[cam]") {
    using Catch::Approx;
    // sensitivity=0.25; both offsets exceed 89/0.25=356 → both clamp to
    // pitch=89
    Camera a, b;
    a.on_mouse_movement(0.0f, 400.0f);
    b.on_mouse_movement(0.0f, 1000.0f);
    auto const va = a.view_matrix();
    auto const vb = b.view_matrix();
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            REQUIRE(va[c][r] == Approx(vb[c][r]).margin(1e-5f));
}

TEST_CASE("camera pitch clamped at -89 degrees", "[cam]") {
    using Catch::Approx;
    Camera a, b;
    a.on_mouse_movement(0.0f, -400.0f);
    b.on_mouse_movement(0.0f, -1000.0f);
    auto const va = a.view_matrix();
    auto const vb = b.view_matrix();
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            REQUIRE(va[c][r] == Approx(vb[c][r]).margin(1e-5f));
}

TEST_CASE("camera scroll zooms in", "[cam]") {
    Camera camera;
    camera.set_fov_clean();
    camera.on_mouse_scroll(0.0, 1.0);
    REQUIRE(camera.fov() < Camera::FOV);
    REQUIRE(camera.fov_dirty());
}

TEST_CASE("camera scroll fov clamped at minimum", "[cam]") {
    Camera camera;
    camera.on_mouse_scroll(0.0, 10000.0);
    REQUIRE(camera.fov() == Catch::Approx(1.0f));
}

TEST_CASE("camera scroll fov clamped at maximum", "[cam]") {
    Camera camera;
    camera.on_mouse_scroll(0.0, -10000.0);
    REQUIRE(camera.fov() == Catch::Approx(Camera::FOV));
}

TEST_CASE("camera scroll sets fov dirty", "[cam]") {
    Camera camera;
    camera.set_fov_clean();
    camera.on_mouse_scroll(0.0, 1.0);
    REQUIRE(camera.fov_dirty());
}

TEST_CASE("camera reset restores fov", "[cam]") {
    Camera camera;
    camera.on_mouse_scroll(0.0, 10.0);
    REQUIRE(camera.fov() < Camera::FOV);
    camera.reset(glm::vec3(0.0f));
    REQUIRE(camera.fov() == Catch::Approx(Camera::FOV));
}

TEST_CASE("camera reset sets view dirty", "[cam]") {
    Camera camera;
    camera.set_view_clean();
    camera.reset(glm::vec3(5.0f, 0.0f, 0.0f));
    REQUIRE(camera.view_dirty());
}

TEST_CASE("camera reset changes view matrix", "[cam]") {
    Camera camera;
    camera.move(Camera::Direction::FORWARD, 10.0f);
    auto const moved = camera.view_matrix();
    camera.reset(glm::vec3(0.0f));
    REQUIRE(camera.view_matrix() != moved);
}
