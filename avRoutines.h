#ifndef AVROUTINES_H
#define AVROUTINES_H
#include <vector>
#include <Eigen/Dense>
std::vector<Eigen::Vector3f> calculate3R(float L, float W, float R1, float R2, float R3, int atom_i, float dg,
                                         const std::vector<Eigen::Vector4f> &coordsVdW, //x,y,z,VdWradius
                                         float linkersphere, int linknodes // linker routing parameters
                                         );
std::vector<Eigen::Vector3f> calculate3R(float L, float W, float R1, float R2, float R3, const Eigen::Vector3f &r0, float dg,
                                         const std::vector<Eigen::Vector4f> &coordsVdW, //x,y,z,VdWradius
                                         float linkersphere, int linknodes // linker routing parameters
                                         )
{
    Eigen::Vector4f r0_4(r0[0],r0[1],r0[2],0.0f);
    int atom_i=0;
    for(const auto& v:coordsVdW)
    {
        r0_4[3]=v[3];
        if( (r0_4-v).norm()<0.01f )
        {
            return calculate3R(L,  W,  R1,  R2,  R3, atom_i,  dg,
                               coordsVdW, //x,y,z,VdWradius
                               linkersphere, linknodes);
        }
        atom_i++;
    }
}

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
#endif // AVROUTINES_H
