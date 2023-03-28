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
    int place;
    int width;
    int height;
    png_byte **data;
} block;

typedef struct Frame{
    int index;
    float real;
    float imaginary;
} frame;

frame fr;
block knapsack;
png_byte **img_data;

void write_img()
{
    char buf[12];
    snprintf(buf, 12, "%d.png", fr.index);
    file = buf;

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

    // printf("HERE<<<<\n");
    // png_write_image(png_ptr, row_pointers);
    png_write_image(png_ptr, knapsack.data);
    // png_write_rows(png_ptr, &knapsack.data, 40);
    // png_write_row(png_ptr, &knapsack.data[4]);
    // printf("THERE>>>>>\n");
    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);


    for (y = 0; y < height; y++)
        free(knapsack.data[y]);
    free(knapsack.data);

    fclose(fp);
}

void pack_calc(int index)
{
    int bit = 0;
    float scale = 1.5;
    float cx = fr.real;
    float cy = fr.imaginary;
    float zx = scale * (float)(width / 2 - x) / (width / 2);
    float zy = scale * (float)(height / 2 - y) / (height / 2);
    int i = 0;
    int c = 0;
    float tmp = zx * zx - zy * zy + cx;

    // CHANGE PIXELS
    for (y = index; y < height /*SIZE + index*/; y++, c++)
    {
        // printf("%d\n",y);
        png_byte *row = knapsack.data[y];

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
                    bit = 1;
                    break;
                }
            }

            if (bit == 0)
                ptr[0] = 93;
            else
                ptr[0] = 255;

            ptr[1] = 185;
            ptr[2] = 239;
            ptr[3] = 255;
            // printf("%d ",x);
        }
        // printf("%d - %d\n", y, bit);
    }
}

void allocate()
{
    img_data = (png_byte **)malloc(sizeof(png_byte *) * height);

    for (y = 0; y < height; y++)
        img_data[y] = (png_byte *)malloc(sizeof(png_bytep) * width);

    knapsack.height = height;
    knapsack.width = width;
    knapsack.size = height * width;
    knapsack.place = 0;

    knapsack.data = img_data;
}


int main(int argc, char **argv)
{
    int process_Rank, size_Of_Cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);
    MPI_Status status;

    width = 3840;
    height = 2160;

    fr.imaginary = 0.0;
    fr.index = 0;
    fr.real = 0.0;

    allocate();

    // Tags: 1 - Sending Block info, 2 - Sending index, 3 - Work request
    if (process_Rank == 0)
    {
        int num_frames = 100;
        int wants_work = 1;
        
        for(int i = 0; i < num_frames; i++){
            fr.imaginary = 0.136 + (0.01 * i);
            fr.index = i;
            fr.real = -0.8;

            //Ask for worker
            MPI_Recv(&wants_work, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);

            printf("%d <<<\n", wants_work);
            MPI_Send(&fr, 4, MPI_INT, wants_work, 1, MPI_COMM_WORLD);
            // for (int k = 0; k < height; k++)
            //     MPI_Send(knapsack.data[k], knapsack.width * 4, MPI_BYTE, wants_work, 1, MPI_COMM_WORLD);
        }
        
        fr.index = -1;
        for(int i = 1; i < size_Of_Cluster; i++)
            MPI_Send(&fr, 4, MPI_INT, i, 1, MPI_COMM_WORLD);
    }
    else //if(process_Rank == 1)
    {

        while(fr.index >= 0){
            MPI_Send(&process_Rank, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);

            MPI_Recv(&fr, 4, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // receive the data matrix
            // for (int k = knapsack.place; k < height; k++)
            //     MPI_Recv(knapsack.data[k], knapsack.width * 4, MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(fr.index != -1){
                allocate();
                pack_calc(0);
                write_img();
            }
        }



       

        printf("Recieving %f - %d - %f \n", fr.imaginary, fr.index, fr.real);
    }

    printf("Hello World from process %d of %d\n", process_Rank, size_Of_Cluster);

    MPI_Finalize();

    return 0;
}