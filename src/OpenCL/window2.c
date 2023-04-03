#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// clang -o main window.c -L/usr/local/lib -lglfw -framework OpenGL
// clang - o main window2.c - lglfw - framework OpenGL - framework OpenCL &&./ main

#define GL_SILENCE_DEPRECATION

// Without this gl.h gets included instead of gl3.h
// #define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_SOURCE_SIZE (0x100000)
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
// For includes related to OpenGL, make sure their are included after glfw3.h
// #include <OpenGL/gl3.h>

#define SIZE 1024 * 2

void renderer(unsigned char *data, unsigned char *A);

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
    clock_t begin = clock();
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


        // START OPENCL

        // Create the two input vectors
        int i;
        const int LIST_SIZE = SIZE * SIZE; // 1024 * 1024;
        unsigned char *A = (unsigned char *)malloc(sizeof(unsigned char) * LIST_SIZE * 4);
        // float B[2] = {-0.8, 0.143};
        float B[2] = {-1.0, 0.0};
        // float B[2] = {0.3, -0.4};
        // float B[2] = {-0.1, 0.8};

        // Load the kernel source code into the array source_str
        FILE *fp;
        char *source_str;
        size_t source_size;

        fp = fopen("julia.cl", "r");
        if (!fp)
        {
            fprintf(stderr, "Failed to load kernel.\n");
            exit(1);
        }
        source_str = (char *)malloc(MAX_SOURCE_SIZE);
        source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);

        // Get platform and device information
        cl_platform_id platform_id = NULL;
        cl_device_id device_id = NULL;
        cl_uint ret_num_devices;
        cl_uint ret_num_platforms;
        cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
        // cl_uchar ret_num_devices;
        // cl_uchar ret_num_platforms;
        // cl_char ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);

        ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1,
                             &device_id, &ret_num_devices);

        // Create an OpenCL context
        cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

        // Create a command queue
        cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

        // Create memory buffers on the device for each vector
        cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                          LIST_SIZE * sizeof(unsigned char) * 4, NULL, &ret);
        cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                          sizeof(float) * 2, NULL, &ret);

        // Copy the lists A and B to their respective memory buffers
        ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
                                   LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);
        ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0,
                                   sizeof(float) * 2, B, 0, NULL, NULL);

        // Create a program from the kernel source
        cl_program program = clCreateProgramWithSource(context, 1,
                                                       (const char **)&source_str, (const size_t *)&source_size, &ret);

        // Build the program
        ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

        // Create the OpenCL kernel
        cl_kernel kernel = clCreateKernel(program, "julia", &ret);

        // Set the arguments of the kernel
        ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
        ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);

        // Execute the OpenCL kernel on the list
        size_t global_item_size = LIST_SIZE; // Process the entire lists
        size_t local_item_size = 64;         // Divide work items into groups of 64
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                     &global_item_size, &local_item_size, 0, NULL, NULL);

        // Read the memory buffer C on the device to the local variable C
        ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
                                  LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);

        //OPEN CL DONE
    

    // const int LIST_SIZE = SIZE * SIZE; // 1024 * 1024;
    unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * LIST_SIZE * 3);

    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
        {
            data[y * SIZE * 3 + x * 3] = A[y * SIZE * 4 + x * 4 ];
            data[y * SIZE * 3 + x * 3 + 1] = A[y * SIZE * 4 + x * 4 + 1];
            data[y * SIZE * 3 + x * 3 + 2] = A[y * SIZE * 4 + x * 4 + 2];
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

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Pack time: %f < %c >\n", time_spent, A[4]);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        begin = clock();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        B[1] += 0.0001;
        // printf(" %f ", B[1]);

        // Generate a frame

        ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
        ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
        // ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
        //                            LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);
        ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0,
                                   sizeof(float) * 2, B, 0, NULL, NULL);
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                     &global_item_size, &local_item_size, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
                                  LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);
        renderer(data, A);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZE, SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        // Resize the viewport
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, 0, height, -1, 1);
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture_handle);
        glBegin(GL_QUADS);
        glTexCoord2d(0, 0);
        glVertex2i(0, 0);
        glTexCoord2d(1, 0);
        glVertex2i(SIZE, 0);
        glTexCoord2d(1, 1);
        glVertex2i(SIZE, SIZE);
        glTexCoord2d(0, 1);
        glVertex2i(0, SIZE);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        // glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        // Swap front and back buffers
        glfwSwapBuffers(window);
        glfwWaitEvents();
        // Poll for and process events
        glfwPollEvents();
        end = clock();
        time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        // printf("Pack time: %f < %c >\n", time_spent, A[4]);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void renderer(unsigned char *data, unsigned char *A)
{
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
        {
            data[y * SIZE * 3 + x * 3] = A[y * SIZE * 4 + x * 4];
            data[y * SIZE * 3 + x * 3 + 1] = A[y * SIZE * 4 + x * 4 + 1];
            data[y * SIZE * 3 + x * 3 + 2] = A[y * SIZE * 4 + x * 4 + 2];
        }
}