#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// clang -o main window.c -L/usr/local/lib -lglfw -framework OpenGL

#define GL_SILENCE_DEPRECATION

// Without this gl.h gets included instead of gl3.h
// #define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// For includes related to OpenGL, make sure their are included after glfw3.h
// #include <OpenGL/gl3.h>

#define SIZE 1024 * 2

void errorCallback(int error, const char *description)
{
    fputs(description, stderr);
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void frameBufferResizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main(void)
{
    GLFWwindow *window;

    // Set callback for errors
    // glfwSetErrorCallback(errorCallback);

    // Initialize the library
    if (!glfwInit())
        return -1;

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // // Without these two hints, nothing above OpenGL version 2.1 is supported
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(1024, 1024, "CP431: Julia Sets", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }


    // Set callback for window
    // glfwSetKeyCallback(window, keyCallback);

    // // Set callback fro framebuffer
    // glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);

    // Make the window's context current
    // glfwMakeContextCurrent(window);

    // // Used to avoid screen tearing
    // glfwSwapInterval(1);

    // // OpenGL initializations start from here
    // glClearColor(0.5f, 0.3f, 0.3f, 1.0f);


    // OpenGL initializations end here

    const int LIST_SIZE = SIZE * SIZE; // 1024 * 1024;
    unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * LIST_SIZE * 3);

    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
        {
            data[y * SIZE * 3 + x * 3] = 50;
            data[y * SIZE * 3 + x * 3 + 1] = 255;
            data[y * SIZE * 3 + x * 3 + 2] = 150;
        }

    glfwMakeContextCurrent(window);

    GLuint texture_handle;
    glGenTextures(1, &texture_handle);
    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZE, SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Resize the viewport
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        // glViewport(0, 0, width, height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0,width,0,height,-1,1);
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture_handle);
        glBegin(GL_QUADS);
            glTexCoord2d(0, 0); glVertex2i(0,0);
            glTexCoord2d(1, 0); glVertex2i(SIZE, 0);
            glTexCoord2d(1, 1); glVertex2i(SIZE, SIZE);
            glTexCoord2d(0, 1); glVertex2i(0, SIZE);
        glEnd();
        glDisable(GL_TEXTURE_2D);


            // glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
            // Swap front and back buffers
        glfwSwapBuffers(window);
        glfwWaitEvents();
            // Poll for and process events
            // glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}