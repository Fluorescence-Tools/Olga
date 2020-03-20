#include "MolecularTrajectory.h"

MolecularTrajectory::MolecularTrajectory()
{
}

MolecularTrajectory MolecularTrajectory::fromPdb(const std::string &fileName)
{
	MolecularTrajectory tr;
	auto fileNamePtr = std::make_shared<std::string>(fileName);
	tr.setTopology(fileNamePtr);
	tr.addPdbChunk(fileNamePtr);
	return tr;
}

std::vector<MolecularTrajectory>
MolecularTrajectory::fromPdbs(const std::vector<std::string> &fileNames)
{
	std::vector<MolecularTrajectory> trajectories;
	for (const std::string &fName : fileNames) {
		if (fName.empty()) {
			continue;
		}
		trajectories.emplace_back(fromPdb(fName));
	}
	return trajectories;
}
