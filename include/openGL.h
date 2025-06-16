#ifndef OPENGL_H
#define OPENGL_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "clothsim.h"
#include <vector>

class ClothRenderer {
private:
    GLuint vao, vbo, ebo;
    GLuint shaderProgram;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 Normal;
        out vec3 FragPos;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 Normal;
        in vec3 FragPos;
        
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 clothColor;
        
        out vec4 FragColor;
        
        void main() {
            // Ambient
            float ambientStrength = 0.2;
            vec3 ambient = ambientStrength * lightColor;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            vec3 result = (ambient + diffuse) * clothColor;
            FragColor = vec4(result, 1.0);
        }
    )";
    
    GLuint compileShader(GLenum type, const char* source);
    void setupShaders();
    void updateBuffers(const std::vector<Particle>& particles, int width, int height);
    void calculateNormals(std::vector<float>& vertices, const std::vector<Particle>& particles, int width, int height);
    
public:
    ClothRenderer();
    ~ClothRenderer();
    
    void initialize();
    void render(const Cloth& cloth, const std::vector<Particle>& particles, const glm::mat4& projection, const glm::mat4& view);
};

#endif
