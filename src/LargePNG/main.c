// mpicc video.c -o hello -O3 -lpng && mpirun -np 5 --use-hwthread-cpus  ./hello
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#include <mpi.h>

#include <complex.h>
#include <math.h>
#include <time.h>

#define PNG_DEBUG 3
#include <png.h>

int x, y;
#define MAX_ITER 50
#define SIZE 1000

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep *row_pointers;

char *file = "a.png";

typedef struct Block
{
    int size;
    int is_done;
    int place;
    int width;
    int height;
    png_byte **data;
} block;

block knapsack;
png_byte **img_data;

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
    // png_write_image(png_ptr, row_pointers);
    png_write_image(png_ptr, knapsack.data);
    // png_write_rows(png_ptr, &knapsack.data, 40);
    // png_write_row(png_ptr, &knapsack.data[4]);
    printf("THERE>>>>>\n");
    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);

    // height = SIZE;
    /* cleanup heap allocation */
    // for (y = 0; y < height; y++)
    //     free(row_pointers[y]);
    // free(row_pointers);

    for (y = 0; y < height; y++)
        free(knapsack.data[y]);
    free(knapsack.data);

    fclose(fp);
}

void pack_calc(int index)
{
    int bit = 0;
    float scale = 1.5;
    float cx = -0.8;
    float cy = 0.156;
    float zx = scale * (float)(width / 2 - x) / (width / 2);
    float zy = scale * (float)(height / 2 - y) / (height / 2);
    int i = 0;
    int c = 0;
    float tmp = zx * zx - zy * zy + cx;

    // CHANGE PIXELS
    for (y = index; y < /*height*/ SIZE + index; y++, c++)
    {
        // printf("%d\n",y);
        png_byte *row = knapsack.data[/*y*/ c];

        for (x = 0; x <= width; x++)
        {
            png_byte *ptr = &(row[x * 4]);

            bit = 0;
            // scale = 1.5;
            // cx = -0.8;
            // cy = 0.156;
            zx = scale * (float)(width / 2 - x) / (width / 2);
            zy = scale * (float)(height / 2 - y) / (height / 2);

            for (i = 0; i < MAX_ITER; i++)
            {
                tmp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = tmp;
                if (zx * zx + zy * zy > 4.0)
                {
                    // bit = 1;
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
            // if (bit == 0)
            //     ptr[0] = 93;
            // else
            //     ptr[0] = 255;

            // ptr[1] = 185;
            // ptr[2] = 239;
            ptr[3] = 255;
            // printf("%d ",x);
        }
        // printf("%d - %d\n", y, bit);
    }
}

void allocate(int s)
{
    img_data = (png_byte **)malloc(sizeof(png_byte *) * s);

    for (y = 0; y < s; y++)
        img_data[y] = (png_byte *)malloc(sizeof(png_bytep) * width);

    knapsack.height = height;
    knapsack.width = width;
    knapsack.size = height * width;
    knapsack.place = 0;
    knapsack.is_done = 0;

    knapsack.data = img_data;
}

int main(int argc, char **argv)
{
    int process_Rank, size_Of_Cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);
    MPI_Status status;

    width = 10840;
    height = 10160;
    int cluster = size_Of_Cluster - 1;
    // cluster = 2 - 1;

    int num_blocks = (height / SIZE); // add plus one edge case

    if (process_Rank == 0)
        allocate(height);
    else
        allocate(SIZE);

    if (process_Rank == 0)
    {
        int wants_work = 1;

        // First give out work
        for (int i = 0; i < cluster; i++)
        {
            knapsack.place = i * SIZE;

            MPI_Recv(&wants_work, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);

            // Send a Knapsack
            MPI_Send(&knapsack, 4, MPI_INT, wants_work, 1, MPI_COMM_WORLD);

            // for (int k = 0; k < SIZE; k++){
            //     MPI_Send(knapsack.data[k], knapsack.width * 4, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
            // }
        }

        printf("HELLLLLLLLOOOOOOO%d\n", num_blocks);
        for (int i = 0; i < num_blocks - cluster; i++)
        {

            MPI_Recv(&wants_work, 1, MPI_INT, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
            MPI_Recv(&knapsack, 4, MPI_INT, wants_work, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int k = 0; k < SIZE; k++)
                MPI_Recv(knapsack.data[k + knapsack.place], knapsack.width * 4, MPI_BYTE, wants_work, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // if (i == num_blocks - 1) knapsack.is_done = 1;

            knapsack.place = i * SIZE + (cluster * SIZE);
            // Send a Knapsack
            MPI_Send(&knapsack, 4, MPI_INT, wants_work, 7, MPI_COMM_WORLD);
            // if(knapsack.is_done != 1)
            // for (int k = 0; k < SIZE; k++)
            //     MPI_Send(knapsack.data[k], knapsack.width * 4, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
        }

        for (int i = 0; i < cluster; i++)
        {
            MPI_Recv(&wants_work, 1, MPI_INT, MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
            MPI_Recv(&knapsack, 4, MPI_INT, wants_work, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int k = 0; k < SIZE; k++)
                MPI_Recv(knapsack.data[k + knapsack.place], knapsack.width * 4, MPI_BYTE, wants_work, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            knapsack.is_done = 1;
            MPI_Send(&knapsack, 4, MPI_INT, wants_work, 7, MPI_COMM_WORLD);
        }

        clock_t begin = clock();
        write_img();
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Write time: %f %d\n", time_spent, knapsack.width);
    }
    else // if(process_Rank == 1)
    {
        // Ask for work
        MPI_Send(&process_Rank, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Recv(&knapsack, 4, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // receive the data matrix
        // for (int k = knapsack.place; k < SIZE + knapsack.place; k++)
        //     MPI_Recv(knapsack.data[k], knapsack.width * 4, MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        pack_calc(knapsack.place);
        printf("heerrre%d\n", knapsack.place);

        // clock_t begin = clock();

        while (knapsack.is_done == 0)
        {

            MPI_Send(&process_Rank, 1, MPI_INT, 0, 5, MPI_COMM_WORLD);
            MPI_Send(&knapsack, 4, MPI_INT, 0, 8, MPI_COMM_WORLD);

            for (int k = 0; k < SIZE; k++)
                // MPI_Send(knapsack.data[k + knapsack.place], knapsack.width * 4, MPI_BYTE, 0, 2, MPI_COMM_WORLD);
                MPI_Send(knapsack.data[k], knapsack.width * 4, MPI_BYTE, 0, 2, MPI_COMM_WORLD);

            printf("NOT >>>>>\n");
            MPI_Recv(&knapsack, 4, MPI_INT, 0, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // if(knapsack.is_done != 1)
            // for (int k = knapsack.place; k < SIZE + knapsack.place; k++)
            //     MPI_Recv(knapsack.data[k], knapsack.width * 4, MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (knapsack.is_done != -1)
                pack_calc(knapsack.place);

            printf("Recieving %d\n", knapsack.place);
        }

        // clock_t end = clock();
        // double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        // printf("Pack time: %f %d\n", time_spent, knapsack.place);
    }

    printf("Hello World from process %d of %d\n", process_Rank, size_Of_Cluster);

    MPI_Finalize();

    return 0;
}
