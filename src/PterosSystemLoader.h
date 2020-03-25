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
	PterosSysTask makeTask(const FrameDescriptor &frame) const;
	async::task<int> numFrames(const std::string &topPath,
				   const std::string &trajPath) const;

private:
	static async::threadpool_scheduler _threadpool;
	pteros::System load(const FrameDescriptor &frame) const;
	mutable std::unordered_map<std::string, pteros::System> _trajCache;
	auto getDcd(const std::string &topPath,
		    const std::string &trajPath) const;
};

#endif // PTEROSSYSTEMLOADER_H
