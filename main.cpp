// main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    
    out vec3 FragPos;
    out vec3 Normal;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    
    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    
    void main() {
        // Ambient
        float ambientStrength = 0.3;
        vec3 ambient = ambientStrength * lightColor;
        
        // Diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

// Cloth simulation parameters
const int CLOTH_SIZE = 20;
const float CLOTH_WIDTH = 2.0f;
const float SPRING_STIFFNESS = 500.0f;
const float DAMPING = 5.0f;
const float MASS = 0.1f;
const float GRAVITY = 9.8f;
const int ITERATIONS = 15;
const float TIME_STEP = 0.01f;

// Mouse interaction
glm::vec2 lastMousePos(0.0f);
bool mouseDown = false;
int selectedParticle = -1;

// Window dimensions
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov = 45.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Structure to represent a particle in the cloth
struct Particle {
    glm::vec3 position;
    glm::vec3 oldPosition;
    glm::vec3 velocity;
    glm::vec3 force;
    glm::vec3 normal;
    bool fixed;
    
    Particle(const glm::vec3& pos, bool isFixed = false) : 
        position(pos),
        oldPosition(pos),
        velocity(0.0f),
        force(0.0f),
        normal(0.0f, 1.0f, 0.0f),
        fixed(isFixed) {}
};

// Structure to represent a spring connecting two particles
struct Spring {
    int p1, p2;
    float restLength;
    
    Spring(int particle1, int particle2, float length) : 
        p1(particle1), 
        p2(particle2), 
        restLength(length) {}
};

// Global variables for cloth simulation
std::vector<Particle> particles;
std::vector<Spring> springs;
std::vector<unsigned int> indices;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);
void updateCloth();
void calculateNormals();
unsigned int compileShader(unsigned int type, const char* source);
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
int getClosestParticle(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
glm::vec3 getRayFromMouse(GLFWwindow* window, double mouseX, double mouseY);

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Cloth Simulation", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    // Tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    
    // Create shader program
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    // Initialize cloth particles and springs
    particles.clear();
    springs.clear();
    indices.clear();
    
    // Create cloth particles in a grid
    float spacing = CLOTH_WIDTH / (CLOTH_SIZE - 1);
    for (int i = 0; i < CLOTH_SIZE; i++) {
        for (int j = 0; j < CLOTH_SIZE; j++) {
            float x = (j - CLOTH_SIZE / 2) * spacing;
            float y = CLOTH_WIDTH / 2;
            float z = (i - CLOTH_SIZE / 2) * spacing;
            
            // Fix the top corners
            bool isFixed = (i == 0 && (j == 0 || j == CLOTH_SIZE-1));
            
            particles.push_back(Particle(glm::vec3(x, y, z), isFixed));
        }
    }
    
    // Create structural springs (horizontal and vertical)
    for (int i = 0; i < CLOTH_SIZE; i++) {
        for (int j = 0; j < CLOTH_SIZE; j++) {
            int idx = i * CLOTH_SIZE + j;
            
            // Horizontal springs
            if (j < CLOTH_SIZE - 1) {
                int idx2 = i * CLOTH_SIZE + (j + 1);
                float restLength = glm::length(particles[idx].position - particles[idx2].position);
                springs.push_back(Spring(idx, idx2, restLength));
            }
            
            // Vertical springs
            if (i < CLOTH_SIZE - 1) {
                int idx2 = (i + 1) * CLOTH_SIZE + j;
                float restLength = glm::length(particles[idx].position - particles[idx2].position);
                springs.push_back(Spring(idx, idx2, restLength));
            }
            
            // Diagonal springs
            if (i < CLOTH_SIZE - 1 && j < CLOTH_SIZE - 1) {
                int idx2 = (i + 1) * CLOTH_SIZE + (j + 1);
                float restLength = glm::length(particles[idx].position - particles[idx2].position);
                springs.push_back(Spring(idx, idx2, restLength));
            }
            
            if (i < CLOTH_SIZE - 1 && j > 0) {
                int idx2 = (i + 1) * CLOTH_SIZE + (j - 1);
                float restLength = glm::length(particles[idx].position - particles[idx2].position);
                springs.push_back(Spring(idx, idx2, restLength));
            }
        }
    }
    
    // Create indices for rendering
    for (int i = 0; i < CLOTH_SIZE - 1; i++) {
        for (int j = 0; j < CLOTH_SIZE - 1; j++) {
            int tl = i * CLOTH_SIZE + j;
            int tr = i * CLOTH_SIZE + (j + 1);
            int bl = (i + 1) * CLOTH_SIZE + j;
            int br = (i + 1) * CLOTH_SIZE + (j + 1);
            
            // First triangle
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            
            // Second triangle
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }
    
    // Create buffers
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Process input
        processInput(window);
        
        // Update cloth physics
        for (int i = 0; i < ITERATIONS; i++) {
            updateCloth();
        }
        
        // Calculate normals for lighting
        calculateNormals();
        
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Activate shader
        glUseProgram(shaderProgram);
        
        // Create transformations
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        
        // Pass transformation matrices to shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        // Set lighting uniforms
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 1.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.5f, 0.8f);
        
        // Prepare buffers with updated particle positions
        std::vector<float> vertexData;
        for (const auto& p : particles) {
            // Position
            vertexData.push_back(p.position.x);
            vertexData.push_back(p.position.y);
            vertexData.push_back(p.position.z);
            
            // Normal
            vertexData.push_back(p.normal.x);
            vertexData.push_back(p.normal.y);
            vertexData.push_back(p.normal.z);
        }
        
        // Bind buffers and set vertex attributes
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Draw the cloth
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        
        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // De-allocate resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    // Terminate GLFW
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mouseDown && selectedParticle != -1) {
        // Move the selected particle based on mouse movement
        glm::vec3 rayDir = getRayFromMouse(window, xpos, ypos);
        glm::vec3 rayOrigin = cameraPos;
                    
        // Calculate a plane perpendicular to the view direction
        glm::vec3 planePoint = particles[selectedParticle].position;
        glm::vec3 planeNormal = glm::normalize(cameraPos - planePoint);
                    
        // Ray-plane intersection to find the new particle position
        float t = glm::dot(planePoint - rayOrigin, planeNormal) / glm::dot(rayDir, planeNormal);
        glm::vec3 newPos = rayOrigin + t * rayDir;
                    
        // Update the particle position
        if (!particles[selectedParticle].fixed) {
            particles[selectedParticle].position = newPos;
            particles[selectedParticle].oldPosition = newPos;
            particles[selectedParticle].velocity = glm::vec3(0.0f);
            }
        } else {
            // Regular camera rotation logic
                if (firstMouse) {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                    }
                    
                    float xoffset = xpos - lastX;
                    float yoffset = lastY - ypos;
                    lastX = xpos;
                    lastY = ypos;
                    
                    const float sensitivity = 0.1f;
                    xoffset *= sensitivity;
                    yoffset *= sensitivity;
                    
                    yaw += xoffset;
                    pitch += yoffset;
                    
                    // Clamp pitch to prevent flipping
                    if (pitch > 89.0f) pitch = 89.0f;
                    if (pitch < -89.0f) pitch = -89.0f;
                    
                    // Update camera front vector
                    glm::vec3 front;
                    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                    front.y = sin(glm::radians(pitch));
                    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                    cameraFront = glm::normalize(front);
                }
            }
            
            void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
                fov -= (float)yoffset;
                if (fov < 1.0f) fov = 1.0f;
                if (fov > 90.0f) fov = 90.0f;
            }
            
            void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    if (action == GLFW_PRESS) {
                        mouseDown = true;
                        
                        // Get current mouse position
                        double xpos, ypos;
                        glfwGetCursorPos(window, &xpos, &ypos);
                        
                        // Get ray from mouse position
                        glm::vec3 rayDir = getRayFromMouse(window, xpos, ypos);
                        glm::vec3 rayOrigin = cameraPos;
                        
                        // Find the closest particle to the ray
                        selectedParticle = getClosestParticle(rayOrigin, rayDir);
                    } else if (action == GLFW_RELEASE) {
                        mouseDown = false;
                        selectedParticle = -1;
                    }
                }
            }
            
            void processInput(GLFWwindow* window) {
                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                    glfwSetWindowShouldClose(window, true);
                
                // Camera movement
                float cameraSpeed = 2.5f * deltaTime;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    cameraPos += cameraSpeed * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    cameraPos -= cameraSpeed * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                    cameraPos += cameraUp * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
                    cameraPos -= cameraUp * cameraSpeed;
                
                // Reset cloth
                if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                    float spacing = CLOTH_WIDTH / (CLOTH_SIZE - 1);
                    for (int i = 0; i < CLOTH_SIZE; i++) {
                        for (int j = 0; j < CLOTH_SIZE; j++) {
                            int idx = i * CLOTH_SIZE + j;
                            float x = (j - CLOTH_SIZE / 2) * spacing;
                            float y = CLOTH_WIDTH / 2;
                            float z = (i - CLOTH_SIZE / 2) * spacing;
                            
                            if (!particles[idx].fixed) {
                                particles[idx].position = glm::vec3(x, y, z);
                                particles[idx].oldPosition = glm::vec3(x, y, z);
                                particles[idx].velocity = glm::vec3(0.0f);
                            }
                        }
                    }
                }
            }
            
            void updateCloth() {
                // Apply forces
                for (auto& p : particles) {
                    if (!p.fixed) {
                        // Reset forces
                        p.force = glm::vec3(0.0f);
                        
                        // Apply gravity
                        p.force += glm::vec3(0.0f, -GRAVITY, 0.0f) * MASS;
                    }
                }
                
                // Apply spring forces
                for (const auto& s : springs) {
                    Particle& p1 = particles[s.p1];
                    Particle& p2 = particles[s.p2];
                    
                    glm::vec3 delta = p2.position - p1.position;
                    float dist = glm::length(delta);
                    glm::vec3 dir = delta / dist;
                    
                    // Spring force
                    float springForce = (dist - s.restLength) * SPRING_STIFFNESS;
                    
                    // Damping force
                    glm::vec3 relativeVelocity = p2.velocity - p1.velocity;
                    float dampingForce = glm::dot(relativeVelocity, dir) * DAMPING;
                    
                    glm::vec3 force = dir * (springForce + dampingForce);
                    
                    if (!p1.fixed) p1.force += force;
                    if (!p2.fixed) p2.force -= force;
                }
                
                // Integrate forces using Verlet integration
                for (auto& p : particles) {
                    if (!p.fixed) {
                        glm::vec3 temp = p.position;
                        glm::vec3 acceleration = p.force / MASS;
                        
                        // Verlet integration
                        p.position = p.position * 2.0f - p.oldPosition + acceleration * TIME_STEP * TIME_STEP;
                        p.oldPosition = temp;
                        
                        // Update velocity (for damping calculation)
                        p.velocity = (p.position - p.oldPosition) / TIME_STEP;
                    }
                }
            }
            
            void calculateNormals() {
                // Reset normals
                for (auto& p : particles) {
                    p.normal = glm::vec3(0.0f);
                }
                
                // Calculate face normals and add them to vertices
                for (size_t i = 0; i < indices.size(); i += 3) {
                    unsigned int idx1 = indices[i];
                    unsigned int idx2 = indices[i + 1];
                    unsigned int idx3 = indices[i + 2];
                    
                    glm::vec3 v1 = particles[idx1].position;
                    glm::vec3 v2 = particles[idx2].position;
                    glm::vec3 v3 = particles[idx3].position;
                    
                    glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
                    
                    particles[idx1].normal += normal;
                    particles[idx2].normal += normal;
                    particles[idx3].normal += normal;
                }
                
                // Normalize
                for (auto& p : particles) {
                    p.normal = glm::normalize(p.normal);
                }
            }
            
            unsigned int compileShader(unsigned int type, const char* source) {
                unsigned int shader = glCreateShader(type);
                glShaderSource(shader, 1, &source, NULL);
                glCompileShader(shader);
                
                // Check for shader compile errors
                int success;
                char infoLog[512];
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success) {
                    glGetShaderInfoLog(shader, 512, NULL, infoLog);
                    std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
                }
                
                return shader;
            }
            
            unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
                unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
                unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
                
                unsigned int program = glCreateProgram();
                glAttachShader(program, vertexShader);
                glAttachShader(program, fragmentShader);
                glLinkProgram(program);
                
                // Check for linking errors
                int success;
                char infoLog[512];
                glGetProgramiv(program, GL_LINK_STATUS, &success);
                if (!success) {
                    glGetProgramInfoLog(program, 512, NULL, infoLog);
                    std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
                }
                
                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);
                
                return program;
            }
            
            int getClosestParticle(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
                int closestIdx = -1;
                float closestDist = std::numeric_limits<float>::max();
                
                // Simple approach: find the particle whose projection on the ray is closest to the ray origin
                for (int i = 0; i < particles.size(); i++) {
                    glm::vec3 particleDir = particles[i].position - rayOrigin;
                    float projection = glm::dot(particleDir, rayDir);
                    
                    if (projection > 0) {  // Only consider particles in front of the camera
                        glm::vec3 projectedPoint = rayOrigin + rayDir * projection;
                        float dist = glm::distance(projectedPoint, particles[i].position);
                        
                        if (dist < closestDist && dist < 0.5f) {  // Only accept hits within a certain radius
                            closestDist = dist;
                            closestIdx = i;
                        }
                    }
                }
                
                return closestIdx;
            }
            
            glm::vec3 getRayFromMouse(GLFWwindow* window, double mouseX, double mouseY) {
                // Convert mouse position to normalized device coordinates
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                
                float x = (2.0f * mouseX) / width - 1.0f;
                float y = 1.0f - (2.0f * mouseY) / height;
                
                // Calculate ray direction in world space
                glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
                glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
                
                glm::mat4 invProj = glm::inverse(projection);
                glm::mat4 invView = glm::inverse(view);
                
                glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
                glm::vec4 rayEye = invProj * rayClip;
                rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                
                glm::vec4 rayWorld = invView * rayEye;
                glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld));
                
                return rayDir;
            }
