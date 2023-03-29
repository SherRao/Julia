#include <GL/glut.h>

// define the size of the window
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// define the size of the texture
const int TEXTURE_WIDTH = 256;
const int TEXTURE_HEIGHT = 256;

// create an array of pixels to use as the texture
unsigned char pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 3];

void display()
{
    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // bind the texture
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 1);

    // specify the texture image
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // update a portion of the texture with new pixels
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXTURE_WIDTH / 2, TEXTURE_HEIGHT / 2, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // draw a quad with the texture
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(-1, -1);
    glTexCoord2f(1, 0);
    glVertex2f(1, -1);
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    glTexCoord2f(0, 1);
    glVertex2f(-1, 1);
    glEnd();

    // flush the pipeline and swap the buffers
    glFlush();
    glutSwapBuffers();
}

int main(int argc, char **argv)
{
    // initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    // create a window
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("OpenGL Texture Example");

    // initialize OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // generate a texture object
    glGenTextures(1, &textureId);

    // set texture parameters
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // specify the texture image
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // register the display function
    glutDisplayFunc(display);

    // start the main loop
    glutMainLoop();

    return 0;
}
