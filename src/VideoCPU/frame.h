/**
 * 
 * @file frame.h
 * @brief Represents a frame in the video.
 * 
 */
typedef struct _Frame {
    int index;       // The position of this frame in the video.
    float real;      // The real value of the point of this frame 
    float imaginary; // The imaginary value of the point of this frame 
} Frame;
