#include "openGL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

ClothRenderer::ClothRenderer() 
    : vao(0), vbo(0), ebo(0), shaderProgram(0) {
}

ClothRenderer::~ClothRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
}

GLuint ClothRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    // Check for shader compile errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}

void ClothRenderer::setupShaders() {
    // Compile vertex and fragment shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    // Create shader program and link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void ClothRenderer::initialize() {
    // Set up shaders
    setupShaders();
    
    // Create vertex array object, vertex buffer object, and element buffer object
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
}

void ClothRenderer::calculateNormals(std::vector<float>& vertices, const std::vector<Particle>& particles, int width, int height) {
    // For each vertex, calculate normal by averaging the normals of adjacent triangles
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            glm::vec3 normal(0.0f, 0.0f, 0.0f);
            
            // Calculate normals from surrounding triangles
            if (x < width - 1 && y < height - 1) {
                glm::vec3 p0 = particles[index].position;
                glm::vec3 p1 = particles[index + 1].position;
                glm::vec3 p2 = particles[index + width].position;
                
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p2 - p0;
                glm::vec3 triNormal = glm::cross(v1, v2);
                
                normal += glm::normalize(triNormal);
            }
            
            if (x > 0 && y < height - 1) {
                glm::vec3 p0 = particles[index].position;
                glm::vec3 p1 = particles[index + width].position;
                glm::vec3 p2 = particles[index - 1].position;
                
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p2 - p0;
                glm::vec3 triNormal = glm::cross(v1, v2);
                
                normal += glm::normalize(triNormal);
            }
            
            if (x > 0 && y > 0) {
                glm::vec3 p0 = particles[index].position;
                glm::vec3 p1 = particles[index - 1].position;
                glm::vec3 p2 = particles[index - width].position;
                
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p2 - p0;
                glm::vec3 triNormal = glm::cross(v1, v2);
                
                normal += glm::normalize(triNormal);
            }
            
            if (x < width - 1 && y > 0) {
                glm::vec3 p0 = particles[index].position;
                glm::vec3 p1 = particles[index - width].position;
                glm::vec3 p2 = particles[index + 1].position;
                
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p2 - p0;
                glm::vec3 triNormal = glm::cross(v1, v2);
                
                normal += glm::normalize(triNormal);
            }
            
            // Normalize the final normal
            if (glm::length(normal) > 0.0f) {
                normal = glm::normalize(normal);
            } else {
                normal = glm::vec3(0.0f, 0.0f, 1.0f); // Default normal
            }
            
            // Update the normal in the vertex data
            int vertexIndex = index * 6; // Each vertex has 6 components (position + normal)
            vertices[vertexIndex + 3] = normal.x;
            vertices[vertexIndex + 4] = normal.y;
            vertices[vertexIndex + 5] = normal.z;
        }
    }
}

void ClothRenderer::updateBuffers(const std::vector<Particle>& particles, int width, int height) {
    // Create vertices array
    vertices.clear();
    vertices.resize(particles.size() * 6, 0.0f); // 3 for position, 3 for normal
    
    // Set positions
    for (size_t i = 0; i < particles.size(); i++) {
        vertices[i * 6 + 0] = particles[i].position.x;
        vertices[i * 6 + 1] = particles[i].position.y;
        vertices[i * 6 + 2] = particles[i].position.z;
        // Normals are set to default and will be calculated later
        vertices[i * 6 + 3] = 0.0f;
        vertices[i * 6 + 4] = 0.0f;
        vertices[i * 6 + 5] = 1.0f;
    }
    
    // Calculate normals
    calculateNormals(vertices, particles, width, height);
    
    // Create indices for triangles
    indices.clear();
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            int topLeft = y * width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * width + x;
            int bottomRight = bottomLeft + 1;
            
            // First triangle (top-left, bottom-left, bottom-right)
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
            
            // Second triangle (top-left, bottom-right, top-right)
            indices.push_back(topLeft);
            indices.push_back(bottomRight);
            indices.push_back(topRight);
        }
    }
    
    // Update buffers
    glBindVertexArray(vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void ClothRenderer::render(const Cloth& cloth, const std::vector<Particle>& particles, const glm::mat4& projection, const glm::mat4& view) {
    // Update buffers with current particle positions
    updateBuffers(particles, cloth.width, cloth.height);
    
    // Use shader program
    glUseProgram(shaderProgram);
    
    // Set uniform values
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Set light properties
    glm::vec3 lightPos(2.0f, 3.0f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 clothColor(0.2f, 0.5f, 0.8f);
    
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderProgram, "clothColor"), 1, glm::value_ptr(clothColor));
    
    // Draw mesh
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Clean up
    glUseProgram(0);
}