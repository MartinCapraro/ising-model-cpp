#include <string>
#include <GL/glew.h>

GLuint create_shader_program(const char* vertexSource, const char* fragmentSource);
GLuint create_shader(GLenum type, const char* source);
std::string load_shader_source(const std::string& filePath);
