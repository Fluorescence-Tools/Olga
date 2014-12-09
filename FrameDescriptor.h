#ifndef FRAMEDESCRIPTOR_H
#define FRAMEDESCRIPTOR_H

#include <string>

class FrameDescriptor
{
public:
	FrameDescriptor();
	FrameDescriptor(std::string top, std::string traj,
			unsigned frame=0):_topologyFileName(std::move(top)),
		_trajFileName(std::move(traj)),_frame(std::move(frame))
	{
	}

	bool trajNameEmpty() const
	{
		return _trajFileName.empty();
	}
	bool inline operator==(const FrameDescriptor& rhs) const
	{
	    return  _topologyFileName == rhs._topologyFileName &&
			   _trajFileName == rhs._trajFileName &&
			    _frame == rhs._frame;
	}

	friend struct std::hash<FrameDescriptor>;
private:
	std::string _topologyFileName, _trajFileName;
	unsigned _frame;

};
namespace std {
template <>
struct hash<FrameDescriptor> {
	size_t operator()(const FrameDescriptor& desc) const {
		return hash<string>()(desc._topologyFileName)^
				hash<string>()(desc._trajFileName)^
				hash<unsigned>()(desc._frame);
	}
};
}

#endif // FRAMEDESCRIPTOR_H
