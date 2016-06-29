#ifndef FRETAV_H
#define FRETAV_H
#include <vector>
#include <Eigen/Dense>

std::vector<Eigen::Vector3f> calculateAV(const std::vector<Eigen::Vector4f> &xyzR,
					 Eigen::Vector4f rSource, float linkerLength,
					 float linkerWidth, float dyeRadius,
					 float discretizationStep);

std::vector<Eigen::Vector3f> calculateAV3(const std::vector<Eigen::Vector4f> &xyzR,
					 Eigen::Vector4f rSource, float linkerLength,
					 float linkerWidth, Eigen::Vector3f dyeRadii,
					 float discretizationStep);
#endif // FRETAV_H
