#include "PterosSystemLoader.h"
#include "CalcResult.h"

#include <pteros/pteros.h>

async::threadpool_scheduler PterosSystemLoader::_threadpool(1);
PterosSystemLoader::PterosSystemLoader()
{

}

PterosSystemLoader::~PterosSystemLoader()
{

}

pteros::System PterosSystemLoader::load(const FrameDescriptor &frame) const
{
	try {
		return pteros::System(frame.topologyFileName());
	}
	catch (...) {
		std::cerr<<"ERROR! Can not load "+frame.fullName()<<std::flush;
		return pteros::System();
	}
}
