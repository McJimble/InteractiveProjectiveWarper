#include "PixelRGBA.h"

PixelRGBA** PixelRGBA::CreatePixmap(const int& rows, const int& cols, bool initValues)
{
    // Avoid some invalid array exceptions.
    if (rows <= 0 || cols <= 0) return nullptr;

    // Allocate pixmap in a way such that memory is contiguous and therefore
    // accessed much quicker than a normal 2d array.
    PixelRGBA** pixmap;
    pixmap = new PixelRGBA* [rows];
    pixmap[0] = new PixelRGBA[rows * cols]; 

    for (int i = 1; i < rows; i++)
        pixmap[i] = pixmap[i - 1] + cols;

    if (initValues)
    {
        for (int row = 0; row < rows; row++)
            for (int col = 0; col < cols; col++)
            {
                pixmap[row][col].r = 0;
                pixmap[row][col].g = 0;
                pixmap[row][col].b = 0;
                pixmap[row][col].a = 255;
            }
    }

    return pixmap;
}


PixelRGBA** PixelRGBA::CopyPixmap(PixelRGBA**& fromPixmap, const int& rows, const int& cols)
{
    PixelRGBA** newPixmap = PixelRGBA::CreatePixmap(rows, cols, false);

    for (int row = 0; row < rows; row++)
        for (int col = 0; col < cols; col++)
        {
            newPixmap[row][col].r = fromPixmap[row][col].r;
            newPixmap[row][col].g = fromPixmap[row][col].g;
            newPixmap[row][col].b = fromPixmap[row][col].b;
            newPixmap[row][col].a = fromPixmap[row][col].a;
        }

    return newPixmap;
}

void PixelRGBA::ContiguousDataToPixmap(PixelRGBA**& oldPixmap, unsigned char*& copyFromPixmap, 
        const int& width, const int& height, const int& channels)
{
    // Resize oldPixmap appropriately, then transfer new data into resized structure.
    oldPixmap = PixelRGBA::CreatePixmap(height, width, true);

    bool adjustAlpha = (channels > 3);
    int greyScaleAccount = (channels == 1) ? 0 : 1;

    // Don't know if this could be faster, but loop through each pixel
    // and read the next 4 elements to get each channel value.
    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
        {
            // Need to store image upside due OpenGL having different start position for scanlines.
            int twoDimConv = ((height - row - 1) * width + col);

            oldPixmap[row][col].r = copyFromPixmap[twoDimConv * channels];
            oldPixmap[row][col].g = copyFromPixmap[twoDimConv * channels + (1 * greyScaleAccount)];
            oldPixmap[row][col].b = copyFromPixmap[twoDimConv * channels + (2 * greyScaleAccount)];
            if (adjustAlpha)
                oldPixmap[row][col].a = copyFromPixmap[twoDimConv * channels + 3];
        }
}

void PixelRGBA::DeletePixmap(PixelRGBA**& pixmap)
{
    // The way we allocate the pixmap in CreatePixmap allows this.
    delete[] pixmap[0];
    delete[] pixmap;
}
