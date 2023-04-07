#include <png.h>

typedef struct _Block
{
    int size;
    int is_done;
    int place;
    int width;
    int height;
    png_byte **data;
} Block;
