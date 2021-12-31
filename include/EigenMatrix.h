#pragma once
#include <Eigen/Dense>

/*
 *	The entire point of this header is to condense usage of
 *	the eigen library; makes some typedefs for matrices of sizes
 *  we'll need in our calculations.
 */

typedef Eigen::Matrix<float, 3, 3> Matrix3D;
typedef Eigen::Matrix<float, 8, 8> Matrix8D;

typedef Eigen::Matrix<float, 3, 1> Vector3D;
typedef Eigen::Matrix<float, 8, 1> Vector8D;
