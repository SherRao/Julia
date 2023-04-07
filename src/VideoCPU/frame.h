/* Represents a frame in the final video output. */
typedef struct _Frame
{
    /* The position of this frame*/
    int index;

    /* The real value of the point of this frame */
    float real;

    /* The imaginary value of the point of this frame */
    float imaginary;

} Frame;
