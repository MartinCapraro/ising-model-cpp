#pragma once

#include <GL/glew.h> // We need GLuint for the member variables
#include "Renderer.h"


class ModernGLRenderer : public Renderer {
public:
    ModernGLRenderer();
    ~ModernGLRenderer();

    void draw(const int* latticeState, int latticeDimension) override;

private:
    // Member variables to hold the OpenGL object handles.
    GLuint m_shader_program;
    GLuint m_instance_vbo;
    GLuint m_color_vbo;
    GLuint m_vao;
};