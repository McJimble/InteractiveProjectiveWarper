#pragma once
#include "EigenMatrix.h"

struct Point
{
	double x;
	double y;

	Point() { x = 0; y = 0; }

	Point(double x, double y)
	{
		this->x = x;
		this->y = y;
	}
};