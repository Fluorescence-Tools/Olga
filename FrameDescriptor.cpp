#include "FrameDescriptor.h"
#include <string>
std::string FrameDescriptor::topologyFileName() const
{
    return _topologyFileName;
}

void FrameDescriptor::setTopologyFileName(const std::string &topologyFileName)
{
    _topologyFileName = topologyFileName;
}
std::string FrameDescriptor::trajFileName() const
{
	return _trajFileName;
}


unsigned FrameDescriptor::frame() const
{
	return _frame;
}




