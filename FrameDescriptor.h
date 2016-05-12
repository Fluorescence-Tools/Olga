#ifndef FRAMEDESCRIPTOR_H
#define FRAMEDESCRIPTOR_H

#include <string>
#include <iostream>
#include <QMetaType>
#include <memory>

class FrameDescriptor
{
public:

	FrameDescriptor(std::shared_ptr<const std::string> top, std::shared_ptr<const std::string> traj,
			unsigned frame=0):_topologyFileName(top),
		_trajFileName(traj),_frame(frame)
	{
	}
	FrameDescriptor():_topologyFileName(nullptr),_trajFileName(nullptr) {}
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

	FrameDescriptor & operator=(const FrameDescriptor&) = default;
	FrameDescriptor & operator= ( FrameDescriptor && ) = default;
	bool trajNameEmpty() const
	{
		return _trajFileName->empty();
	}
	bool inline operator==(const FrameDescriptor& rhs) const
	{
		return  *_topologyFileName == *(rhs._topologyFileName) &&
				*_trajFileName == *(rhs._trajFileName) &&
				_frame == rhs._frame;
	}
	friend struct std::hash<FrameDescriptor>;
	std::string topologyFileName() const;
	//void setTopologyFileName(std::shared_ptr<const std::string> topologyFileName);
	std::string trajFileName() const;
	unsigned frame() const;
    std::string fullName() const
    {
	if(_topologyFileName!=_trajFileName) {
	    return *_topologyFileName+","+*_trajFileName+"#"+std::to_string(_frame);
	}
	return *_trajFileName+"#"+std::to_string(_frame);
    }

private:
	std::shared_ptr<const std::string> _topologyFileName, _trajFileName;
	unsigned _frame=0;

};
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}
namespace std {
template <>
struct hash<FrameDescriptor> {
	size_t operator()(const FrameDescriptor& desc) const {

		size_t seed;
		if (desc._topologyFileName) {
			seed=hash<std::string>()(*(desc._topologyFileName));
		} else {
			seed=hash<std::string>()("");
		}
		if (desc._trajFileName) {
			hash_combine(seed,*(desc._trajFileName));
		}
		hash_combine(seed,desc._frame);
		return seed;
	}
};
}
Q_DECLARE_METATYPE(FrameDescriptor)
#endif // FRAMEDESCRIPTOR_H
