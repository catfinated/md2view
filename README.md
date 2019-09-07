# md2view
This is a personal project used for learning. Issue reports or pull request will be ignored.

The purpose of this project is to be able to load, render, texture, and animate
MD2 models that are stored in Quake2 PAK Files or which have been extracted into
directory structure. The rendering is currently done in OpenGL v4.

Using the GUI you can manipulate the camera and the model and observe the updated
model-view-projection matrices. Here is an example:

![Example](docs/screenshot.png)

I use a locally managed toolchain to manage dependnencies. The dependencies are:

* Boost (Filesystem, system, program_options)
* GLFW
* GLEW
* GLM
* SOIL
* Dear Imgui (bundled)
