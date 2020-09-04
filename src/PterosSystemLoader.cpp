#include <set>
#include <pteros/pteros.h>

#include "PterosSystemLoader.h"
#include "CalcResult.h"

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
	if (trajExtensions.count(trajSfx) > 0) {
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

auto PterosSystemLoader::getTaskIterator(const FrameDescriptor &frame) const
{
	static auto tid = std::this_thread::get_id();
	assert(tid == std::this_thread::get_id());
	auto it = _sysCache.find(frame);
	if (it != _sysCache.end()) {
		return it;
	} else {
		auto &oldKey = _sysRingBuf[sysRingBufIndex];
		_sysCache.erase(oldKey);
		oldKey = frame;
		++sysRingBufIndex;
		sysRingBufIndex %= _sysRingBufSize;
		auto pair = _sysCache.emplace(frame, makeTask(frame));
		return pair.first;
	}
}
PterosSystemLoader::PterosSysTask
PterosSystemLoader::getTask(const FrameDescriptor &frame) const
{
	return async::spawn(_threadpool,
			    [frame, this] {
				    auto it = getTaskIterator(frame);
				    return it->second;
			    })
		.share();
}

int PterosSystemLoader::taskCount() const
{
	return async::spawn(_threadpool, [this] { return _sysCache.size(); })
		.get();
}
