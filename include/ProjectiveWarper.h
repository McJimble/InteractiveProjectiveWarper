#pragma once

#include <OpenImageIO/imageio.h>
#include <GL/glut.h>

#include <string>
#include <vector>
#include <iostream>
#include <memory>

#include "Layer.h"
#include "Point.h"

OIIO_NAMESPACE_USING
#define PROGRAM_NAME "Projective Warper"

class ProjectiveWarper
{
private:

	static const int MAX_LAYERS;

	int windowHeight, windowWidth;				// Size of window currently
	int mouseMoveX, mouseMoveY;					// Last detected mouse movement while clicked
	int lastMouseX, lastMouseY;					// Valid mouse position detected previous frame.
	int activeLayer;							// Which layer is selected
	std::vector<std::unique_ptr<Layer>> layers;

	// These render on the current layer
	std::unique_ptr<Layer> cornerIcon;
	std::unique_ptr<Layer> centerIcon;
	float warpIconRange;				// <- Distance of mouse click from icon
										// for warp to be valid.

	/*
	 *	For speed and simplicity, laying out all the points that need to have an
	 *  icon rendered on them here. 
	 *	0 = Forward mapped lower left
	 *	1 = Forward mapped lower right
	 *  2 = Forward mapped upper right
	 *  3 = Forward mapped upper left
	 *  4 = Forward mapped center
	 */
	Point activeLayerBoundPoints[5];
	int mouseMovementPointIndex = -1;
	bool layerBoundPointsDirty = true;
	bool leftMousePressedLastFrame = false;
	bool saveWindowThisFrame = false;

public:

	ProjectiveWarper();
	~ProjectiveWarper();
	
	// Img. handling and layer stuff
	bool ReadImageFile(Layer* writeToLayer, std::string openFileName = "");
	void WriteImage();
	bool AddLayer();
	bool DeleteLayer(const int& layer);
	bool ResetLayer(const int& layer);
	void RenderLayer(const Layer* rendLayer);
	void ProjectiveWarpLayer(Layer* warpLayer);
	void MapSelectedLayerPoints();
	void ResetMouseStates();

	// Glut callback funcs. Call these in some other function that isn't
	// tied to a class and can actually be used by glut!
	void DisplayLayers();
	void HandleWindowReshape(int newWidth, int newHeight);
	void HandleKeyPresses(unsigned char key, int x, int y);
	void HandleSpecialKeyPresses(int key, int x, int y);
	void HandleMouseDownEvent(int button, int state, int x, int y);
	void HandleClickedMouseMotion(int x, int y);
	void ResetLastMousePosition(int x, int y);

	int GetWindowWidth();
	int GetWindowHeight();
};
