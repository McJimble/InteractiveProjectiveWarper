#include "ProjectiveWarper.h"

const int ProjectiveWarper::MAX_LAYERS = 10;

ProjectiveWarper::ProjectiveWarper()
{
    windowHeight = 500;
    windowWidth = 500;
    activeLayer = -1;
    mouseMoveX = 0;
    mouseMoveY = 0;
    lastMouseX = INT_MIN;
    lastMouseY = INT_MIN;

    warpIconRange = 20.0f;

    Matrix3D temp = Matrix3D::Identity();

    // Load icons at start them at positions off screen.
    cornerIcon = std::make_unique<Layer>();
    ReadImageFile(cornerIcon.get(), "./icons/cornerblob.png");
    cornerIcon->rasterPosX = INT_MIN;
    cornerIcon->rasterPosX = INT_MIN;
    cornerIcon->InvWarpLayer(temp);

    centerIcon = std::make_unique<Layer>();
    ReadImageFile(centerIcon.get(), "./icons/centerblob.png");
    centerIcon->rasterPosX = INT_MIN;
    centerIcon->rasterPosX = INT_MIN;
    centerIcon->InvWarpLayer(temp);
}

ProjectiveWarper::~ProjectiveWarper()
{

}

/*
 *	Prompts the user for an image file name. If image found, place image
 *	data into given layer using OIIO helper functions.
 *  This will overwrite any data stored in the layer. Only use when making new one.
 */
bool ProjectiveWarper::ReadImageFile(Layer* writeToLayer, std::string openFileName)
{
    if (openFileName.empty())
    {
        std::cout << "Please enter file name of image to add" << std::endl;
        std::cin >> openFileName;
    }

    // Create the oiio file handler for the image
    std::unique_ptr<ImageInput> openFile = ImageInput::open(openFileName);
    if (!openFile) {
        std::cerr << "Could not open image for " << openFileName << ", error = " << geterror() << std::endl;
        openFileName = "";  // Don't need file name anymore, reset now in case of error
        return false;
    }

    // Get image spec data from opened image file and use spec 
    // to allocate correct space for reading the pixel data.
    const ImageSpec& spec = openFile->spec();
    unsigned char* readPixmap = new unsigned char[spec.width * spec.height * spec.nchannels];

    // Override currentImgData with new data from read_image.
    if (!openFile->read_image(TypeDesc::UINT8, readPixmap))
    {
        std::cerr << "Could not read data from " << openFileName << ", error = " << geterror() << std::endl;
        openFileName = "";
        return false;
    }

    // Read successful, copy into layer.
    if (writeToLayer->rawImageData) PixelRGBA::DeletePixmap(writeToLayer->rawImageData);
    PixelRGBA::ContiguousDataToPixmap(writeToLayer->rawImageData, readPixmap, spec.width, spec.height, spec.nchannels);
    writeToLayer->warpedImageData = PixelRGBA::CopyPixmap(writeToLayer->rawImageData, spec.height, spec.width);
    writeToLayer->rasterPosX = 0;
    writeToLayer->rasterPosY = 0;
    writeToLayer->imageWidth = spec.width;
    writeToLayer->imageHeight = spec.height;

    // Close file. Don't need to manually destroy it due to nature of unique_ptrs.
    openFile->close();
    delete[] readPixmap;
    return true;
}

/*
 *  Writes the currently displayed pixels in the window to an outptut file name.
 *  User will be prompted for file name to write to. Output image SHOULD contain
 *  alpha channel data.
 */
void ProjectiveWarper::WriteImage()
{
    std::string outFileName;
    std::cout << "\nPlease enter name of output file (must have valid image file type)" << std::endl;

    std::cin >> outFileName;

    // Pixmap vars and window width/height.
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    unsigned char* pixmap = new unsigned char[4 * windowWidth * windowHeight];

    // create the oiio file handler for the image
    std::unique_ptr<ImageOutput> outfile = ImageOutput::create(outFileName);
    if (!outfile) {
        std::cerr << "Could not create output image for " << outFileName << ", error = " << geterror() << std::endl;
        return;
    }

    // Open a file for writing the image. The file header will indicate an image of
    // width w, height h, and 4 channels per pixel (RGBA). All channels will be of
    // type unsigned char
    ImageSpec spec(windowWidth, windowHeight, 4, TypeDesc::UINT8);
    if (!outfile->open(outFileName, spec)) {
        while (spec.nchannels > 0)
        {
            spec.nchannels--;
            if (!outfile->open(outFileName, spec))
            {
                std::cerr << "Could not open " << outFileName << ", error = " << geterror() << std::endl;
                return;
            }
            else
                break;
        }
    }

    // Get the current pixels from the OpenGL framebuffer and store in pixmap
    // Also determine proper format for glReadPixels to use based on number of channels!
    auto format = (spec.nchannels >= 4) ? GL_RGBA : (spec.nchannels == 3) ? GL_RGB : (spec.nchannels == 2) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
    glReadPixels(0, 0, w, h, format, GL_UNSIGNED_BYTE, pixmap);

    // Write the image to the file. All channel values in the pixmap are taken to be
    // unsigned chars. Flip image as well due to OpenGL storing pixmaps differently.
    int sclineLength = windowWidth * spec.nchannels * sizeof(unsigned char);
    if (!outfile->write_image(TypeDesc::UINT8, pixmap + (windowHeight - 1) * sclineLength, AutoStride, -sclineLength))
    {
        std::cerr << "Could not write image to " << outFileName << ", error = " << geterror() << std::endl;
        return;
    }
    else
        std::cout << "Image is stored" << std::endl;

    // close the image file after the image is written
    if (!outfile->close())
    {
        std::cerr << "Could not close " << outFileName << ", error = " << geterror() << std::endl;
        return;
    }

    delete[] pixmap;
}

/*
 *  Adds a new layer at the end of our list of layers. Reads image file
 *  specified by user as well via ReadImageFile();
 */
bool ProjectiveWarper::AddLayer()
{
    // Don't let them add more than max layers.
    if (layers.size() == MAX_LAYERS)
    {
        std::cout << "Cannot add more than " << MAX_LAYERS <<" layers\n";
        return false;
    }

    std::unique_ptr<Layer> newLayer = std::make_unique<Layer>();
    if (ReadImageFile(newLayer.get()))
    {
        // Sets warped image data to a warp using identity matrix, so the output
        // appears the same as the input pixmap when first created.
        Matrix3D temp = Matrix3D::Identity();
        newLayer->InvWarpLayer(temp);

        layers.push_back(std::move(newLayer));
        activeLayer = (int)layers.size() - 1;
        layerBoundPointsDirty = true;
        std::cout << "Layer " << activeLayer << " added and selected!\n\n";
        return true;
    }
    return false;
}

/*
 *  Deletes a layer with the specified index, and shifts all layers
 *  of higher indexes down one layer!
 */
bool ProjectiveWarper::DeleteLayer(const int& layer)
{
    const int& size = (int)layers.size();
    if (layer < 0 || layer > size) return false;

    for (int i = layer; i < size - 1; i++)
        layers[layer].swap(layers[layer + 1]);

    layers[size - 1].release();

    layers.resize(size - 1);
    std::cout << "\nLayer " << activeLayer << " deleted!\n";
    activeLayer = (layers.size() <= 0) ? (-1) : activeLayer % (int)layers.size();
    if (activeLayer >= 0)
        std::cout << "\nLayer " << activeLayer << " is now selected!\n";
    layerBoundPointsDirty = true;
    glutPostRedisplay();
    return true;
}

bool ProjectiveWarper::ResetLayer(const int& layer)
{
    if (layer < 0 || layer >= layers.size())
    {
        std::cout << "Invalid layer to delete\n";
        return false;
    }

    Matrix3D identity = Matrix3D::Identity();
    Layer* resetLayer = layers[layer].get();
    resetLayer->InvWarpLayer(identity);
    resetLayer->rasterPosX = resetLayer->rasterPosY = 0;
    MapSelectedLayerPoints();
    return true;
}

/*
 *  Renders a given layer (if the index exists) based on it's stored raster position
 */
void ProjectiveWarper::RenderLayer(const Layer* rendLayer)
{
    // To anyone online who sees this who might care, yes, I know glDrawPixels is deprecated
    // and slow. I do not care in this case. OpenGL was not the focus the project; this was made
    // for the purpose of improving my ability to architecture larger programs well and learn
    // how projective warping works on a conceptual level (matrices and all)
    glRasterPos2d(0, 0);
    glBitmap(0, 0, 0, 0, (float)rendLayer->rasterPosX, (float)rendLayer->rasterPosY, NULL);
    glDrawPixels(rendLayer->outputWidth, rendLayer->outputHeight, GL_RGBA, GL_UNSIGNED_BYTE, rendLayer->warpedImageData[0]);
}

/*
 *  Using the currently set positions of the active layer's forward mapped points
 *  (either from directly forward mapping or recently moving one with the mouse),
 *  create a warp matrix based on the known uv-coordinates and xy-coordinates.
 *  
 *  Formula I used to simplify these calculations for each individual matrix coefficient
 *  can be found here: http://graphics.cs.cmu.edu/courses/15-463/2008_fall/Papers/proj.pdf
 *  
 *  I tried using the fast method on page 4, but it wasn't working for some reason no matter
 *  how hard I tried. Therefore I do the traditional 8x8 system with gaussian elimination on pg. 3
 */
void ProjectiveWarper::ProjectiveWarpLayer(Layer* warpLayer)
{
    Matrix3D newM = Matrix3D::Identity();
    Matrix8D projectiveSystem = Matrix8D::Zero();
    Vector8D destPoints;
    Vector8D solution;
    Vector8D srcPoints;
    srcPoints << 0, warpLayer->imageWidth, warpLayer->imageWidth, 0, 0, 0, warpLayer->imageHeight, warpLayer->imageHeight;
    
    // Caching these since they're called in below loop, should be slightly faster.
    const int& imgRasterPosX = warpLayer->rasterPosX;
    const int& imgRasterPosY = warpLayer->rasterPosY;

    // Populate 8x8 matrix for system to solve. Also populate a vector
    // with destination points (where they were moved to by the mouse) by removing
    // the raster position.
    for (int i = 0; i < 4; i++)
    {
        // Desintation points, first 4 elements are x-positions, last 4 are y-positions.
        destPoints(i, 0) = activeLayerBoundPoints[i].x - imgRasterPosX;
        destPoints(i + 4, 0) = activeLayerBoundPoints[i].y - imgRasterPosY;
    
        // 8x8 matrix to solve. Any points not set here are implicitly zero.
        projectiveSystem(i, 0) = projectiveSystem(i + 4, 3) = srcPoints(i, 0);
        projectiveSystem(i, 1) = projectiveSystem(i + 4, 4) = srcPoints(i + 4, 0);
        projectiveSystem(i, 2) = projectiveSystem(i + 4, 5) = 1;
        
        projectiveSystem(i, 6)      = -srcPoints(i, 0) * destPoints(i, 0);
        projectiveSystem(i, 7)      = -srcPoints(i + 4, 0) * destPoints(i, 0);
        projectiveSystem(i + 4, 6)  = -srcPoints(i, 0) * destPoints(i + 4, 0);
        projectiveSystem(i + 4, 7)  = -srcPoints(i + 4, 0) * destPoints(i + 4, 0);
    }

    // Solve and explicity set 3D matrix version of solution; do so for
    // efficiency by avoiding for loops.
    solution = projectiveSystem.partialPivLu().solve(destPoints);
    newM(0, 0) = solution(0, 0);
    newM(0, 1) = solution(1, 0);
    newM(0, 2) = solution(2, 0);
    newM(1, 0) = solution(3, 0);
    newM(1, 1) = solution(4, 0);
    newM(1, 2) = solution(5, 0);
    newM(2, 0) = solution(6, 0);
    newM(2, 1) = solution(7, 0);
    newM(2, 2) = 1.0;

    warpLayer->InvWarpLayer(newM);

    // Move center icon to proper position after warping.
    Vector3D srcCenter, imgPoint;
    srcCenter << warpLayer->imageWidth / 2, warpLayer->imageHeight / 2, 1.0;

    imgPoint = newM * srcCenter;
    imgPoint(0, 0) = imgPoint(0, 0) / imgPoint(2, 0);
    imgPoint(1, 0) = imgPoint(1, 0) / imgPoint(2, 0);
    activeLayerBoundPoints[4].x = imgPoint(0, 0) + warpLayer->rasterPosX;
    activeLayerBoundPoints[4].y = imgPoint(1, 0) + warpLayer->rasterPosY;
}

/*
 *  Maps the tracked active layer points properly with where the active layer
 *  is currently positioned and how the output pixmap is forward mapped.
 */
void ProjectiveWarper::MapSelectedLayerPoints()
{
    Layer* selectedLayer = layers[activeLayer].get();
    const float& inWidth = (float)selectedLayer->imageWidth;
    const float& inHeight = (float)selectedLayer->imageHeight;

    // Get forward mapped points of corners and image center
    Vector3D imgPoints[5];
    Vector3D srcPoints[5];
    srcPoints[0] << 0.0, 0.0, 1.0;
    srcPoints[1] << inWidth, 0.0, 1.0;
    srcPoints[2] << inWidth, inHeight, 1.0;
    srcPoints[3] << 0.0, inHeight, 1.0;
    srcPoints[4] << inWidth / 2, inHeight / 2, 1.0;

    const Matrix3D& mTemp = selectedLayer->warpMatrix;
    imgPoints[0] = mTemp * srcPoints[0]; // Lower left
    imgPoints[1] = mTemp * srcPoints[1]; // Lower right
    imgPoints[2] = mTemp * srcPoints[2]; // Upper right
    imgPoints[3] = mTemp * srcPoints[3]; // Upper left
    imgPoints[4] = mTemp * srcPoints[4]; // Middle

    // Normalize and set calculated values to proper points.
    for (int i = 0; i < 5; i++)
    {
        imgPoints[i](0, 0) = imgPoints[i](0, 0) / imgPoints[i](2, 0);
        imgPoints[i](1, 0) = imgPoints[i](1, 0) / imgPoints[i](2, 0);
        activeLayerBoundPoints[i].x = imgPoints[i](0, 0) + selectedLayer->rasterPosX;
        activeLayerBoundPoints[i].y = imgPoints[i](1, 0) + selectedLayer->rasterPosY;
    }

    layerBoundPointsDirty = false;
}

void ProjectiveWarper::ResetMouseStates()
{
    leftMousePressedLastFrame = false;
    mouseMovementPointIndex = -1;
    mouseMoveX = 0;
    mouseMoveY = 0;
    lastMouseX = INT_MIN;
    lastMouseY = INT_MIN;
}

/*
 *  Handles displaying all layers or necessary graphics to the screen.
 *  Use this in the glut render callback!
 */
void ProjectiveWarper::DisplayLayers()
{
    glClearColor(0.05, 0.05, 0.05, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render layers first.
    for (int i = 0; i < (int)layers.size(); i++)
        RenderLayer(layers[i].get());    

    if (saveWindowThisFrame)
    {
        WriteImage();
        saveWindowThisFrame = false;
    }

    // Render icon points to proper forward mapped location on the screen.
    else if (!layers.empty())
    {
        if (layerBoundPointsDirty)
            MapSelectedLayerPoints();

        // Render corners
        for (int i = 0; i < 4; i++)
        {
            cornerIcon->rasterPosX = (int)activeLayerBoundPoints[i].x - (cornerIcon->outputWidth / 2);
            cornerIcon->rasterPosY = (int)activeLayerBoundPoints[i].y - (cornerIcon->outputHeight / 2);
            RenderLayer(cornerIcon.get());
        }

        // Render center
        centerIcon->rasterPosX = (int)activeLayerBoundPoints[4].x - (centerIcon->outputWidth / 2);
        centerIcon->rasterPosY = (int)activeLayerBoundPoints[4].y - (centerIcon->outputHeight / 2);
        RenderLayer(centerIcon.get());
    }

    glFlush();
}

/*
 *  Handles what occurs upon the display window getting reshaped.
 *	Use this in the glut reshape callback!
 */
void ProjectiveWarper::HandleWindowReshape(int newWidth, int newHeight)
{
    // Now store new window sizes.
    windowWidth = newWidth;
    windowHeight = newHeight;

    // Set the viewport to be the entire window
    glViewport(0, 0, windowWidth, windowHeight);

    // Define the drawing coordinate system on the viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
}

/*
 *  Handle normal key presses and call appropriate functions.
 */
void ProjectiveWarper::HandleKeyPresses(unsigned char key, int x, int y)
{
    switch (key)
    {
        // Create new layer
        case 'N':
        case 'n':
            AddLayer();
            break;
        // Delete on backspace or delete pressed.
        case 127:
        case 8:
            DeleteLayer(activeLayer);
            break;
        // Save current window display to an image.
        case 'S':
        case 's':
            saveWindowThisFrame = true;
            break;
        // Reset current layer to origin and identity matrix.
        case 'R':
        case 'r':
            ResetLayer(activeLayer);
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

/*
 *  Handle special keys that aren't included in normal key press func. like arrow keys. 
 */
void ProjectiveWarper::HandleSpecialKeyPresses(int key, int x, int y)
{
    int newLayer;
    switch (key)
    {
        // Select layer above or below.
        case GLUT_KEY_UP:
            activeLayer = (activeLayer + 1) % (int)layers.size();
            layerBoundPointsDirty = true;
            std::cout << "Selected layer: " << activeLayer << std::endl;
            break;
        case GLUT_KEY_DOWN:
            activeLayer = (activeLayer - 1 <= -1) ? ((int)layers.size() - 1) : (activeLayer - 1);
            layerBoundPointsDirty = true;
            std::cout << "Selected layer: " << activeLayer << std::endl;
            break;

        // Swap layer with one above or below it.
        case GLUT_KEY_RIGHT:
            newLayer = (activeLayer + 1) % (int)layers.size();
            layers[activeLayer].swap(layers[newLayer]);
            std::cout << "Swapping layer " << activeLayer << " with layer " << newLayer << std::endl;
            activeLayer = newLayer;
            break;
        case GLUT_KEY_LEFT:
            newLayer = (activeLayer - 1 <= -1) ? ((int)layers.size() - 1) : (activeLayer - 1);
            layers[activeLayer].swap(layers[newLayer]);
            std::cout << "Swapping layer " << activeLayer << " with layer " << newLayer << std::endl;
            activeLayer = newLayer;
            break;
    }
    glutPostRedisplay();
}

/*
 *  Purely handles determining if the left mouse was just clicked this frame.
 *  If it was, search for an active layer's corner that the mouse may be in range of.
 *  It it wasn't, reset which point will be affected by mouse movement.
 */
void ProjectiveWarper::HandleMouseDownEvent(int button, int state, int x, int y)
{
    y = windowHeight - y;

    // Mouse released, reset mouse movement index.
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
    {
        ResetMouseStates();
        return;
    }
    if (button != GLUT_LEFT_BUTTON || leftMousePressedLastFrame) return;

    leftMousePressedLastFrame = true;
    //std::cout << "X: " << x << " Y: " << y << std::endl;

    // We know the button is the left one and is pressed down. If this didn't happen
    // last frame (so it just began occurring), search for corner to allow mouse movement to warp.
    if (!layers.empty())
    {
        // Find first corner where mouse position is within range.
        double validDistSqr = warpIconRange * warpIconRange;
        for (int i = 0; i < 4; i++)
        {
            const Point& layerPt = activeLayerBoundPoints[i];
            double sqrMouseDistFromPoint = ((layerPt.x - x) * (layerPt.x - x))
                + ((layerPt.y - y) * (layerPt.y - y));

            // Within range, select this point.
            if (validDistSqr > sqrMouseDistFromPoint)
            {
                std::cout << "Moving Point: " << i << std::endl;
                mouseMovementPointIndex = i;
                break;
            }
            
            // Move entire image otherwise.
            mouseMovementPointIndex = 4;
        }
    }

}
/*
 *  Called by glut when mouse movment is detected while any mouse button IS clicked.
 *  Here we track how much the mouse moved between calls and use that for actually
 *  manipulating the image.
 */
void ProjectiveWarper::HandleClickedMouseMotion(int x, int y)
{
    if (!leftMousePressedLastFrame) return;
    mouseMoveX = (lastMouseX == INT_MIN) ? 0 : (x - lastMouseX);
    mouseMoveY = (lastMouseY == INT_MIN) ? 0 : (lastMouseY - y);    //GL coordinates are weird

    lastMouseX = x;
    lastMouseY = y;

    //std::cout << mouseMoveX << ", " << mouseMoveY << std::endl;

    if (mouseMoveX == 0 && mouseMoveY == 0) return;

    if (mouseMovementPointIndex == 4)
    {
        layers[activeLayer]->rasterPosX += mouseMoveX;
        layers[activeLayer]->rasterPosY += mouseMoveY;
        for (auto& boundPoint : activeLayerBoundPoints)
        {
            boundPoint.x += mouseMoveX;
            boundPoint.y += mouseMoveY;
        }
        glutPostRedisplay();
    }

    // A point is being clicked by mouse, move this point and warp the image.
    else if (mouseMovementPointIndex >= 0 && mouseMovementPointIndex < 4)
    {
        activeLayerBoundPoints[mouseMovementPointIndex].x += mouseMoveX;
        activeLayerBoundPoints[mouseMovementPointIndex].y += mouseMoveY;

        ProjectiveWarpLayer(layers[activeLayer].get());
    }
    glutPostRedisplay();
}

/*
 *  Called by glut when mouse movement is detected while nothing is clicked.
 *  Here we reset the amount the mouse moved since it wasn't clicked
 */
void ProjectiveWarper::ResetLastMousePosition(int x, int y)
{
    ResetMouseStates();
}

int ProjectiveWarper::GetWindowWidth() { return windowWidth; }
int ProjectiveWarper::GetWindowHeight() { return windowHeight; }
