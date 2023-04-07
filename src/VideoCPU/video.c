#include <complex.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include <png.h>
#include "block.h"
#include "frame.h"

#define FRAME_COUNT 100
#define MAX_ITER 50
#define PNG_DEBUG 3
#define SIZE 1000
#define OUTPUT_FILE_NAME "a.png"

int x;
int y;
int width;
int height;
png_byte color_type;
png_byte bit_depth;
png_structp image;
png_infop image_info;

int number_of_passes;
png_bytep *row_pointers;
Frame frame;
Block knapsack;
png_byte **image_data;

/**
 *
 * @brief Write the image to a file.
 *
 */
void write_image()
{
    char buffer[12];
    snprintf(buffer, 12, "%d.png", frame.index);
    file = buffer;
    color_type = PNG_COLOR_TYPE_RGBA;
    bit_depth = 8;
    number_of_passes = 1;

    FILE *file = fopen(file, "wb");
    if (!file)
    {
        printf("[write_png_file] File %s could not be opened for writing", file);
        exit(1);
    }

    image = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!image)
    {
        printf("[write_png_file] png_create_write_struct failed");
        exit(1);
    }

    image_info = png_create_info_struct(image);
    if (!image_info)
    {
        printf("[write_png_file] png_create_info_struct failed");
        exit(1);
    }

    if (setjmp(png_jmpbuf(image)))
    {
        printf("[write_png_file] Error during init_io");
        exit(1);
    }

    png_init_io(image, file);
    if (setjmp(png_jmpbuf(image)))
    {
        printf("[write_png_file] Error during writing header");
        exit(1);
    }

    png_set_IHDR(image, image_info, width, height, bit_depth, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(image, image_info);
    if (setjmp(png_jmpbuf(image)))
    {
        printf("[write_png_file] Error during writing bytes");
        exit(1);
    }

    png_set_compression_level(image, 6);
    png_set_filter(image, 0, 0);
    png_write_image(image, knapsack.data);
    if (setjmp(png_jmpbuf(image)))
    {
        printf("[write_png_file] Error during end of write");
        exit(1);
    }

    png_write_end(image, NULL);
    fclose(file);
}

/**
 *
 * @brief Calculate the mandelbrot set for a given frame.
 * @param index The index of the frame.
 *
 */
void pack_calc(int index)
{
    float scale = 1.5;
    float cx = frame.real;
    float cy = frame.imaginary;
    float zx = scale * (float)(width / 2 - x) / (width / 2);
    float zy = scale * (float)(height / 2 - y) / (height / 2);
    int i = 0;
    int c = 0;
    float tmp = zx * zx - zy * zy + cx;

    for (int y = index; y < height; y++, c++)
    {
        png_byte *row = knapsack.data[y];
        for (int x = 0; x <= width; x++)
        {
            png_byte *ptr = &(row[x * 4]);
            int bit = 0;
            zx = scale * (float)(width / 2 - x) / (width / 2);
            zy = scale * (float)(height / 2 - y) / (height / 2);
            for (int i = 0; i < MAX_ITERATIONS; i++)
            {
                tmp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = tmp;
                if (zx * zx + zy * zy > 4.0)
                {
                    bit = i % 5;
                    if (bit == 0)
                        bit = 5;

                    break;
                }
            }

            if (bit == 0)
            {
                ptr[0] = 0;
                ptr[1] = 0;
                ptr[2] = 0;
            }
            else if (bit == 1)
            {
                ptr[0] = 204;
                ptr[1] = 107;
                ptr[2] = 73;
            }
            else if (bit == 2)
            {
                ptr[0] = 210;
                ptr[1] = 162;
                ptr[2] = 76;
            }
            else if (bit == 3)
            {
                ptr[0] = 236;
                ptr[1] = 230;
                ptr[2] = 194;
            }
            else if (bit == 4)
            {
                ptr[0] = 115;
                ptr[1] = 189;
                ptr[2] = 168;
            }
            else
            {
                ptr[0] = 153;
                ptr[1] = 190;
                ptr[2] = 183;
            }

            ptr[3] = 255;
        }
    }
}

/**
 *
 * @brief Allocate memory for the image data.
 *
 */
void allocate()
{
    image_data = (png_byte **)malloc(sizeof(png_byte *) * height);
    for (int y = 0; y < height; y++)
        image_data[y] = (png_byte *)malloc(sizeof(png_bytep) * width);

    knapsack.height = height;
    knapsack.width = width;
    knapsack.size = height * width;
    knapsack.place = 0;
    knapsack.data = image_data;
}

void root_process()
{
    bool wants_work = true;
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        frame.imaginary = 0.136 + (0.001 * i);
        frame.index = i;
        frame.real = -0.8 + (0.001 * i);

        MPI_Recv(&wants_work, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);
        printf("%d <<<\n", wants_work);
        MPI_Send(&frame, 4, MPI_INT, wants_work, 1, MPI_COMM_WORLD);
    }

    frame.index = -1;
    for (int i = 1; i < processor_count; i++)
        MPI_Send(&frame, 4, MPI_INT, i, 1, MPI_COMM_WORLD);
}

void child_process(int rank, int processor_count)
{
    while (frame.index >= 0)
    {
        MPI_Send(&rank, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Recv(&frame, 4, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (frame.index != -1)
        {
            allocate();
            pack_calc(0);
            write_image();
        }
    }

    printf("Recieving %f - %d - %f \n", frame.imaginary, frame.index, frame.real);
    for (int y = 0; y < height; y++)
        free(knapsack.data[y]);

    free(knapsack.data);
    printf("Hello World from process %d of %d\n", rank, processor_count);
}

int main(int argc, char **argv)
{
    int rank;
    int processor_count;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &processor_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    width = 3840;
    height = 2160;
    frame.imaginary = 0.0;
    frame.index = 0;
    frame.real = 0.0;

    allocate();
    if (rank == 0)
        root_process();

    else
        child_process(rank, processor_count);

    MPI_Finalize();
    return 0;
}
