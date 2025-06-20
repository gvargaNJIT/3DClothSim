#include "openGL.h"
#include <QOpenGLFunctions> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

ClothRenderer::ClothRenderer() 
    : vao(0), vbo(0), ebo(0), shaderProgram(0) {
}

ClothRenderer::~ClothRenderer() {
    gl->glDeleteVertexArrays(1, &vao);
    gl->glDeleteBuffers(1, &vbo);
    gl->glDeleteBuffers(1, &ebo);
    gl->glDeleteProgram(shaderProgram);
}

GLuint ClothRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader = gl->glCreateShader(type);
    gl->glShaderSource(shader, 1, &source, NULL);
    gl->glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        gl->glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}

void ClothRenderer::setupShaders() {
    const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal;
    layout(location = 2) in float curvature; // New: surface curvature for fold detection

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform bool isBackFace;
    uniform float thickness;

    out vec3 FragPos;
    out vec3 Normal;
    out vec3 ViewPos;
    out float Height;
    out float Curvature; // Pass curvature to fragment shader
    out vec3 WorldPos;   // World position for lighting calculations

    void main() {
        vec3 displacedPos = position;
        if (isBackFace) {
            displacedPos -= normal * thickness;
        }

        WorldPos = vec3(model * vec4(displacedPos, 1.0));
        FragPos = WorldPos;
        Normal = mat3(transpose(inverse(model))) * normal;
        
        // Extract camera position from view matrix
        ViewPos = vec3(inverse(view)[3]);
        
        Height = displacedPos.y;
        Curvature = curvature;
        
        gl_Position = projection * view * model * vec4(displacedPos, 1.0);
    }
)";

    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    in vec3 ViewPos;
    in float Height;
    in float Curvature;
    in vec3 WorldPos;

    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 clothColor;
    uniform int shadingMode;
    uniform float time; // For animated effects

    out vec4 FragColor;

    void main() {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 viewDir = normalize(ViewPos - FragPos);
        
        vec3 finalColor;
        
        if (shadingMode == 0) {
            // Enhanced fold-aware basic lighting
            float diff = max(dot(norm, lightDir), 0.0);
            
            // Emphasize folds with curvature-based darkening
            float foldFactor = 1.0 - clamp(abs(Curvature) * 5.0, 0.0, 0.7);
            
            vec3 diffuse = diff * lightColor * foldFactor;
            vec3 ambient = vec3(0.15) * foldFactor;
            
            finalColor = (diffuse + ambient) * clothColor;
            
        } else if (shadingMode == 1) {
            // Fold-enhanced Phong lighting
            vec3 ambient = 0.15 * lightColor;
            
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            // Enhanced specular with fold consideration
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
            
            // Reduce specular in deep folds (makes them appear darker/more recessed)
            float foldSpecularMask = 1.0 - clamp(abs(Curvature) * 3.0, 0.0, 0.8);
            vec3 specular = 0.6 * spec * lightColor * foldSpecularMask;
            
            // Darken deep folds
            float foldShadow = 1.0 - clamp(abs(Curvature) * 4.0, 0.0, 0.6);
            
            finalColor = (ambient + diffuse * foldShadow + specular) * clothColor;
            
        } else if (shadingMode == 2) {
            // Curvature-based fold visualization
            float normalizedCurvature = clamp(abs(Curvature) * 10.0, 0.0, 1.0);
            
            // Create color gradient: flat areas = cloth color, folded areas = darker
            vec3 foldColor = vec3(0.3, 0.2, 0.4); // Dark purple for deep folds
            vec3 flatColor = clothColor;
            
            vec3 curvatureColor = mix(flatColor, foldColor, normalizedCurvature);
            
            // Apply basic lighting
            float diff = max(dot(norm, lightDir), 0.2); // Higher ambient
            finalColor = diff * curvatureColor;
            
        } else if (shadingMode == 3) {
            // Dramatic fold highlighting with rim lighting
            float fresnel = 1.0 - max(dot(norm, viewDir), 0.0);
            fresnel = pow(fresnel, 1.5);
            
            // Basic lighting
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            // Curvature-enhanced rim lighting
            float curvatureRim = clamp(abs(Curvature) * 8.0, 0.0, 1.0);
            vec3 rimLight = (fresnel + curvatureRim * 0.5) * vec3(0.9, 0.7, 1.0);
            
            // Shadow folds heavily
            float foldShadow = 1.0 - clamp(abs(Curvature) * 5.0, 0.0, 0.8);
            
            finalColor = (diffuse * clothColor * foldShadow + rimLight * 0.8);
            
        } else if (shadingMode == 4) {
            // Debug mode: pure curvature visualization
            float normalizedCurvature = abs(Curvature) * 15.0;
            
            if (normalizedCurvature < 0.1) {
                finalColor = vec3(0.2, 0.8, 0.2); // Green for flat areas
            } else if (normalizedCurvature < 0.5) {
                finalColor = vec3(0.8, 0.8, 0.2); // Yellow for slight curves
            } else if (normalizedCurvature < 1.0) {
                finalColor = vec3(0.8, 0.4, 0.2); // Orange for moderate folds
            } else {
                finalColor = vec3(0.8, 0.2, 0.2); // Red for deep folds
            }
        }
        
        FragColor = vec4(finalColor, 1.0);
    }
)";
    
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = gl->glCreateProgram();
    gl->glAttachShader(shaderProgram, vertexShader);
    gl->glAttachShader(shaderProgram, fragmentShader);
    gl->glLinkProgram(shaderProgram);

    GLint success;
    GLchar infoLog[512];
    gl->glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        gl->glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    gl->glDeleteShader(vertexShader);
    gl->glDeleteShader(fragmentShader);
    
    currentShadingMode = 1;
}

void ClothRenderer::initialize(QOpenGLFunctions_3_3_Core* funcs) {
    gl = funcs;
    setupShaders();
    gl->glGenVertexArrays(1, &vao);
    gl->glGenBuffers(1, &vbo);
    gl->glGenBuffers(1, &ebo);
}

void ClothRenderer::setShadingMode(int mode) {
    currentShadingMode = mode;
}

float ClothRenderer::calculateCurvature(const std::vector<Particle>& particles, int index, int width, int height) {
    int x = index % width;
    int y = index / width;
    
    if (x < 2 || x >= width - 2 || y < 2 || y >= height - 2) {
        return 0.0f;
    }
    
    glm::vec3 center = particles[index].position;
    
    glm::vec3 left2 = particles[y * width + (x - 2)].position;
    glm::vec3 left1 = particles[y * width + (x - 1)].position;
    glm::vec3 right1 = particles[y * width + (x + 1)].position;
    glm::vec3 right2 = particles[y * width + (x + 2)].position;
    
    glm::vec3 up2 = particles[(y - 2) * width + x].position;
    glm::vec3 up1 = particles[(y - 1) * width + x].position;
    glm::vec3 down1 = particles[(y + 1) * width + x].position;
    glm::vec3 down2 = particles[(y + 2) * width + x].position;
    
    float curvatureX = glm::length((left2 - 2.0f * left1 + center) + (center - 2.0f * right1 + right2));
    float curvatureY = glm::length((up2 - 2.0f * up1 + center) + (center - 2.0f * down1 + down2));
    
    return (curvatureX + curvatureY) * 0.5f;
}

void ClothRenderer::calculateNormalsAndCurvature(std::vector<float>& vertices, const std::vector<Particle>& particles, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            int normalCount = 0;
            
            if (x < width - 1 && y < height - 1) {
                glm::vec3 p0 = particles[index].position;
                glm::vec3 p1 = particles[index + 1].position;
                glm::vec3 p2 = particles[index + width].position;
                
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p2 - p0;
                glm::vec3 triNormal = glm::normalize(glm::cross(v1, v2));
                
                normal += triNormal;
                normalCount++;
            }
            
            if (x > 0 && y < height - 1) {
                glm::vec3 p0 = particles[index].position;
                glm::vec3 p1 = particles[index + width].position;
                glm::vec3 p2 = particles[index - 1].position;
                
                glm::vec3 v1 = p1 - p0;
                glm::vec3 v2 = p2 - p0;
                glm::vec3 triNormal = glm::normalize(glm::cross(v1, v2));
                
                normal += triNormal;
                normalCount++;
            }
            
            if (normalCount > 0) {
                normal = glm::normalize(normal / float(normalCount));
            }
            
            float curvature = calculateCurvature(particles, index, width, height);
            
            int vertexIndex = index * 7;
            vertices[vertexIndex + 3] = normal.x;
            vertices[vertexIndex + 4] = normal.y;
            vertices[vertexIndex + 5] = normal.z;
            vertices[vertexIndex + 6] = curvature;
        }
    }
}

void ClothRenderer::updateBuffers(const std::vector<Particle>& particles, int width, int height) {
    vertices.clear();
    vertices.resize(particles.size() * 7, 0.0f);
    for (size_t i = 0; i < particles.size(); i++) {
        vertices[i * 7 + 0] = particles[i].position.x;
        vertices[i * 7 + 1] = particles[i].position.y;
        vertices[i * 7 + 2] = particles[i].position.z;
    }

    calculateNormalsAndCurvature(vertices, particles, width, height);

    indices.clear();
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            int i0 = y * width + x;
            int i1 = i0 + 1;
            int i2 = i0 + width;
            int i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
            indices.push_back(i0);
            indices.push_back(i3);
            indices.push_back(i1);
        }
    }

    gl->glBindVertexArray(vao);

    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    gl->glEnableVertexAttribArray(0);

    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    gl->glEnableVertexAttribArray(1);

    gl->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    gl->glEnableVertexAttribArray(2);

    gl->glBindVertexArray(0);
}

void ClothRenderer::render(const Cloth& cloth, const std::vector<Particle>& particles, 
                          const glm::mat4& projection, const glm::mat4& view) {
    updateBuffers(particles, cloth.width, cloth.height);

    gl->glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);

    gl->glUniformMatrix4fv(gl->glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    gl->glUniformMatrix4fv(gl->glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    gl->glUniformMatrix4fv(gl->glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 lightPos(1.5f, 2.5f, 1.5f);
    glm::vec3 lightColor(1.2f, 1.2f, 1.0f);
    glm::vec3 clothColor(0.8f, 0.6f, 0.9f);
    
    gl->glUniform3fv(gl->glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    gl->glUniform3fv(gl->glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
    gl->glUniform3fv(gl->glGetUniformLocation(shaderProgram, "clothColor"), 1, glm::value_ptr(clothColor));
    gl->glUniform1i(gl->glGetUniformLocation(shaderProgram, "shadingMode"), currentShadingMode);

    static float time = 0.0f;
    time += 0.016f;
    gl->glUniform1f(gl->glGetUniformLocation(shaderProgram, "time"), time);

    gl->glBindVertexArray(vao);
    gl->glEnable(GL_POLYGON_OFFSET_FILL);
    gl->glDisable(GL_CULL_FACE);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glPolygonOffset(1.0f, 1.0f);

    gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    gl->glDisable(GL_POLYGON_OFFSET_FILL);
    gl->glBindVertexArray(0);
    gl->glUseProgram(0);
}