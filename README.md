# md2view
This is a personal project used for learning. Issue reports or pull requests will be ignored.

The purpose of this project is to be able to load, render, texture, and animate
MD2 models that are stored in Quake2 PAK files or which have been extracted into
a directory structure. The rendering is currently done in OpenGL.

Here is a screenshot of md2view in action:

![Example](docs/screenshot.png)

Using the GUI you can manipulate the model transform and observe the updated matrix.
You can control the camera by pressing F1. The camera controls are:

    * W - Forward
    * S - Backward
    * A - Left
    * D- Right
    * Mouse - Pitch / Yaw
    * Scroll wheel - Field of View

The FOV can also be adjusted with the GUI. The curret view and projection matrices
will be displayed in the GUI when you manipulate the camera.

I have tested this on the following systems:

    * Mac OS X Mojave (10.14.6) - AMD Radeon R9 M370X
    * Ubuntu 18.04.3 LTS - NVIDIA GeForce GTX 1050 Ti

I use a locally managed toolchain for build dependencies. These dependencies are:

    * Boost (filesystem, system, program_options)
    * GLFW
    * GLEW
    * GLM
    * SOIL
    * Dear ImGui (bundled)
