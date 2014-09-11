#include "avRoutines.h"

#include <algorithm>

int movemask(const Eigen::Vector4f &a)
{
    using std::signbit;
    return signbit(a[3])<<3 | signbit(a[2])<<2 | signbit(a[1])<<1 | signbit(a[0]);
}

std::vector<Eigen::Vector3f> calculate3R(float L, float W, float R1, float R2, float R3, unsigned atom_i,
                                         float dg, const std::vector<Eigen::Vector4f> &coordsVdW, float linkersphere, int linknodes)
{
    using std::max;
    using std::min;
    using std::vector;
    using Eigen::Vector4f;
    // grid
    float Rmax = max(R1, R2); Rmax = max(Rmax, R3);
    Vector4f r0=coordsVdW.at(atom_i);
    r0[3]=0.0;
    int npm = (int)floor(L / dg);
    int ng = 2 * npm + 1;
    int ng3 = ng * ng * ng;
    vector<float> grid;
    grid.reserve(ng);
    vector<Vector4f> dgridsq;
    dgridsq.reserve(ng);
    for(int i = -npm; i <= npm; i++)
    {
        grid.push_back(i*dg);
        int tmp=(2*i+1)*dg*dg;
        dgridsq.emplace_back(tmp,tmp,tmp,tmp);
    }

    const unsigned NAtoms=coordsVdW.size();
    float  vdWRMax=coordsVdW.front()(3);
    for(const auto& v:coordsVdW)
    {
        vdWRMax=max(v(3),vdWRMax);
    }

    // select atoms potentially within reach, excluding the attachment point
    float rmaxsq = (L + Rmax + vdWRMax) * (L + Rmax + vdWRMax), rmax, r, rsq;
    Vector4f dr;
    vector<int> atomindex;
    for (unsigned i = 0; i < NAtoms; i++)
    {
        dr = coordsVdW.at(i) - r0;
        dr(3)=0.0f;
        rsq = dr.squaredNorm();
        if ((rsq < rmaxsq) && (i != atom_i)) atomindex.push_back(i);
    }
    const int natomsgrid = atomindex.size();

    // local coordinates
    std::vector<Eigen::Vector4f> ra;
    ra.reserve(natomsgrid);
    for (int i = 0; i < natomsgrid; i++)
    {
        int n = atomindex[i];
        ra.push_back(coordsVdW.at(n)-r0);
    }

    // search for allowed positions
    vector<unsigned char> clash(ng3,0);

    // search for positions causing clashes with atoms
    int ix2, iy2, ir2, rmaxsqint;
    float dx2, dy2, rmaxsq_dye1, rmaxsq_dye2, rmaxsq_dye3, rmaxsq_linker;
    int ixmin, ixmax, iymin, iymax, izmin, izmax, offset;
    for (int i = 0; i < natomsgrid; i++)
    {
        rmaxsq_dye1 = (coordsVdW[i][3] + R1) * (coordsVdW[i][3] + R1);
        rmaxsq_dye2 = (coordsVdW[i][3] + R2) * (coordsVdW[i][3] + R2);
        rmaxsq_dye3 = (coordsVdW[i][3] + R3) * (coordsVdW[i][3] + R3);
        rmaxsq_linker = (coordsVdW[i][3] + 0.5f * W) * (coordsVdW[i][3] + 0.5f * W);
        rmax = coordsVdW[i][3] + max(Rmax, 0.5f * W);
        rmaxsq = rmax * rmax;
        Vector4f rmaxsq_dye_linker((float)rmaxsq_dye3, (float)rmaxsq_dye2,
            (float)rmaxsq_dye1, (float)rmaxsq_linker);
        ixmin = max((int)ceil((ra[i][0] - rmax) / dg), -npm);
        ixmax = min((int)floor((ra[i][0] + rmax) / dg), npm);
        izmin = max((int)ceil((ra[i][2] - rmax) / dg), -npm);
        izmax = min((int)floor((ra[i][2] + rmax) / dg), npm);

        float za_i = ra[i][2];
        Vector4f dz2((float)(-2.*za_i*dg));
        for (int ix = ixmin; ix <= ixmax; ix++)
        {
            dx2 = (grid[ix] - ra[i][0]) * (grid[ix] - ra[i][0]);
            dr[1] = sqrt(max(rmaxsq - dx2, 0.f));
            iymin = max((int)ceil((ra[i][1] - dr[1]) / dg), -npm);
            iymax = min((int)floor((ra[i][1] + dr[1]) / dg), npm);
            offset = ng * (ng * (ix + npm) + iymin + npm) + npm;
            for (int iy = iymin; iy <= iymax; iy++)
            {
                dy2 = (grid[iy] - ra[i][1]) * (grid[iy] - ra[i][1]);
                float r12 = (float)(dx2 + dy2 + (za_i - grid[izmin])*(za_i - grid[izmin]));
                Vector4f dxy2(r12);
                dxy2-=rmaxsq_dye_linker;
                for (int iz = izmin; iz <= izmax; iz++)
                {
                    Vector4f tmp;
                    clash[iz + offset] |= movemask(dxy2);
                    tmp = dgridsq[iz] + dz2;
                    dxy2 = dxy2 + tmp;
                }
                offset += ng;
            }
        }
    }

    // route linker as a flexible pipe
    std::vector<float> rlink;
    rlink.reserve(ng3);
    double rlink0;
    int ix0, iy0, iz0, linknodes_eff, dlz = 2 * linknodes + 1;
    std::vector<int> newpos;
    newpos.reserve(ng3);

    for (int i = 0; i < ng3; i++) {
        rlink.push_back((clash[i] & GridFlags::ClashLinker) ? -L : L + L);
    }

    // (1) all positions within linkerinitialsphere*W from the attachment point are allowed
    rmaxsqint = (int)floor(linkersphere * linkersphere * W * W / dg / dg);
    ixmax = min((int)floor(linkersphere * W / dg), npm);
    //n = 0;
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
                rlink[iz + offset] = sqrt((double)(ix2 + iy2 + iz * iz)) * dg;
                newpos.push_back(ng * (ng * (npm + ix) + iy + npm) + npm + iz);
            }
            offset += ng;
        }
    }

    // (2) propagate from new positions
    vector<float> sqrts_dg;
    sqrts_dg.resize((2 * linknodes*linknodes + 1) * (2 * linknodes + 1));//suboptimal!
    for (int ix = 0; ix <= linknodes; ix++)
    for (int iy = 0; iy <= linknodes; iy++)
    for (int iz = -linknodes; iz <= linknodes; iz++)
        sqrts_dg[(ix*ix + iy*iy)*dlz + iz + linknodes] = sqrt((float)(ix*ix + iy*iy + iz*iz)) * dg;

    int nnew=newpos.size();//+1?
    while (nnew > 0)
    {
        for (int n = 0; n < nnew; n++)
        {
            rlink0 = rlink[newpos[n]];
            linknodes_eff = min(linknodes, (int)floor((L - rlink0) / dg));
            ix0 = newpos[n] / (ng*ng);
            iy0 = newpos[n] / ng - ix0*ng;
            iz0 = newpos[n] - ix0*ng*ng - iy0*ng;
            ixmin = max(-linknodes_eff, -ix0);
            ixmax = min(linknodes_eff, 2 * npm - ix0);
            iymin = max(-linknodes_eff, -iy0);
            iymax = min(linknodes_eff, 2 * npm - iy0);
            izmin = max(-linknodes_eff, -iz0);
            izmax = min(linknodes_eff, 2 * npm - iz0);

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

    // search for positions satisfying everything
    Eigen::Vector3f r0_3(r0[0],r0[1],r0[2]);
    vector<Eigen::Vector3f> points;
    int n = 0; int dn = 0;
    int ng2=ng*ng;
    for (int ix = -npm; ix <= npm; ix++)
    {
        offset = ng2 * (ix + npm) + npm;
        for (int iy = -npm; iy <= npm; iy++)
        {
            for (int iz = -npm; iz <= npm; iz++){
                int i=iz + offset;//==i++
                if ((clash[i] & GridFlags::ClashLinker) || rlink[i] > L) continue;
                dn = ((~clash[i] & GridFlags::ClashDyeR3) >> 3) + ((~clash[i] & GridFlags::ClashDyeR2) >> 2) + ((~clash[i] & GridFlags::ClashDyeR1) >> 1);
                n += dn;
                if (dn > 0)
                {
                    Eigen::Vector3f p(ix,iy,iz);
                    points.push_back(p*dg+r0_3);
                }
            }
            offset += ng;
        }
    }

    return points;
}
