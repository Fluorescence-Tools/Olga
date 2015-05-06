#include "av_routines.h"
#include <iostream>

std::vector<Eigen::Vector3f> calculate3R(float L, float W, float R1, float R2, float R3, unsigned atom_i, float dg,
					 float vdWRMax,				// v.d.Waals radii
					 float linkersphere, int linknodes, 					// linker routing parameters
					 const std::vector<Eigen::Vector4f> &xyzW)									// returns density array
{

	unsigned NAtoms=xyzW.size();
	// grid
	float Rmax = __max(R1, R2); Rmax = __max(Rmax, R3);
	float x0 = xyzW[atom_i][0], y0 = xyzW[atom_i][1], z0 = xyzW[atom_i][2];
	int npm = static_cast<int>(floor(L / dg));
	int ng = 2 * npm + 1, n;
	int ng3 = ng * ng * ng;
	float* grid = new float[ng] + npm;
	__m128* dgridsq =  static_cast<__m128*>((_malloca(ng * 16))) + npm;
	for (int i = -npm; i <= npm; i++)
	{
		grid[i] = i * dg;
		dgridsq[i] = _mm_set_ps1(static_cast<float>((2 * i + 1)*dg*dg));
	}

	// select atoms potentially within reach, excluding the attachment point
	float rmaxsq = (L + Rmax + vdWRMax) * (L + Rmax + vdWRMax), rmax, r, rsq, dx, dy, dz;
	int* atomindex = new int[NAtoms];
	int natomsgrid;
	n = 0;
	for (unsigned i = 0; i < NAtoms; i++)
	{
		dx = xyzW[i][0] - x0; dy = xyzW[i][1] - y0; dz = xyzW[i][2] - z0;
		rsq = dx * dx + dy * dy + dz * dz;
		if ((rsq < rmaxsq) && (i != atom_i)) atomindex[n++] = i;
	}
	natomsgrid = n;

	// local coordinates
	float* xa = new float[natomsgrid];
	float* ya = new float[natomsgrid];
	float* za = new float[natomsgrid];
	float* vdWr = new float[natomsgrid];
	for (int i = 0; i < natomsgrid; i++)
	{
		n = atomindex[i]; vdWr[i] = xyzW[n][3];
		xa[i] = xyzW[n][0] - x0; ya[i] = xyzW[n][1] - y0; za[i] = xyzW[n][2] - z0;
	}

	// search for allowed positions
	unsigned char* clash = new unsigned char[ng3];
	for (int i = 0; i < ng3; i++) clash[i] = 0;

	// search for positions causing clashes with atoms
	int ix2, iy2, ir2, rmaxsqint;
	float dx2, dy2, rmaxsq_dye1, rmaxsq_dye2, rmaxsq_dye3, rmaxsq_linker;
	int ixmin, ixmax, iymin, iymax, izmin, izmax, offset;
	for (int i = 0; i < natomsgrid; i++)
	{
		rmaxsq_dye1 = (vdWr[i] + R1) * (vdWr[i] + R1);
		rmaxsq_dye2 = (vdWr[i] + R2) * (vdWr[i] + R2);
		rmaxsq_dye3 = (vdWr[i] + R3) * (vdWr[i] + R3);
		rmaxsq_linker = (vdWr[i] + 0.5 * W) * (vdWr[i] + 0.5 * W);
		rmax = vdWr[i] + __max(Rmax, 0.5 * W);
		rmaxsq = rmax * rmax;
		__m128 rmaxsq_dye_linker = _mm_set_ps(static_cast<float>(rmaxsq_dye3),
						      static_cast<float>(rmaxsq_dye2),
						      static_cast<float>(rmaxsq_dye1),
						      static_cast<float>(rmaxsq_linker));
		ixmin = __max(static_cast<int>(ceil((xa[i] - rmax) / dg)), -npm);
		ixmax = __min(static_cast<int>(floor((xa[i] + rmax) / dg)), npm);
		izmin = __max(static_cast<int>(ceil((za[i] - rmax) / dg)), -npm);
		izmax = __min(static_cast<int>(floor((za[i] + rmax) / dg)), npm);

		float za_i = za[i];
		__m128 dz2 = _mm_set_ps1(static_cast<float>(-2.*za_i*dg));
		for (int ix = ixmin; ix <= ixmax; ix++)
		{
			dx2 = (grid[ix] - xa[i]) * (grid[ix] - xa[i]);
			dy = sqrt(__max(rmaxsq - dx2, 0.));
			iymin = __max(static_cast<int>(ceil((ya[i] - dy) / dg)), -npm);
			iymax = __min(static_cast<int>(floor((ya[i] + dy) / dg)), npm);
			offset = ng * (ng * (ix + npm) + iymin + npm) + npm;
			for (int iy = iymin; iy <= iymax; iy++)
			{
				dy2 = (grid[iy] - ya[i]) * (grid[iy] - ya[i]);
				float r12 = static_cast<float>(dx2 + dy2 + (za_i - grid[izmin])*(za_i - grid[izmin]));
				__m128 dxy2 = _mm_sub_ps(_mm_set_ps1(r12), rmaxsq_dye_linker);
				for (int iz = izmin; iz <= izmax; iz++)
				{
					__m128 tmp;
					clash[iz + offset] |= _mm_movemask_ps(dxy2);
					tmp = _mm_add_ps(dgridsq[iz], dz2);
					dxy2 = _mm_add_ps(dxy2, tmp);
				}
				offset += ng;
			}
		}
	}

	// route linker as a flexible pipe
	float* rlink = new float[ng3];
	float rlink0;
	int ix0, iy0, iz0, linknodes_eff, dlz = 2 * linknodes + 1;
	int nnew = 0;
	int* newpos = new int[ng3];

	for (int i = 0; i < ng3; i++) rlink[i] = (clash[i] & GridFlags::ClashLinker) ? -L : L + L;

	// (1) all positions within linkerinitialsphere*W from the attachment point are allowed
	rmaxsqint = static_cast<int>(floor(linkersphere * linkersphere * W * W / dg / dg));
	ixmax = __min(static_cast<int>(floor(linkersphere * W / dg)), npm);
	n = 0;
	for (int ix = -ixmax; ix <= ixmax; ix++)
	{
		ix2 = ix * ix;
		offset = ng * (ng * (ix + npm) - ixmax + npm) + npm;
		for (int iy = -ixmax; iy <= ixmax; iy++)
		{
			iy2 = iy * iy;
			for (int iz = -ixmax; iz <= ixmax; iz++)
				if (ix2 + iy2 + iz * iz <= rmaxsqint)
				{
					rlink[iz + offset] = sqrt(static_cast<float>((ix2 + iy2 + iz * iz)) * dg);
					newpos[nnew++] = ng * (ng * (npm + ix) + iy + npm) + npm + iz;
				}
			offset += ng;
		}
	}

	// (2) propagate from new positions
	float* sqrts_dg = static_cast<float *>(_malloca((2 * linknodes*linknodes + 1) * (2 * linknodes + 1) * 4));
	for (int ix = 0; ix <= linknodes; ix++)
		for (int iy = 0; iy <= linknodes; iy++)
			for (int iz = -linknodes; iz <= linknodes; iz++)
				sqrts_dg[(ix*ix + iy*iy)*dlz + iz + linknodes] = sqrt(static_cast<float>((ix*ix + iy*iy + iz*iz)) * dg);

	while (nnew > 0)
	{
		for (n = 0; n < nnew; n++)
		{
			rlink0 = rlink[newpos[n]];
			linknodes_eff = __min(linknodes, static_cast<int>(floor((L - rlink0) / dg)));
			ix0 = newpos[n] / (ng*ng);
			iy0 = newpos[n] / ng - ix0*ng;
			iz0 = newpos[n] - ix0*ng*ng - iy0*ng;
			ixmin = __max(-linknodes_eff, -ix0);
			ixmax = __min(linknodes_eff, 2 * npm - ix0);
			iymin = __max(-linknodes_eff, -iy0);
			iymax = __min(linknodes_eff, 2 * npm - iy0);
			izmin = __max(-linknodes_eff, -iz0);
			izmax = __min(linknodes_eff, 2 * npm - iz0);

			for (int ix = ixmin; ix <= ixmax; ix++)
			{
				offset = newpos[n] + ng * (ng * ix + iymin);
				ix2 = ix * ix;
				for (int iy = iymin; iy <= iymax; iy++)
				{
					ir2 = (ix2 + iy*iy) * dlz + linknodes;
					for (int iz = izmin; iz <= izmax; iz++)
					{
						r = sqrts_dg[ir2 + iz] + rlink0;
						if ((rlink[iz + offset] > r) && (r < L))
						{
							rlink[iz + offset] = r;
							clash[iz + offset] |= GridFlags::NewPositionEvenIter;
						}
					}
					offset += ng;
				}
			}
		}

		// update "new" positions
		nnew = 0;
		for (int i = 0; i < ng3; i++)
		{
			if (clash[i] & GridFlags::NewPositionEvenIter) newpos[nnew++] = i;
			clash[i] &= GridFlags::ClashAny;
		}
	}

	std::vector<Eigen::Vector3f> points;
	n = 0; int dn = 0;
	int ng2=ng*ng;
	for (int ix = -npm; ix <= npm; ix++)
	{
		offset = ng2 * (ix + npm) + npm;
		for (int iy = -npm; iy <= npm; iy++)
		{
			for (int iz = -npm; iz <= npm; iz++)
			{
				int i=iz + offset;
				if ((clash[i] & GridFlags::ClashLinker) || rlink[i] > L) continue;
				dn = ((~clash[i] & GridFlags::ClashDyeR3) >> 3) + ((~clash[i] & GridFlags::ClashDyeR2) >> 2) + ((~clash[i] & GridFlags::ClashDyeR1) >> 1);
				n += dn;
				if (dn > 0)
				{
					Eigen::Vector3f p;
					p[0] = ix*dg + x0;
					p[1] = iy*dg + y0;
					p[2] = iz*dg + z0;
					points.push_back(p);
				}
			}
			offset += ng;
		}
	}
	grid -= npm; dgridsq -= npm;
	delete[] grid; delete[] atomindex;
	delete[] xa; delete[] ya; delete[] za; delete[] vdWr;
	delete[] clash; delete[] rlink; delete[] newpos;
	_freea(sqrts_dg); _freea(dgridsq);

	return points;
}

