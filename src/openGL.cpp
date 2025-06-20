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

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        uniform bool isBackFace;
        uniform float thickness;

        out vec3 FragPos;
        out vec3 Normal;

        void main() {
            vec3 displacedPos = position;
            if (isBackFace) {
                displacedPos -= normal * thickness;
            }

            FragPos = vec3(model * vec4(displacedPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * normal;
            gl_Position = projection * view * model * vec4(displacedPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;

        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 clothColor;

        out vec4 FragColor;

        void main() {
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);

            vec3 diffuse = diff * lightColor;
            vec3 result = (diffuse + vec3(0.1)) * clothColor;
            FragColor = vec4(result, 1.0);
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
}

void ClothRenderer::initialize(QOpenGLFunctions_3_3_Core* funcs) {
    gl = funcs;
    setupShaders();
    gl->glGenVertexArrays(1, &vao);
    gl->glGenBuffers(1, &vbo);
    gl->glGenBuffers(1, &ebo);
}

void ClothRenderer::calculateNormals(std::vector<float>& vertices, const std::vector<Particle>& particles, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            glm::vec3 normal(0.0f, 0.0f, 0.0f);

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
            
            if (glm::length(normal) > 0.0f) {
                normal = glm::normalize(normal);
            } else {
                normal = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            
            int vertexIndex = index * 6;
            vertices[vertexIndex + 3] = normal.x;
            vertices[vertexIndex + 4] = normal.y;
            vertices[vertexIndex + 5] = normal.z;
        }
    }
}

void ClothRenderer::updateBuffers(const std::vector<Particle>& particles, int width, int height) {
    vertices.clear();
    vertices.resize(particles.size() * 6, 0.0f);

    for (size_t i = 0; i < particles.size(); i++) {
        vertices[i * 6 + 0] = particles[i].position.x;
        vertices[i * 6 + 1] = particles[i].position.y;
        vertices[i * 6 + 2] = particles[i].position.z;
    }

    calculateNormals(vertices, particles, width, height);

    float thickness = 0.02f;
    std::vector<float> verticesBack = vertices;
    for (size_t i = 0; i < particles.size(); ++i) {
        glm::vec3 normal(
            verticesBack[i * 6 + 3],
            verticesBack[i * 6 + 4],
            verticesBack[i * 6 + 5]
        );
        glm::vec3 offsetPos(
            verticesBack[i * 6 + 0],
            verticesBack[i * 6 + 1],
            verticesBack[i * 6 + 2]
        );
        offsetPos -= thickness * normal;

        verticesBack[i * 6 + 0] = offsetPos.x;
        verticesBack[i * 6 + 1] = offsetPos.y;
        verticesBack[i * 6 + 2] = offsetPos.z;

        verticesBack[i * 6 + 3] = -normal.x;
        verticesBack[i * 6 + 4] = -normal.y;
        verticesBack[i * 6 + 5] = -normal.z;
    }

    vertices.insert(vertices.end(), verticesBack.begin(), verticesBack.end());

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

            // Back face (CW order, offset by particles.size())
            int offset = particles.size();
            indices.push_back(i1 + offset);
            indices.push_back(i3 + offset);
            indices.push_back(i0 + offset);
            indices.push_back(i3 + offset);
            indices.push_back(i2 + offset);
            indices.push_back(i0 + offset);
        }
    }

    gl->glBindVertexArray(vao);

    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    gl->glEnableVertexAttribArray(0);

    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    gl->glEnableVertexAttribArray(1);

    gl->glBindVertexArray(0);
}

void ClothRenderer::render(const Cloth& cloth, const std::vector<Particle>& particles, const glm::mat4& projection, const glm::mat4& view) {
    updateBuffers(particles, cloth.width, cloth.height);

    gl->glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);

    gl->glUniformMatrix4fv(gl->glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    gl->glUniformMatrix4fv(gl->glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glm::mat4 zBias = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.001f));
    glm::mat4 biasedProjection = projection * zBias;

    gl->glUniformMatrix4fv(gl->glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(biasedProjection));

    glm::vec3 lightPos(2.0f, 3.0f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 clothColor(0.2f, 0.5f, 0.8f);
    
    gl->glUniform3fv(gl->glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    gl->glUniform3fv(gl->glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
    gl->glUniform3fv(gl->glGetUniformLocation(shaderProgram, "clothColor"), 1, glm::value_ptr(clothColor));

    gl->glBindVertexArray(vao);

    gl->glEnable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    gl->glPolygonOffset(100.0f, 100.0f);

    gl->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    gl->glDisable(GL_POLYGON_OFFSET_FILL);
    gl->glBindVertexArray(0);
    gl->glUseProgram(0);
}