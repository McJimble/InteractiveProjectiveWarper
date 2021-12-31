#include "Layer.h"

Layer::Layer()
{
    rawImageData = warpedImageData = nullptr;
    imageWidth = 0;
    imageHeight = 0;
    rasterPosX = 0;
    rasterPosY = 0;
    warpMatrix = Matrix3D::Identity();
}

Layer::~Layer()
{
    if (rawImageData) PixelRGBA::DeletePixmap(rawImageData);
    if (warpedImageData) PixelRGBA::DeletePixmap(warpedImageData);
}

void Layer::MoveImage(int offsetX, int offsetY)
{
	rasterPosX += offsetX;
	rasterPosY += offsetY;
}

void Layer::InvWarpLayer(const Matrix3D& M)
{
    // First compute bounding box required for output pixmap, using the difference
    // between the forward-mapped min and max values to find the width and height.
    Vector3D forwardMappedCorners[4];
    Vector3D srcPoints[4];
    srcPoints[0] << 0.0f, 0.0f, 1.0f;
    srcPoints[1] << (float)imageWidth, 0.0f, 1.0f;
    srcPoints[2] << (float)imageWidth, (float)imageHeight, 1.0f;
    srcPoints[3] << 0.0f, (float)imageHeight, 1.0f;

    forwardMappedCorners[0] = M * srcPoints[0];         // Lower left
    forwardMappedCorners[1] = M * srcPoints[1];         // Lower right
    forwardMappedCorners[2] = M * srcPoints[2];         // Upper right
    forwardMappedCorners[3] = M * srcPoints[3];         // Upper left

    // Also normalize these values!
    for (int i = 0; i < 4; i++)
    {
        forwardMappedCorners[i](0, 0) = forwardMappedCorners[i](0, 0) / forwardMappedCorners[i](2, 0);
        forwardMappedCorners[i](1, 0) = forwardMappedCorners[i](1, 0) / forwardMappedCorners[i](2, 0);
    }

    // Need minX, minY, maxX, and maxY values resulting from these corners
    // Once done, use them to get output image width/height.
    double minX, minY, maxX, maxY;
    minX = minY = DBL_MAX;
    maxX = maxY = DBL_MIN;
    for (int i = 0; i < 4; i++)
    {
        minX = (forwardMappedCorners[i](0, 0) < minX) ? forwardMappedCorners[i](0, 0) : minX;
        maxX = (forwardMappedCorners[i](0, 0) > maxX) ? forwardMappedCorners[i](0, 0) : maxX;
        minY = (forwardMappedCorners[i](1, 0) < minY) ? forwardMappedCorners[i](1, 0) : minY;
        maxY = (forwardMappedCorners[i](1, 0) > maxY) ? forwardMappedCorners[i](1, 0) : maxY;
    }
    int outWidth = (int)(maxX - minX);     // Width  = right - left   (of bounding box)
    int outHeight = (int)(maxY - minY);    // Height = top   - bottom (of bounding box)
    
    // Get inverse matrix; also shift raster position based on minimum x/y values
    // so the transformed image stays within the bounds of the allocated pixmap
    Matrix3D invM = M.inverse();
    rasterPosX += (int)(minX + rasterPosX) - rasterPosX;
    rasterPosY += (int)(minY + rasterPosY) - rasterPosY;

    // Allocate output pixmap and begin computing each necessary pixel via inverse mapping.
    if (warpedImageData[0] != nullptr) PixelRGBA::DeletePixmap(warpedImageData);
    warpedImageData = PixelRGBA::CreatePixmap(outHeight, outWidth, false);

    outputWidth = outWidth;
    outputHeight = outHeight;

    // Inverse map for each output pixel
    for (int y = 0; y < outHeight; y++)
        for (int x = 0; x < outWidth; x++)
        {
            // Mapped pixel coordinates
            Vector3D pixel_out;
            pixel_out << (float)x, (float)y, 1.0f;

            Vector3D pixel_in = invM * pixel_out;

            // Normalize pixmap; dividing u' and v' by the w' component
            double u = pixel_in(0, 0) / pixel_in(2, 0);
            double v = pixel_in(1, 0) / pixel_in(2, 0);

            // Get proper inversely mapped pixel from input map. If not in bounds,
            // set it to a clear pixel.
            PixelRGBA inPixel;
            if (std::lround(u) < 0 || std::lround(v) < 0 || std::lround(u) >= imageWidth || std::lround(v) >= imageHeight)
                inPixel.r = inPixel.g = inPixel.b = inPixel.a = 0;
            else
                inPixel = rawImageData[std::lround(v)][std::lround(u)];

            warpedImageData[y][x] = inPixel;
        }

    warpMatrix = M;
}