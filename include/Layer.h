#pragma once
#include <iostream>

#include "PixelRGBA.h"
#include "EigenMatrix.h"

struct Layer
{
	Matrix3D warpMatrix;
	PixelRGBA** rawImageData;
	PixelRGBA** warpedImageData;
	int rasterPosX, rasterPosY;
	int imageWidth, imageHeight;
	int outputWidth, outputHeight;

	Layer();
	~Layer();

	/*
	 *	Move this image's raster position by the amount of pixels specified.
	 */
	void MoveImage(int offsetX, int offsetY);

	/*
	 *	Warps the pixmap using inverse mapping and stores the output in warpedImageData.
	 *	Also correctly sets the warp matrix and output dimensions.
	 */
	void InvWarpLayer(const Matrix3D& M);
};