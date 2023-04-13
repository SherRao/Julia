#include <png.h>

/**
 * 
 * @file block.h
 * @brief Represents a block of data in the final video output.
 * 
 */
typedef struct _Block {
    int size;        // The size of the block 
    int place;       // The place of the block
    int width;       // The width of the block
    int height;      //  The height of the block 
    png_byte **data; // The location of the image data of the block 
} Block;
