#include "ModernGLRenderer.h"
#include "../shader_utils.h"
#include "../globals.h"
#include <vector>
#include <glm/glm.hpp>
#include <iostream>


ModernGLRenderer::ModernGLRenderer()
    : m_shader_program(0), m_instance_vbo(0), m_color_vbo(0), m_vao(0)
{
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW init failed: " << glewGetErrorString(err) << std::endl;
        // TODO: throw an exception here?
        return;
    }

    float vertices[] = {
      -0.5f, -0.5f,
       0.5f, -0.5f,
       0.5f,  0.5f,
      -0.5f,  0.5f
    };


    GLuint VBO;
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &VBO);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Vertex position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Per-instance offset buffer
    glGenBuffers(1, &m_instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // Per-instance color buffer
    glGenBuffers(1, &m_color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Compile the shaders
    m_shader_program = create_shader_program(
      load_shader_source("shaders/vertex_shader.vert").c_str(),
      load_shader_source("shaders/fragment_shader.frag").c_str()
    );
}

ModernGLRenderer::~ModernGLRenderer()
{
    if (m_shader_program != 0) {
        glDeleteProgram(m_shader_program);
    }
    if (m_instance_vbo != 0) {
        glDeleteBuffers(1, &m_instance_vbo);
    }
    if (m_color_vbo != 0) {
        glDeleteBuffers(1, &m_color_vbo);
    }
    if (m_vao != 0) {
     glDeleteVertexArrays(1, &m_vao);
    }

}

void ModernGLRenderer::draw(const int* latticeState, int latticeDimension)
{
    std::vector<glm::vec2> positions;
    std::vector<glm::vec3> colors;

    const float L_float = static_cast<float>(latticeDimension);

    for (int x = 0; x < latticeDimension; ++x) {
        for (int y = 0; y < latticeDimension; ++y) {
            // Map lattice coordinates (0 to L) to screen coordinates (-1.0 to +1.0)
            // We add 0.5f to center the quad on the lattice point.
            float xPos = ((static_cast<float>(x) + 0.5f) / L_float) * 2.0f - 1.0f;
            float yPos = 1.0f - ((static_cast<float>(y) + 0.5f) / L_float) * 2.0f;

            positions.emplace_back(xPos, yPos);

            if (latticeState[x + y * latticeDimension] > 0)
                colors.emplace_back(0.0f, 1.0f, 0.39f); // green
            else
                colors.emplace_back(0.78f, 0.39f, 0.0f); // orange
        }
    }

    glUseProgram(m_shader_program);

    // Send size
    GLint sizeLoc = glGetUniformLocation(m_shader_program, "size");
    glUniform1f(sizeLoc, 2.0f / static_cast<float>(latticeDimension));

    // Update instance data
    glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(glm::vec2)), positions.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(colors.size() * sizeof(glm::vec3)), colors.data(), GL_DYNAMIC_DRAW);

    // Draw all instances
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(positions.size()));
}