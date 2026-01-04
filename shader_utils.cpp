#ifdef MODERN_GL

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "shader_utils.h"


GLuint create_shader_program(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = create_shader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = create_shader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertexShader);
    glAttachShader(shader_program, fragmentShader);
    glLinkProgram(shader_program);

    // Error check
    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        std::cerr << "Program linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shader_program;
}


GLuint create_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Error check
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }

    return shader;
}


std::string load_shader_source(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        std::cerr << "Unable to open shader file: " << filePath << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

#endif
