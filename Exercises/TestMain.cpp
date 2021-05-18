#include <iostream>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"


#define STB_IMAGE_IMPLEMENTATION   
#include "stb_image.h"

using namespace std;

glm::mat4 modelMat(1.0);
glm::mat4 viewMat(1.0);
glm::mat4 projMat(1.0);
float fov = 90;
string transformString = "v";

struct Vertex {
    glm::vec3 pos = glm::vec3(0, 0, 0);
    glm::vec4 color = glm::vec4(1, 1, 1, 1);
    glm::vec3 normal = glm::vec3(0, 0, 0);
    glm::vec2 texcoords = glm::vec2(0, 0);

    Vertex() {};
    Vertex(glm::vec3 p) { pos = p; };
    Vertex(glm::vec3 p, glm::vec4 c) { pos = p; color = c; };
    Vertex(glm::vec3 p, glm::vec4 c, glm::vec3 n) { pos = p; color = c; normal = n; };
    Vertex(glm::vec3 p, glm::vec4 c, glm::vec3 n, glm::vec2 t) { pos = p; color = c; normal = n; texcoords = t;  };
};

struct PointLight {
    glm::vec4 pos = glm::vec4(0, 0, 0, 1);
    glm::vec4 color = glm::vec4(1, 1, 1, 1);
};

string vertCode = R"(
#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texcoords;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat3 normMat;

out vec4 interPos; // AFTER view transform
out vec4 interColor;
out vec3 interNorm;
out vec2 uv;

void main() {
    vec4 pos = vec4(position, 1.0);
    vec4 vpos = viewMat * modelMat * pos;
    interPos = vpos;

    vec4 projpos = projMat * vpos;
    gl_Position = projpos;    

    interColor = color;

    interNorm = normMat * normal;

    uv = texcoords;
}
)";

string fragCode = R"(
#version 430 core

layout(location = 0) out vec4 out_color;

struct PointLight {
    vec4 pos;
    vec4 color;
};

uniform PointLight light;

in vec4 interPos;
in vec4 interColor;
in vec3 interNorm;
in vec2 uv;

uniform sampler2D diffuseTexture;

void main() {
    //out_color = interColor;

    
    vec3 UL = vec3(light.pos - interPos);    
    float d = length(UL);
    float at = 1.0 / (d*d + 1.0);

    vec3 N = normalize(interNorm);
    vec3 L = normalize(UL);

    float diffuse = max(0, dot(N, L));
    vec3 diffuseColor = diffuse*vec3(interColor)*vec3(light.color);

    vec4 textureColor = texture(diffuseTexture, uv);
    //diffuseColor = vec3(textureColor);

    diffuseColor *= textureColor.xyz;

    //out_color = vec4(at, at, at, 1.0);
    //out_color = vec4(N, 1.0);
    //out_color = vec4(diffuse, diffuse, diffuse, 1.0);
    out_color = vec4(diffuseColor, 1.0);
    
}
)";

void printRM(string name, glm::mat4& M) {
    cout << name << ":" << endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            cout << M[j][i] << ", ";
        }
        cout << endl;
    }
}

static void key_callback(GLFWwindow* window, int key,
    int scancode, int action,
    int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {

        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
        else if (key == GLFW_KEY_Q) {
            modelMat = glm::rotate(glm::radians(5.0f), glm::vec3(0, 0, 1)) * modelMat;
            transformString = "R(5)*" + transformString;
        }
        else if (key == GLFW_KEY_E) {
            modelMat = glm::rotate(glm::radians(-5.0f), glm::vec3(0, 0, 1)) * modelMat;
            transformString = "R(-5)*" + transformString;
        }
        else if (key == GLFW_KEY_SPACE) {
            modelMat = glm::mat4(1.0);
            transformString = "v";
        }
        else if (key == GLFW_KEY_F) {
            modelMat = glm::scale(glm::vec3(0.8, 1, 1)) * modelMat;
            transformString = "S(0.8, 1.0)*" + transformString;
        }
        else if (key == GLFW_KEY_G) {
            modelMat = glm::scale(glm::vec3(1.2, 1, 1)) * modelMat;
            transformString = "S(1.2, 1.0)*" + transformString;
        }
        else if (key == GLFW_KEY_W) {
            modelMat = glm::translate(glm::vec3(0, 0.1, 0)) * modelMat;
            transformString = "T(0.0, 0.1)*" + transformString;
        }
        else if (key == GLFW_KEY_S) {
            modelMat = glm::translate(glm::vec3(0, -0.1, 0)) * modelMat;
            transformString = "T(0.0, -0.1)*" + transformString;
        }
        else if (key == GLFW_KEY_A) {
            modelMat = glm::translate(glm::vec3(-0.1, 0, 0)) * modelMat;
            transformString = "T(-0.1, 0.0)*" + transformString;
        }
        else if (key == GLFW_KEY_D) {
            modelMat = glm::translate(glm::vec3(0.1, 0, 0)) * modelMat;
            transformString = "T(-0.1, 0.0)*" + transformString;
        }
        else if (key == GLFW_KEY_Z) {
            modelMat = glm::rotate(glm::radians(5.0f), glm::vec3(0, 1, 0)) * modelMat;
            transformString = "Rz(5)*" + transformString;
        }
        else if (key == GLFW_KEY_C) {
            modelMat = glm::rotate(glm::radians(-5.0f), glm::vec3(0, 1, 0)) * modelMat;
            transformString = "Rz(-5)*" + transformString;
        }
        else if (key == GLFW_KEY_M) {
            fov += 1.0;
        }
        else if (key == GLFW_KEY_N) {
            fov -= 1.0;
        }

        printRM("Model", modelMat);
        cout << transformString << endl;
    }
}

void computeOneNormal(vector<Vertex>& v, int i1, int i2, int i3) {
    glm::vec3 p1 = v.at(i1).pos;
    glm::vec3 p2 = v.at(i2).pos;
    glm::vec3 p3 = v.at(i3).pos;

    glm::vec3 N = glm::cross((p2 - p1), (p3 - p1));

    v.at(i1).normal += N;
    v.at(i2).normal += N;
    v.at(i3).normal += N;
}

void computeAllNormals(vector<Vertex>& v, vector<GLuint>& ind) {
    for (int i = 0; i < ind.size(); i += 3) {
        computeOneNormal(v, ind.at(i), ind.at(i + 1), ind.at(i + 2));
    }

    for (int i = 0; i < v.size(); i++) {
        v.at(i).normal = glm::normalize(v.at(i).normal);
    }
}

void makeCylinder(vector<Vertex>& v, vector<GLuint>& ind, float length, float radius, int faceCnt) {
    v.clear();
    ind.clear();

    float angleInc = 360.0 / faceCnt;
    float hlen = length / 2.0;

    for (int i = 0; i < faceCnt; i++) {
        Vertex vf, vb;
        float angle = glm::radians(angleInc * i);

        vf.pos = glm::vec3(-hlen, radius * sin(angle), radius * cos(angle));
        vb.pos = glm::vec3(+hlen, radius * sin(angle), radius * cos(angle));

        vf.color = glm::vec4(1, 0, 0, 1);
        vb.color = glm::vec4(0, 1, 0, 1);

        vf.texcoords = glm::vec2(-0.5, i);
        vb.texcoords = glm::vec2(1.5, i); 

        v.push_back(vf);
        v.push_back(vb);
    }

    int cap = 2 * faceCnt;

    for (int i = 0; i < cap; i += 2) {
        ind.push_back(i);
        ind.push_back((i + 1) % cap);
        ind.push_back((i + 2) % cap);

        ind.push_back((i + 1) % cap);
        ind.push_back((i + 3) % cap);
        ind.push_back((i + 2) % cap);
    }

    computeAllNormals(v, ind);
}


static void error_callback(int error, const char* description) {
    cerr << "ERROR " << error << ": " << description << endl;
}

int main(int argc, char** argv) {
    cout << "GREETINGS HUMANS!" << endl;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        exit(1);
    }

    // Window hints go here

    GLFWwindow* window = glfwCreateWindow(800, 800, "GREAT WINDOW",
        NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);

    glewExperimental = true;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cerr << "GLEW ERROR: ";
        cerr << glewGetErrorString(err) << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        exit(1);
    }

    GLint glMajor, glMinor;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glMinor);
    cout << "OpenGL context version: ";
    cout << glMajor << "." << glMinor << endl;

    GLuint vertID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vertPtr = vertCode.c_str();
    const char* fragPtr = fragCode.c_str();
    glShaderSource(vertID, 1, &vertPtr, NULL);
    glShaderSource(fragID, 1, &fragPtr, NULL);

    glCompileShader(vertID);
    glCompileShader(fragID);

    GLuint progID = glCreateProgram();
    glAttachShader(progID, vertID);
    glAttachShader(progID, fragID);

    glLinkProgram(progID);

    glDeleteShader(vertID);
    glDeleteShader(fragID);

    GLint modelMatLoc = glGetUniformLocation(progID, "modelMat");
    GLint viewMatLoc = glGetUniformLocation(progID, "viewMat");
    GLint projMatLoc = glGetUniformLocation(progID, "projMat");

    vector<Vertex> vertData;
    vector<GLuint> indices;
    makeCylinder(vertData, indices, 1, 0.5, 10);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertData.size() * sizeof(Vertex),
        vertData.data(), GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));

    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(GLuint),
        indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    glClearColor(0.5, 0.15, 0.979797979, 1.0);

    int fwidth, fheight;

    glEnable(GL_DEPTH_TEST);

    PointLight light;
    light.pos = glm::vec4(0, 0.5, 0.5, 1);
    GLint lightPosID = glGetUniformLocation(progID, "light.pos");
    GLint lightColorID = glGetUniformLocation(progID, "light.color");

    GLint normMatLoc = glGetUniformLocation(progID, "normMat");

    string filename = "../test.jpeg";
    int width, height, nrComponents;
    unsigned int textureID = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

    if (data != NULL)
    {
        glGenTextures(1, &textureID);
        GLenum format;
        if (nrComponents == 3)
        {
            format = GL_RGB;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        }
        else if (nrComponents == 4)
        {
            format = GL_RGBA;
        }
        else
        {
            cerr << "INVALID CHANNEL NUMBER! " << nrComponents << endl;
            glfwTerminate();
            exit(1);
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

        stbi_image_free(data);

    }
    else
    {
        cerr << "ERROR: Cannot load texture: " << filename << endl;
        glfwTerminate();
        exit(1);
    }
    
    GLint uniformTextureID = glGetUniformLocation(progID, "diffuseTexture");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);


    while (!glfwWindowShouldClose(window)) {
        //cout << "Rendering..." << endl;
        glfwGetFramebufferSize(window, &fwidth, &fheight);
        glViewport(0, 0, fwidth, fheight);
        //cout << "Width and height: " << fwidth << " " << fheight << endl;

        float aspect = 1.0;
        if (fheight > 0 && fwidth > 0) {
            aspect = ((float)fwidth) / ((float)fheight);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(progID);

        glUniformMatrix4fv(modelMatLoc, 1, false, glm::value_ptr(modelMat));

        viewMat = glm::lookAt(glm::vec3(-1, 1, 1),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0));
        glUniformMatrix4fv(viewMatLoc, 1, false, glm::value_ptr(viewMat));


        float near = 0.01;
        float far = 50.0;
        projMat = glm::perspective(glm::radians(fov), aspect, near, far);
        glUniformMatrix4fv(projMatLoc, 1, false, glm::value_ptr(projMat));

        glm::vec4 viewLight = viewMat * light.pos;
        glUniform4fv(lightPosID, 1, glm::value_ptr(viewLight));
        glUniform4fv(lightColorID, 1, glm::value_ptr(light.color));

        glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(viewMat * modelMat)));
        glUniformMatrix3fv(normMatLoc, 1, GL_FALSE, glm::value_ptr(normMat));

        glUniform1i(uniformTextureID, 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);


        glfwSwapBuffers(window);
        glfwPollEvents();
        this_thread::sleep_for(chrono::milliseconds(15));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &(textureID));


    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VBO);

    glUseProgram(0);
    glDeleteProgram(progID);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}