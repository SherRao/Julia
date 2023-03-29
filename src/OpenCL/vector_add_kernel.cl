__kernel void vector_add(__global int *A, __global int *B, __global int *C) {
    int width = 1000;
    int height = 1000;
    int x = 0;
    int y = 1;
    // Get the index of the current element
    int i = get_global_id(0);

    // Do the operation
    // C[i] = A[i] + B[i];
    int bit = 0;
    float scale = 1.5;
    float cx = -0.8;
    float cy = 0.156;
    float zx = scale * (float)(width / 2 - x) / (width / 2);
    float zy = scale * (float)(height / 2 - y) / (height / 2);
    int j = 0;
    int c = 0;
    float tmp = zx * zx - zy * zy + cx;

    for (y = 0; y < height; y++){
        C[i] = 1;
        A[i] = 1;
        B[i] = 1;
    }

}