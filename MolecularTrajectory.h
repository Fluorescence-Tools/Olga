#ifndef MOLECULARTRAJECTORY_H
#define MOLECULARTRAJECTORY_H

#include <string>
#include <vector>
#include <utility>

#include <boost/icl/interval_set.hpp>


class MolecularTrajectory
{
public:
	MolecularTrajectory();
	int frameCount(unsigned trajIndex) const
	{
		assert(trajIndex<_trajectories.size());
		return _trajectories[trajIndex].second.size();
	}
	int trajCount() const
	{
		return _trajectories.size();
	}
	const std::string& topologyFileName() const
	{
		return _topFileName;
	}
	const std::string& trajectoryFileName(unsigned traj) const
	{
		assert(traj<_trajectories.size());
		return _trajectories[traj].first;
	}
	int frameNum(int trajIndex,int frameIndex) const
	{
		(void)trajIndex; (void)frameIndex;
		//TODO: implement correct frame number determination
		return 0;
	}
	int totalFrameCount() const
	{
		int total=0;
		for(const auto& pair:_trajectories)
		{
			total+=pair.second.size();
		}
		return total;
	}
	bool setTopology(const std::string& fileName)
	{
		_topFileName=fileName;
		return true;
	}
	bool loadFrame(const std::string& fileName)
	{
		using boost::icl::interval_set;
		auto frame=std::make_pair(fileName,interval_set<int>().insert(0));
		_trajectories.push_back(frame);
		return true;
	}
private:
	std::string _topFileName;
	using FrameName_Numbers=std::pair<std::string,boost::icl::interval_set<int>>;//trajectoryFileName,frameNumbers
	std::vector<FrameName_Numbers> _trajectories;
};

#endif // MOLECULARTRAJECTORY_H
