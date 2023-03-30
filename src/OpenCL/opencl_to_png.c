#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// clang -framework OpenCL opencl_to_png.c -o run -lpng && ./run

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

#include <mpi.h>

#include <complex.h>
#include <math.h>

#define PNG_DEBUG 3
#include <png.h>

int x, y;
#define MAX_ITER 50
#define SIZE 128 *16 

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep *row_pointers;

char *file = "a.png";
void write_img();
void pack_calc(int index, unsigned char *A);

// clang -framework OpenCL julia_opencl.c -o run
int main(void)
{

    clock_t begin = clock();

    // Create the two input vectors
    int i;
    const int LIST_SIZE = SIZE * SIZE; // 1024 * 1024;
    unsigned char *A = (unsigned char *)malloc(sizeof(unsigned char) * LIST_SIZE * 4);
  

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

    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
                               LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1,
                                                   (const char **)&source_str, (const size_t *)&source_size, &ret);

    // Build the program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "julia", &ret);

    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);

    // Execute the OpenCL kernel on the list
    size_t global_item_size = LIST_SIZE; // Process the entire lists
    size_t local_item_size = 64;         // Divide work items into groups of 64
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                 &global_item_size, &local_item_size, 0, NULL, NULL);

    // Read the memory buffer C on the device to the local variable C
    ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
                              LIST_SIZE * sizeof(unsigned char) * 4, A, 0, NULL, NULL);

    // Display the result to the screen
    // for (i = 0; i < LIST_SIZE * 4; i++)
    //     printf("%d ", A[i]);
    // printf("%lu +\n", sizeof(unsigned char) * LIST_SIZE * 3);

    pack_calc(0, A);
   

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Pack time: %f < %c >\n", time_spent, A[4]);
    
    write_img();

    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
    // ret = clReleaseMemObject(b_mem_obj);
    // ret = clReleaseMemObject(c_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    


    
    free(A);
    
    return 0;
}

void write_img()
{
    
    color_type = PNG_COLOR_TYPE_RGBA;
    bit_depth = 8;

    number_of_passes = 1;

    // Start File
    FILE *fp = fopen(file, "wb");
    if (!fp)
        printf("[write_png_file] File %s could not be opened for writing", file);

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        printf("[write_png_file] png_create_write_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        printf("[write_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[write_png_file] Error during init_io");

    png_init_io(png_ptr, fp);

    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[write_png_file] Error during writing header");

    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[write_png_file] Error during writing bytes");

    // Decrease write time by 10x
    png_set_compression_level(png_ptr, 6);
    png_set_filter(png_ptr, 0, 0);

    printf("HERE<<<<\n");
    png_write_image(png_ptr, row_pointers);
    // png_write_image(png_ptr, knapsack.data);
    // png_write_rows(png_ptr, &knapsack.data, 40);
    // png_write_row(png_ptr, &knapsack.data[4]);
    printf("THERE>>>>>\n");
    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);

    // height = SIZE;
    /* cleanup heap allocation */
    for (y = 0; y < height; y++)
        free(row_pointers[y]);
    free(row_pointers);


    fclose(fp);
}

void pack_calc(int index, unsigned char *A)
{
    height = SIZE;
    width = SIZE;
    // printf("%d - %d - %d - %d - %d\n", A[0], A[1024*4+1024*4], A[2], A[3], A[300]);

    row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (y = 0; y < height; y++)
        row_pointers[y] = (png_byte *)malloc(sizeof(png_bytep) * width);


    // CHANGE PIXELS
    for (y = 0; y < height; y++)
    {
        // printf("%d\n",y);
        png_byte *row = row_pointers[y];

        for (x = 0; x < width; x++)
        {
            png_byte *ptr = &(row[x * 4]);

            ptr[0] = A[y*SIZE*4 + x*4];
            ptr[1] = A[y * SIZE * 4 + x * 4 + 1];
            ptr[2] = A[y * SIZE * 4 + x * 4 + 2];

            ptr[3] = 255;
            // printf("%d ", x);
        }
        // printf("\n");
    }
}