#ifndef PTEROSSYSTEMLOADER_H
#define PTEROSSYSTEMLOADER_H

#include "FrameDescriptor.h"

#include <async++.h>
#include <pteros/pteros.h>

class PterosSystemLoader
{
public:
	using PterosSysTask = async::shared_task<pteros::System>;
	PterosSystemLoader();
	~PterosSystemLoader();

	async::task<int> numFrames(const std::string &topPath,
				   const std::string &trajPath);
	PterosSysTask getTask(const FrameDescriptor &frame);
	int taskCount() const;

private:
	mutable async::threadpool_scheduler _threadpool{
		1}; // must be single thread

	pteros::System load(const FrameDescriptor &frame);
	auto getDcd(const std::string &topPath, const std::string &trajPath);
	auto getTaskIterator(const FrameDescriptor &frame);
	PterosSysTask makeTask(const FrameDescriptor &frame);

	std::unordered_map<std::string, pteros::System> _trajCache;
	std::unordered_map<FrameDescriptor, PterosSysTask> _sysCache;
	const size_t _sysRingBufSize = 64;
	std::vector<FrameDescriptor> _sysRingBuf{_sysRingBufSize};
	size_t sysRingBufIndex = 0;
};

#endif // PTEROSSYSTEMLOADER_H
