#include "md2view/camera.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("camera default ctor", "[cam]") {
    Camera camera;

    REQUIRE(camera.fov() == Camera::FOV);
    REQUIRE(camera.view_dirty());
    REQUIRE(camera.fov_dirty());
}
