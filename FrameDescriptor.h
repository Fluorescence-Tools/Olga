#ifndef FRAMEDESCRIPTOR_H
#define FRAMEDESCRIPTOR_H

#include <string>
#include <iostream>
#include <QMetaType>

class FrameDescriptor
{
public:

	FrameDescriptor(std::string top, std::string traj,
			unsigned frame=0):_topologyFileName(std::move(top)),
		_trajFileName(std::move(traj)),_frame(frame)
	{
	}
	FrameDescriptor(){}
	FrameDescriptor ( FrameDescriptor && o):
		_topologyFileName(std::move(o._topologyFileName)),
		_trajFileName(std::move(o._trajFileName)),
		_frame(std::move(o._frame)) {
	}
	FrameDescriptor( const FrameDescriptor& other ) :
		_topologyFileName(other._topologyFileName),
		_trajFileName(other._trajFileName),
		_frame(other._frame) {
	}

	FrameDescriptor & operator=(const FrameDescriptor&) = delete;
	FrameDescriptor & operator= ( FrameDescriptor && ) = delete;
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
	std::string topologyFileName() const;
	void setTopologyFileName(const std::string &topologyFileName);
	std::string trajFileName() const;
	unsigned frame() const;

private:
	std::string _topologyFileName, _trajFileName;
	unsigned _frame=0;

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
Q_DECLARE_METATYPE(FrameDescriptor);
#endif // FRAMEDESCRIPTOR_H
