#include <complex.h>
#include <math.h>
#include <mpi.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "block.h"
#include "frame.h"

#define FRAME_COUNT 100
#define MAX_ITER 50
#define PNG_DEBUG 3
#define SIZE 1000
#define OUTPUT_FILE_NAME "a.png"

/**
 *
 * @brief Global variables.
 *
 */
int x;
int y;

int WIDTH;
int HEIGHT;
int NUM_PASSES;

png_byte COLOR_TYPE;
png_byte BIT_DEPTH;
png_byte **IMAGE_DATA;

png_structp IMAGE;
png_infop IMAGE_INFO;
png_bytep *ROW_PTR;

Frame FRAME;
Block KNAPSACK;

/**
 *
 * @brief Function prototypes.
 *
 */
void write_image();
void calculate(int index);
void allocate();
void root_process();
void child_process(int rank, int processor_count);

/**
 *
 * @brief Write the image to a file.
 * @param void
 * @return void
 *
 */
void write_image() {
    char buffer[12];
    snprintf(buffer, 12, "%d.png", FRAME.index);
    file = buffer;
    COLOR_TYPE = PNG_COLOR_TYPE_RGBA;
    BIT_DEPTH = 8;
    NUM_PASSES = 1;

    FILE *file = fopen(file, "wb");
    if (!file) {
        fprintf(stderr, "[write_png_file] File %s could not be opened for writing", file);
        exit(1);
    }

    IMAGE = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!IMAGE) {
        fprintf(stderr, "[write_png_file] png_create_write_struct failed");
        exit(1);
    }

    IMAGE_INFO = png_create_info_struct(image);
    if (!IMAGE_INFO) {
        fprintf(stderr, "[write_png_file] png_create_info_struct failed");
        exit(1);
    }

    if (setjmp(png_jmpbuf(IMAGE))) {
        fprintf(stderr, "[write_png_file] Error during init_io");
        exit(1);
    }

    png_init_io(IMAGE, file);
    if (setjmp(png_jmpbuf(IMAGE))) {
        fprintf(stderr, "[write_png_file] Error during writing header");
        exit(1);
    }

    png_set_IHDR(IMAGE, IMAGE_INFO, WIDTH, HEIGHT, BIT_DEPTH, COLOR_TYPE,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(IMAGE, IMAGE_INFO);

    if (setjmp(png_jmpbuf(IMAGE))) {
        fprintf(stderr, "[write_png_file] Error during writing bytes");
        exit(1);
    }

    png_set_compression_level(IMAGE, 6);
    png_set_filter(IMAGE, 0, 0);
    png_write_image(IMAGE, KNAPSACK.data);

    if (setjmp(png_jmpbuf(IMAGE))) {
        fprintf(stderr, "[write_png_file] Error during end of write");
        exit(1);
    }

    png_write_end(IMAGE, NULL);
    fclose(file);

    return;
}

/**
 *
 * @brief Calculate the mandelbrot set for a given frame.
 * @param index The index of the frame.
 * @return void
 *
 */
void calculate(int index) {
    float scale = 1.5;
    float cx = FRAME.real;
    float cy = FRAME.imaginary;
    float zx = (scale * (float)(WIDTH / 2 - x) / (WIDTH / 2));
    float zy = (scale * (float)(HEIGHT / 2 - y) / (HEIGHT / 2));
    int i = 0;
    int c = 0;
    float temp = (zx * zx - zy * zy + cx);

    for (int y = index; y < HEIGHT; y++, c++) {
        png_byte *row = KNAPSACK.data[y];
        for (int x = 0; x <= WIDTH; x++) {
            png_byte *ptr = &(row[x * 4]);
            int bit = 0;
            zx = (scale * (float)(WIDTH / 2 - x) / (WIDTH / 2));
            zy =( scale * (float)(HEIGHT / 2 - y) / (HEIGHT / 2));
            for (int i = 0; i < MAX_ITERATIONS; i++) {
                temp = (zx * zx - zy * zy + cx);
                zy = (2.0 * zx * zy + cy);
                zx = temp;
                if (zx * zx + zy * zy > 4.0) {
                    bit = i % 5;
                    if (bit == 0) {
                        bit = 5;
                    }

                    break;
                }
            }

            if (bit == 0) {
                ptr[0] = 0;
                ptr[1] = 0;
                ptr[2] = 0;
            } else if (bit == 1) {
                ptr[0] = 204;
                ptr[1] = 107;
                ptr[2] = 73;
            } else if (bit == 2) {
                ptr[0] = 210;
                ptr[1] = 162;
                ptr[2] = 76;
            } else if (bit == 3) {
                ptr[0] = 236;
                ptr[1] = 230;
                ptr[2] = 194;
            } else if (bit == 4) {
                ptr[0] = 115;
                ptr[1] = 189;
                ptr[2] = 168;
            } else {
                ptr[0] = 153;
                ptr[1] = 190;
                ptr[2] = 183;
            }

            ptr[3] = 255;
        }
    }

    return;
}

/**
 *
 * @brief Allocate memory for the image data.
 * @param void
 * @return void
 *
 */
void allocate() {
    IMAGE_DATA = (png_byte **) malloc(sizeof(png_byte *) * HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        IMAGE_DATA[y] = (png_byte *) malloc(sizeof(png_bytep) * WIDTH);
    }

    KNAPSACK.height = HEIGHT;
    KNAPSACK.width = WIDTH;
    KNAPSACK.size = HEIGHT * WIDTH;
    KNAPSACK.place = 0;
    KNAPSACK.data = IMAGE_DATA;

    return;
}

/**
 *
 * @brief Code ran by process rank 0 to handle results from all child processes.
 * @param void
 * @return void
 *
 */
void root_process() {
    bool next = true;

    for (int i = 0; i < FRAME_COUNT; i++) {
        FRAME.imaginary = 0.136 + (0.001 * i);
        FRAME.index = i;
        FRAME.real = -0.8 + (0.001 * i);

        MPI_Recv(&next, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);
        fprintf(stdout, "%d <<<\n", next);
        MPI_Send(&FRAME, 4, MPI_INT, next, 1, MPI_COMM_WORLD);
    }

    FRAME.index = -1;

    for (int i = 1; i < processor_count; i++) {
        MPI_Send(&FRAME, 4, MPI_INT, i, 1, MPI_COMM_WORLD);
    }

    return;
}

/**
 *
 * @brief Code ran by all child processes to handle work from the root process.
 * @param rank The rank of the child process.
 * @param processor_count The total number of processes.
 * @return void
 *
 */
void child_process(int rank, int processor_count) {
    while (FRAME.index >= 0) {
        MPI_Send(&rank, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Recv(&FRAME, 4, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (FRAME.index != -1) {
            allocate();
            calculate(0);
            write_image();
        }
    }

    fprintf(stdout, "Recieving %f - %d - %f \n", FRAME.imaginary, FRAME.index, FRAME.real);

    for (int y = 0; y < HEIGHT; y++) {
        free(KNAPSACK.data[y]);
    }

    free(KNAPSACK.data);

    return;
}

/**
 *
 * @brief Main function.
 * @param argc The number of arguments.
 * @param argv The arguments.
 * @return 0
 *
 */
int main(int argc, char **argv) {
    int rank;
    int processor_count;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &processor_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    WIDTH = 3840;
    HEIGHT = 2160;

    FRAME.imaginary = 0.0;
    FRAME.index = 0;
    FRAME.real = 0.0;

    allocate();

    if (rank == 0) {
        root_process();
    } else {
        child_process(rank, processor_count);
    }

    MPI_Finalize();

    return 0;
}
