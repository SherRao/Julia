// Useful: https://www.informit.com/articles/article.aspx?p=1732873&seqNum=3
__kernel void julia(__global uchar3 *A, __global float *B){ //, const unsigned int count){ //(__global int *A, __global int *B, __global int *C) {
    // Get the index of the current element
    int i = get_global_id(0);

    int SIZE = 128 * 16;
    
    int width = SIZE;
    int height = SIZE;
    int MAX_ITER = 256;
    
    int x = (i % SIZE);
    int y = (int) i / SIZE;
    

    // Do the operation
    int bit = 0;
    float scale = 1.5;
    float cx = B[0];// -0.8;
    float cy = B[1];// 0.153;
    float zx = scale * (float)(width / 2 - x) / (width / 2);
    float zy = scale * (float)(height / 2 - y) / (height / 2);
    int j = 0;
    int c = 0;
    float tmp = zx * zx - zy * zy + cx;


        for (j = 0; j < MAX_ITER; j++)
            {
                tmp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = tmp;
                if (zx * zx + zy * zy > 4.0)
                {
                    //bit = 1;
                    bit = j % 5;
                    break;
                }
            }
    
    // if(i % 1024 == 0)
    // A[i] = '\n';
    // else{
        // A[i].x = x;
        // A[i].y = y;
        // A[i].z = 255;
    // }

    if (bit == 0){
        A[i].x = 204;
        A[i].y = 196;
        A[i].z = 53;
    }
    else if (bit == 1){
        A[i].x = 20;
        A[i].y = 42;
        A[i].z = 231;
    }
    else if (bit == 2){
        A[i].x = 132;
        A[i].y = 196;
        A[i].z = 32;
    }
    else if (bit == 3){
        A[i].x = 93;
        A[i].y = 30;
        A[i].z = 105;
    }
    else if (bit == 4){
        A[i].x = 43;
        A[i].y = 211;
        A[i].z = 50;
    }
    else{
        A[i].x = 53;
        A[i].y = 181;
        A[i].z = 204;
    }
}




