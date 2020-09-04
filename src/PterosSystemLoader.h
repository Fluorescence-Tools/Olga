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
				   const std::string &trajPath) const;
	PterosSysTask getTask(const FrameDescriptor &frame) const;
	int taskCount() const;

private:
	mutable async::threadpool_scheduler _threadpool{
		1}; // must be single thread

	pteros::System load(const FrameDescriptor &frame) const;
	auto getDcd(const std::string &topPath,
		    const std::string &trajPath) const;
	auto getTaskIterator(const FrameDescriptor &frame) const;
	PterosSysTask makeTask(const FrameDescriptor &frame) const;

	mutable std::unordered_map<std::string, pteros::System> _trajCache;
	mutable std::unordered_map<FrameDescriptor, PterosSysTask> _sysCache;
	const size_t _sysRingBufSize = 64;
	mutable std::vector<FrameDescriptor> _sysRingBuf{_sysRingBufSize};
	mutable size_t sysRingBufIndex = 0;
};

#endif // PTEROSSYSTEMLOADER_H
