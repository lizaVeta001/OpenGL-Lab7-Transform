#define GLEW_DLL
#define GLFW_DLL

#include <iostream>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Model.h"

// ---------------------------------------------------------------------
// Шейдеры (встроены в код, но мы будем загружать из файлов)
// ---------------------------------------------------------------------
#include <fstream>
#include <sstream>
#include <string>

std::string readFile(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ---------------------------------------------------------------------
// Класс Shader
// ---------------------------------------------------------------------
class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vStr = readFile(vertexPath);
        std::string fStr = readFile(fragmentPath);
        const char* vertexCode = vStr.c_str();
        const char* fragmentCode = fStr.c_str();

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

// ---------------------------------------------------------------------
// Камера
// ---------------------------------------------------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 10.0f);
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
// Аффинные преобразования модели
glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 modelRotation = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 modelScale = glm::vec3(1.0f, 1.0f, 1.0f);
// ---------------------------------------------------------------------
// Основная функция
// ---------------------------------------------------------------------
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Lab6 - Phong Lighting", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Загрузка шейдеров из файлов
    Shader lightingShader("vertex.glsl", "fragment.glsl");

    // Загрузка модели
    Model ourModel("model.obj");

    // Настройка материала (розовый с блеском)
    lightingShader.use();
    lightingShader.setVec3("material.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("material.diffuse", glm::vec3(0.8f, 0.4f, 0.6f));
    lightingShader.setVec3("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("material.shininess", 32.0f);

    // Настройка источника света
    glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, 3.0f);
    lightingShader.setVec3("light.position", lightPos);
    lightingShader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("light.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    lightingShader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float speed = 3.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        // Управление моделью
        float moveSpeed = 2.0f * dt;
        float rotSpeed = 50.0f * dt;
        float scaleSpeed = 1.0f * dt;

        // Перемещение (стрелки)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            modelPosition.y += moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            modelPosition.y -= moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            modelPosition.x -= moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            modelPosition.x += moveSpeed;

        // Вращение (Q / E)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            modelRotation.y += rotSpeed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            modelRotation.y -= rotSpeed;

        // Масштабирование (R / F)
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            modelScale += glm::vec3(scaleSpeed);
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            modelScale -= glm::vec3(scaleSpeed);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, modelScale);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        lightingShader.use();
        lightingShader.setMat4("model", model);
        lightingShader.setMat4("view", view);
        lightingShader.setMat4("projection", projection);
        lightingShader.setVec3("viewPos", cameraPos);
        lightingShader.setVec3("light.position", lightPos);

        ourModel.Draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}