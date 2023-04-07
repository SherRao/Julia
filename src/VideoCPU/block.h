#include <png.h>

/* Represents a block of data in the final video output. */
typedef struct _Block
{
    /* The size of the block */
    int size;

    /* The position of the block */
    int place;

    /* The width of the block */
    int width;

    /* The height of the block */
    int height;

    /* The location of the image data of the block */
    png_byte **data;

} Block;
