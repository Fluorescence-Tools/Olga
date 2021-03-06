#ifndef MOLECULARTRAJECTORY_H
#define MOLECULARTRAJECTORY_H

#include <string>
#include <vector>
#include <utility>

#include <pteros/pteros.h>

#include "FrameDescriptor.h"

class MolecularTrajectory
{
public:
	MolecularTrajectory();
	int frameCount(unsigned chunkIndex) const
	{
		assert(chunkIndex < _chunks.size());
		return _chunks[chunkIndex].frameCount;
	}
	int chunkCount() const
	{
		return _chunks.size();
	}
	std::shared_ptr<const std::string> topologyFileName() const
	{
		return _topFileName;
	}
	std::shared_ptr<const std::string>
	trajectoryFileName(unsigned chunkIndex) const
	{
		assert(chunkIndex < _chunks.size());
		return _chunks[chunkIndex].fileName;
	}
	int totalFrameCount() const
	{
		int total = 0;
		for (const auto &chunk : _chunks) {
			total += chunk.frameCount;
		}
		return total;
	}
	bool empty() const
	{
		return !totalFrameCount();
	}
	void setTopology(std::shared_ptr<const std::string> fileName)
	{
		//_topFileName=std::make_shared<std::string>(fileName);
		_topFileName = fileName;
	}
	bool addPdbChunk(std::shared_ptr<const std::string> fileName)
	{
		_chunks.reserve(_chunks.size() + 1);
		Chunk c;
		c.fileName = std::move(fileName);
		_chunks.emplace_back(std::move(c));
		return true;
	}
	FrameDescriptor descriptor(int chunkIdx, int frameIdx) const
	{
		const Chunk &chunk = _chunks[chunkIdx];
		/*return FrameDescriptor(*_topFileName,*(chunk.fileName),
				       chunk.frameNum(frameIdx));*/
		return FrameDescriptor(_topFileName, chunk.fileName, frameIdx);
	}
	struct Chunk {
		std::shared_ptr<const std::string> fileName;
		int frameCount = 1;
	};
	static MolecularTrajectory fromPdb(const std::string &fileName);
	static std::vector<MolecularTrajectory>
	fromPdbs(const std::vector<std::string> &fileNames);
	static MolecularTrajectory fromDcd(const std::string &topPath,
					   const std::string &trajPath,
					   int numFrames);

private:
	// std::shared_ptr<std::string> _topFileName;
	std::shared_ptr<const std::string> _topFileName;
	std::vector<Chunk> _chunks;
};

#endif // MOLECULARTRAJECTORY_H
