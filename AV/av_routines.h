// Author      : Stas Kalinin

#ifndef AV_ROUTINES_HPP
#define AV_ROUTINES_HPP

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)
#define _malloca malloc
#define _freea free
#endif

#include <math.h>
#include <malloc.h>
#include <xmmintrin.h>

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))

#include <vector>
#include <Eigen/Dense>

std::vector<Eigen::Vector3f> calculate3R(float L, float W, float R1, float R2,
					 float R3, unsigned atom_i, float dg,
					 float vdWRMax,				// v.d.Waals radii
					 float linkersphere, int linknodes,
					 const std::vector<Eigen::Vector4f> &xyzW);

enum GridFlags : unsigned char
{
	ClashLinker = 0x01,
	ClashDyeR1 = 0x02,
	ClashDyeR2 = 0x04,
	ClashDyeR3 = 0x08,
	ClashAny = 0x0F,
	NewPositionEvenIter = 0x10,
	NewPositionOddIter = 0x20
};

int calculate1R(double L, double W, double R, int atom_i, double dg,// linker and grid parameters
		double* XLocal, double* YLocal, double* ZLocal,	// atom coordinates
		double* vdWR, int NAtoms, double vdWRMax,// v.d.Waals radii
		double linkersphere, int linknodes,// linker routing parameters
		unsigned char* density);
int calculate3R(double L, double W, double R1, double R2, double R3, int atom_i, double dg,
	double* XLocal, double* YLocal, double* ZLocal, 		// atom coordinates
	double* vdWR, int NAtoms, double vdWRMax,				// v.d.Waals radii
	double linkersphere, int linknodes, 					// linker routing parameters
	unsigned char* density);

#endif // AV_ROUTINES_HPP
