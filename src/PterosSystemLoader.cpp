#include <set>
#include <pteros/pteros.h>

#include "PterosSystemLoader.h"
#include "CalcResult.h"

async::threadpool_scheduler PterosSystemLoader::_threadpool(1);
PterosSystemLoader::PterosSystemLoader()
{
}

PterosSystemLoader::~PterosSystemLoader()
{
}

PterosSystemLoader::PterosSysTask
PterosSystemLoader::makeTask(const FrameDescriptor &frame) const
{
	return async::
		spawn(_threadpool, [frame, this] {
			try {
				return load(frame);
			} catch (...) {
				std::cerr
					<< "Could not load molecule (exception): "
						   + frame.fullName() + "\n";
				return pteros::System();
			}
		}).share();
}

auto PterosSystemLoader::getDcd(const std::string &topPath,
				const std::string &trajPath) const
{
	auto it = _trajCache.find(trajPath);
	if (it == _trajCache.end()) {
		try {
			pteros::System sys(topPath);
			sys.frame_delete();
			sys.load(trajPath);
			it = _trajCache.emplace(trajPath, std::move(sys)).first;
		} catch (...) {
			it = _trajCache.emplace(trajPath, pteros::System())
				     .first;
			std::cerr << "ERROR! Can not load " + trajPath + "\n"
				  << std::flush;
		}
	}
	return it;
}

pteros::System PterosSystemLoader::load(const FrameDescriptor &frame) const
{
	const std::string &trajPath = frame.trajFileName();
	const std::string &trajSfx = trajPath.substr(trajPath.length() - 4);
	const std::unordered_set<std::string> trajExtensions = {".dcd", ".DCD"};
	if (trajExtensions.count(trajSfx)) {
		auto dcdIt = getDcd(frame.topologyFileName(), trajPath);
		pteros::Selection sel = dcdIt->second.select_all();
		sel.set_frame(frame.frame());
		pteros::System resSys;
		resSys.append(sel, true);
		assert(resSys.num_frames() == 1);
		return resSys;
	}
	// PDB
	try {
		return pteros::System(frame.topologyFileName());
	} catch (...) {
		std::cerr << "ERROR! Can not load " + frame.fullName()
			  << std::flush;
		return pteros::System();
	}
}
async::task<int>
PterosSystemLoader::numFrames(const std::string &topPath,
			      const std::string &trajPath) const

{
	auto task = async::spawn(_threadpool, [&topPath, &trajPath, this] {
		return getDcd(topPath, trajPath)->second.num_frames();
	});
	return task;
}
