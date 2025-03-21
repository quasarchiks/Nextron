#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Renderer {
public:
    void init();
    void render();
    void cleanup();
private:
    unsigned int VAO, VBO, shaderProgram;
    void setupShaders();
    void setupMesh();
};
