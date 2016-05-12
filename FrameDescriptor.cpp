#include "FrameDescriptor.h"
#include <string>
std::string FrameDescriptor::topologyFileName() const
{
	if (_topologyFileName) {
		return *_topologyFileName;
	}
	return "";
}
/*
void FrameDescriptor::setTopologyFileName(std::shared_ptr<const std::string> topologyFileName)
{
	_topologyFileName = topologyFileName;
}*/
std::string FrameDescriptor::trajFileName() const
{
	if(_trajFileName) {
		return *_trajFileName;
	}
	return "";
}


unsigned FrameDescriptor::frame() const
{
	return _frame;
}




