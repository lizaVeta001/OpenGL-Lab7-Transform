#define GLEW_DLL
#define GLFW_DLL

#include <iostream>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Model.h"

// -------------------------------------------------------------
// Шейдеры (вершинный и фрагментный)
// -------------------------------------------------------------
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 FragPos;
out vec3 Normal;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Material material;
uniform Light light;
uniform vec3 viewPos;
in vec3 FragPos;
in vec3 Normal;
void main() {
    vec3 ambient = light.ambient * material.ambient;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";

// -------------------------------------------------------------
// Класс Shader
// -------------------------------------------------------------
class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexCode, const char* fragmentCode) {
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexCode, NULL);
        glCompileShader(vertex);
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentCode, NULL);
        glCompileShader(fragment);
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    void use() { glUseProgram(ID); }
    void setVec3(const std::string& name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }
    void setFloat(const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setMat4(const std::string& name, const glm::mat4& mat) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
};

// -------------------------------------------------------------
// Камера
// -------------------------------------------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f, lastX = 400, lastY = 300;
bool firstMouse = true;
float sensitivity = 0.1f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoff = (xpos - lastX) * sensitivity;
    float yoff = (lastY - ypos) * sensitivity;
    lastX = xpos; lastY = ypos;
    yaw += xoff;
    pitch += yoff;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraPos.z += yoffset * 0.5f;
}

// -------------------------------------------------------------
// Основная функция
// -------------------------------------------------------------
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Lab7 - Affine Transformations", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    Shader lightingShader(vertexShaderSource, fragmentShaderSource);

    // Загрузка сегментов (центры уже в шарнирах)
    Model base("base.obj");
    Model arm("arm.obj");
    Model hand("hand.obj");

    // Материал и свет
    lightingShader.use();
    lightingShader.setVec3("material.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("material.diffuse", glm::vec3(0.8f, 0.4f, 0.6f));
    lightingShader.setVec3("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("material.shininess", 32.0f);

    glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, 3.0f);
    lightingShader.setVec3("light.position", lightPos);
    lightingShader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("light.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    lightingShader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

    // Углы поворота
    float baseAngle = 0.0f;
    float armAngle = 0.0f;
    float handAngle = 0.0f;

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Камера WASD
        float speed = 3.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        // Управление сегментами
        float rotSpeed = 50.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) baseAngle += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) baseAngle -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) armAngle += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) armAngle -= rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) handAngle += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) handAngle -= rotSpeed;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        lightingShader.use();
        lightingShader.setMat4("view", view);
        lightingShader.setMat4("projection", projection);
        lightingShader.setVec3("viewPos", cameraPos);
        lightingShader.setVec3("light.position", lightPos);

        // ---------- Основание ----------
        glm::mat4 baseModel = glm::mat4(1.0f);
        baseModel = glm::rotate(baseModel, glm::radians(baseAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        lightingShader.setMat4("model", baseModel);
        base.Draw();

        // Arm
        glm::mat4 armModel = baseModel;
        armModel = glm::rotate(armModel, glm::radians(armAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        lightingShader.setMat4("model", armModel);
        arm.Draw();

        // Hand
        glm::mat4 handModel = armModel;
        handModel = glm::rotate(handModel, glm::radians(handAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        lightingShader.setMat4("model", handModel);
        hand.Draw();


        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    glfwTerminate();
    return 0;
}