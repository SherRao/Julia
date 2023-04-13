#include <png.h>

/**
 * 
 * @file block.h
 * @brief Represents a block of data in the final video output.
 * 
 */
typedef struct _Block {
    int size;        // The size of the block in bytes
    int is_done;     // Whether the block has been filled with data
    int place;       // The place in the final video output
    int width;       // The width of the block in pixels
    int height;      // The height of the block in pixels
    png_byte **data; // The data of the block
} Block;
