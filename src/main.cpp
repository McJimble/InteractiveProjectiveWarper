/*
 *  Program to warp an input image based on a transformation whose
 *  values are directly input by the user. The warping will be done
 *  via inverse mapping and be displayed to the screen (and optionally saved)
 */


#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>

#include "ProjectiveWarper.h"
#include "PixelRGBA.h"

ProjectiveWarper warper;

/*
 *  All the functions below just call the proper warper functions so that glut's
 *  main loop can actually work (it takes in function pointers incompatible with classes).
 */
void Display()
{
    warper.DisplayLayers();
}

void Reshape(int newWidth, int newHeight)
{
    warper.HandleWindowReshape(newWidth, newHeight);
}

void HandleKeys(unsigned char key, int x, int y)
{
    warper.HandleKeyPresses(key, x, y);
}

void HandleSpecialKeys(int key, int x, int y)
{
    warper.HandleSpecialKeyPresses(key, x, y);
}

void HandleMouseState(int button, int state, int x, int y)
{
    warper.HandleMouseDownEvent(button, state, x, y);
}

void HandleClickedMouseMotion(int x, int y)
{
    warper.HandleClickedMouseMotion(x, y);
}

void HandleUnclickedMouseMotion(int x, int y)
{
    warper.ResetLastMousePosition(x, y);
}

/*
 *  This one has nothing to do with glut, just thought it would be easier
 *  to organize things this was even though it comes off as something a 
 *  new CS student that just starting learning about functions would do lmao
 */
void InitProgramPrompt()
{
    std::cout << "Welcome to James' Projective Warper\n\n";
    std::cout << "CONTROLS:\n----------------------------------\n";
    std::cout << "N:                      Create New Layer from a specified image file\n";
    std::cout << "S:                      Save current window as an output image\n";
    std::cout << "R:                      Reset currently selected layer to raw image state at origin\n";
    std::cout << "DEL or BACKSPACE:       Delete currently selected layer\n";
    std::cout << "<- or -> arrows:        Shift current layer down or up respectively\n";
    std::cout << "v or ^ arrows:          Select an existing layer below or above currently selected one\n\n";

    std::cout << "NOTES:\n----------------------------------\n";
    std::cout << "- The program only checks for image files in the same directory of the executable.\n\n";
    std::cout << "- The window can be expanded like normal; left click and drag the edges to expand it\n\n";
    std::cout << "- A maximum of 10 layers is allowed\n\n";
    std::cout << "- The current layer will have icons overlapping its four corners, as well as an icon in the center.\n";
    std::cout << "Hover the mouse over a corner and LEFT CLICK to start moving the corner.\n";
    std::cout << "The layer selected will then warp based on the new positions of the corners.\n\n";
    std::cout << "- Left clicking anywhere that isn't a corner of the selected layer will allow you\n";
    std::cout << "to move the entire layer with mouse movement\n\n";
    std::cout << "- Prompts for image file names as well as confirmation of important actions will appear in the console here.\n\n";

    std::cout << "\nPress Enter to begin.\n";
    std::cin.ignore();
}

int main(int argc, char* argv[])
{
    InitProgramPrompt();

    // Start up the glut utilities
    std::cout << "Initializing GLUT...\n";

    glutInit(&argc, argv);

    std::cout << "Creating program window...\n";
    // Create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(warper.GetWindowWidth(), warper.GetWindowHeight());
    glutCreateWindow(PROGRAM_NAME);

    // Set up glut main loop functions, then init.
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(HandleKeys);
    glutSpecialFunc(HandleSpecialKeys);
    glutMouseFunc(HandleMouseState);
    glutMotionFunc(HandleClickedMouseMotion);
    glutPassiveMotionFunc(HandleUnclickedMouseMotion);

    std::cout << "Warper is ready!\n";

    glutMainLoop();

    return 0;
}
