#pragma once

#include "CheckError.h"

#include <SOIL.h>

#include <cassert>
#include <iostream>

inline
GLint loadTexture(GLchar const * path, bool alpha = false)
{
    std::cout << "loading texture: " << path << " (alpha: " << std::boolalpha << alpha << ")\n";
    assert(path);
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width, height;
    int soil_force_channels = SOIL_LOAD_RGB;
    GLenum format = GL_RGB;
    GLint wrap = GL_REPEAT;

    if (alpha) {
        soil_force_channels = SOIL_LOAD_RGBA;
        format = GL_RGBA;
        wrap = GL_CLAMP_TO_EDGE;
    }

    unsigned char * image = SOIL_load_image(path, &width, &height, 0, soil_force_channels);
    assert(image);

    // bind the current 2D texture, load in the data to opengl, and generate mipmaps
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    CheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);
    std::cout << "loaded texture: " << path << " (id: " << textureID << ' ' << width << 'x' << height << ")\n";
    return textureID;
}

inline
GLint loadTransparentTexture(GLchar const * path)
{
    return loadTexture(path, true);
}

inline
GLint loadGammaTexture(GLchar const * path)
{
    std::cout << "loading gamma texture: " << path << '\n';
    assert(path);
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width, height;

    unsigned char * image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGB);
    assert(image);

    // bind the current 2D texture, load in the data to opengl, and generate mipmaps
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    CheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);
    std::cout << "loaded texture: " << path << " (" << textureID << ")\n";
    return textureID;
}
