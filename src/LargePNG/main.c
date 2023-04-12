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

char *FILENAME = "a.png";

png_byte COLOR_TYPE;
png_byte BIT_DEPTH;
png_byte **IMG_DATA;

png_structp PNG_PTR;
png_inis thfop INFO_PTR;
png_bytep *ROW_PTR;

Block knapsack;

/**
 *
 * @brief Function prototypes.
 *
 */
void write_img();
void calculate(int index);  
void allocate(int size);

/**
 *
 * @brief Write the image to a file.
 * @param Null
 * @return Null
 *
 */
void write_img() {
    COLOR_TYPE = PNG_COLOR_TYPE_RGBA;
    BIT_DEPTH = 8;

    NUM_PASSES = 1;

    FILE *fp = fopen(FILENAME, "wb");

    if (!fp) {
        fprintf(stderr, "[write_png_file] File %s could not be opened for writing", FILENAME);
    }

    PNG_PTR = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!PNG_PTR) {
        fprintf(stderr, "[write_png_file] png_create_write_struct failed");
    }

    INFO_PTR = png_create_info_struct(PNG_PTR);

    if (!INFO_PTR) {
        fprintf(stderr, "[write_png_file] png_create_info_struct failed");
    }

    if (setjmp(png_jmpbuf(PNG_PTR))) {
        fprintf(stderr, "[write_png_file] Error during init_io");
    }

    png_init_io(PNG_PTR, fp);

    if (setjmp(png_jmpbuf(PNG_PTR))) {
        fprintf(stderr, "[write_png_file] Error during writing header");
    }

    png_set_IHDR(PNG_PTR, INFO_PTR, WIDTH, HEIGHT,
                 BIT_DEPTH, COLOR_TYPE, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(PNG_PTR, INFO_PTR);

    if (setjmp(png_jmpbuf(PNG_PTR))) {
        fprintf(stderr, "[write_png_file] Error during writing bytes");
    }

    png_set_compression_level(PNG_PTR, 6);
    png_set_filter(PNG_PTR, 0, 0);

    fprintf(stdout, "HERE<<<<\n");
    png_write_image(PNG_PTR, knapsack.data);
    fprintf(stdout, "THERE>>>>>\n");

    if (setjmp(png_jmpbuf(PNG_PTR))) {
        fprintf(stderr, "[write_png_file] Error during end of write");
    }

    png_write_end(PNG_PTR, NULL);

    for (y = 0; y < HEIGHT; y++) {
        free(knapsack.data[y]);
    }

    free(knapsack.data);

    fclose(fp);

    return;
}

/**
 *
 * @brief Calculate the mandelbrot set.
 * @param index The index of the block.
 * @return Null
 *
 */
void calculate(int index) {
    int bit = 0;
    float scale = 1.5;

    float cx = -0.8;
    float cy = 0.156;
    
    float zx = (scale * (float)(WIDTH / 2 - x) / (WIDTH / 2));
    float zy = (scale * (float)(HEIGHT / 2 - y) / (HEIGHT / 2));
    
    int i = 0;
    int c = 0;
    
    float temp = (zx * zx - zy * zy + cx);

    for (y = index; y < SIZE + index; y++, c++) {
        png_byte *row = knapsack.data[c];

        for (x = 0; x <= WIDTH; x++) {
            png_byte *ptr = &(row[x * 4]);

            bit = 0;
            zx = (scale * (float)(WIDTH / 2 - x) / (WIDTH / 2));
            zy = (scale * (float)(HEIGHT / 2 - y) / (HEIGHT / 2));

            for (i = 0; i < MAX_ITER; i++) {
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
 * @brief Allocate the image data.
 * @param size The size of the image.
 * @return Null
 *
 */
void allocate(int size) {
    IMG_DATA = (png_byte **) malloc(sizeof(png_byte *) * size);

    for (y = 0; y < size; y++) {
        IMG_DATA[y] = (png_byte *) malloc(sizeof(png_bytep) * WIDTH);
    }

    knapsack.height = HEIGHT;
    knapsack.width = WIDTH;
    knapsack.size = HEIGHT * WIDTH;
    knapsack.place = 0;
    knapsack.is_done = 0;

    knapsack.data = IMG_DATA;

    return;
}

/**
 *
 * @brief The main function.
 * @param argc The number of arguments.
 * @param argv The arguments.
 * @return 0
 *
 */
int main(int argc, char **argv) {
    int process_rank, cluster_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &cluster_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
    MPI_Status status;

    WIDTH = 10840;
    HEIGHT = 10160;
    
    int cluster = cluster_size - 1;

    int num_blocks = (HEIGHT / SIZE); 

    if (process_rank == 0) {
        allocate(HEIGHT);
    } else {
        allocate(SIZE);
    }

    if (process_rank == 0) {
        int next = 1;

        for (int i = 0; i < cluster; i++) {
            knapsack.place = (i * SIZE);

            MPI_Recv(&next, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);
            MPI_Send(&knapsack, 4, MPI_INT, next, 1, MPI_COMM_WORLD);
        }

        for (int i = 0; i < num_blocks - cluster; i++) {
            MPI_Recv(&next, 1, MPI_INT, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
            MPI_Recv(&knapsack, 4, MPI_INT, next, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int k = 0; k < SIZE; k++) {
                MPI_Recv(knapsack.data[k + knapsack.place], knapsack.width * 4, MPI_BYTE, next, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            knapsack.place = i * SIZE + (cluster * SIZE);
            MPI_Send(&knapsack, 4, MPI_INT, next, 7, MPI_COMM_WORLD);
        }

        for (int i = 0; i < cluster; i++) {
            MPI_Recv(&next, 1, MPI_INT, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
            MPI_Recv(&knapsack, 4, MPI_INT, next, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int k = 0; k < SIZE; k++) {
                MPI_Recv(knapsack.data[k + knapsack.place], knapsack.width * 4, MPI_BYTE, next, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            knapsack.is_done = 1;
            MPI_Send(&knapsack, 4, MPI_INT, next, 7, MPI_COMM_WORLD);
        }

        clock_t begin = clock();
        write_img();
        clock_t end = clock();

        double time_spent = ((double)(end - begin) / CLOCKS_PER_SEC);

        fprintf(stdout, "Write time: %f %d\n", time_spent, knapsack.width);

    } else {
        MPI_Send(&process_rank, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Recv(&knapsack, 4, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        calculate(knapsack.place);

        fprintf(stdout, "Recieving %d\n", knapsack.place);

        while (knapsack.is_done == 0) {
            MPI_Send(&process_rank, 1, MPI_INT, 0, 5, MPI_COMM_WORLD);
            MPI_Send(&knapsack, 4, MPI_INT, 0, 8, MPI_COMM_WORLD);

            for (int k = 0; k < SIZE; k++) {
                MPI_Send(knapsack.data[k], knapsack.WIDTH * 4, MPI_BYTE, 0, 2, MPI_COMM_WORLD);
            }

            fprintf(stdout, "Sending %d\n", knapsack.place);
            MPI_Recv(&knapsack, 4, MPI_INT, 0, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
            if (knapsack.is_done != -1) {
                calculate(knapsack.place);
            }

            fprintf(stdout, "Recieving %d\n", knapsack.place);
        }
    }

    MPI_Finalize();

    return 0;
}
