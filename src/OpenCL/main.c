#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

// Without this gl.h gets included instead of gl3.h
// #define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// For includes related to OpenGL, make sure their are included after glfw3.h
// #include <OpenGL/gl3.h>

#ifdef __APPLE__
    #include <OpenCL/opencl.h>
#else
    #include <CL/cl.h>
#endif

#define GL_SILENCE_DEPRECATION
#define MAX_SOURCE_SIZE (0x100000)
#define SIZE 1024 * 2
#define LIST_SIZE SIZE *SIZE

/**
 *
 * @brief Function prototypes.
 *
 */
void error_callback(int, const char *);
void key_callback(GLFWwindow *, int, int, int, int);
void frame_buffer_resize_callback(GLFWwindow *, int, int);
void renderer(unsigned char *, unsigned char *);

/**
 *
 * @brief Error callback function.
 * @param error The error code.
 * @param description The error description.
 * @return Null
 *
 */
void error_callback(int error, const char *description) {
    fputs(description, stderr);

    return;
}

/**
 *
 * @brief Key callback function.
 * @param window The window.
 * @param key The key.
 * @param scancode The scancode.
 * @param action The action.
 * @param mods The mods.
 * @return Null
 *
 */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    return;
}

/**
 *
 * @brief Frame buffer resize callback function.
 * @param window The window.
 * @param width The width.
 * @param height The height.
 * @return Null
 *
 */
void frame_buffer_resize_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

/**
 *
 * @brief Render the image.
 * @param data The image data.
 * @param A The image data.
 * @return Null
 *
 */
void renderer(unsigned char *data, unsigned char *A) {
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            data[y * SIZE * 3 + x * 3] = A[y * SIZE * 4 + x * 4];
            data[y * SIZE * 3 + x * 3 + 1] = A[y * SIZE * 4 + x * 4 + 1];
            data[y * SIZE * 3 + x * 3 + 2] = A[y * SIZE * 4 + x * 4 + 2];
        }
    }
}

/**
 *
 * @brief Main function.
 * @param void
 * @return Null
 *
 */
int main(void) {
    clock_t begin = clock();

    if (!glfwInit()) {
        exit(1);
    }

    GLFWwindow *window = glfwCreateWindow(1024, 1024, "CP431: Julia Sets", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(1);
    }

    unsigned char *A = (unsigned char *) malloc(sizeof(unsigned char) * LIST_SIZE * 4);
    float B[2] = {-0.8, 0.143};

    FILE *filename = fopen("julia.cl", "r");
    if (!filename) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }

    char *source_str = (char *) malloc(MAX_SOURCE_SIZE);
    size_t source_size = fread(source_str, 1, MAX_SOURCE_SIZE, filename);

    fclose(filename);

    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, LIST_SIZE * sizeof(unsigned char) * 4, NULL, &ret);
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * 2, NULL, &ret);

    ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0, LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, sizeof(float) * 2, B, 0, NULL, NULL);

    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);

    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

    cl_kernel kernel = clCreateKernel(program, "julia", &ret);

    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);

    size_t global_item_size = LIST_SIZE; 
    size_t local_item_size = 64;         
    
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);

    ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0, LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);
    unsigned char *data = (unsigned char *) malloc(sizeof(unsigned char) * LIST_SIZE * 3);

    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            data[y * SIZE * 3 + x * 3] = A[y * SIZE * 4 + x * 4];
            data[y * SIZE * 3 + x * 3 + 1] = A[y * SIZE * 4 + x * 4 + 1];
            data[y * SIZE * 3 + x * 3 + 2] = A[y * SIZE * 4 + x * 4 + 2];
        }
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
    double time_spent = ((double)(end - begin) / CLOCKS_PER_SEC);
    printf("Pack time: %f < %c >\n", time_spent, A[4]);

    while (!glfwWindowShouldClose(window)) {
        int width;
        int height;
        
        begin = clock();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        B[1] += 0.0001;
        ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &a_mem_obj);
        ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &b_mem_obj);
        ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, sizeof(float) * 2, B, 0, NULL, NULL);
        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
        ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0, LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);
        renderer(data, A);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIZE, SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

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

        glfwSwapBuffers(window);
        glfwWaitEvents();

        glfwPollEvents();

        end = clock();
        time_spent = ((double)(end - begin) / CLOCKS_PER_SEC);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
