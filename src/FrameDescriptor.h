#ifndef FRAMEDESCRIPTOR_H
#define FRAMEDESCRIPTOR_H

#include <string>
#include <iostream>
#include <QMetaType>
#include <memory>

class FrameDescriptor
{
public:
	FrameDescriptor(std::shared_ptr<const std::string> top,
			std::shared_ptr<const std::string> traj,
			unsigned frame = 0)
	    : _topologyFileName(top), _trajFileName(traj), _frame(frame)
	{
	}
	FrameDescriptor() : _topologyFileName(nullptr), _trajFileName(nullptr)
	{
	}
	FrameDescriptor(FrameDescriptor &&o)
	    : _topologyFileName(std::move(o._topologyFileName)),
	      _trajFileName(std::move(o._trajFileName)),
	      _frame(std::move(o._frame))
	{
	}
	FrameDescriptor(const FrameDescriptor &other)
	    : _topologyFileName(other._topologyFileName),
	      _trajFileName(other._trajFileName), _frame(other._frame)
	{
	}

	FrameDescriptor &operator=(const FrameDescriptor &) = default;
	FrameDescriptor &operator=(FrameDescriptor &&) = default;
	bool trajNameEmpty() const
	{
		return _trajFileName->empty();
	}
	bool inline operator==(const FrameDescriptor &rhs) const
	{
		return *_topologyFileName == *(rhs._topologyFileName)
		       && *_trajFileName == *(rhs._trajFileName)
		       && _frame == rhs._frame;
	}
	friend struct std::hash<FrameDescriptor>;
	std::string topologyFileName() const;
	// void setTopologyFileName(std::shared_ptr<const std::string>
	// topologyFileName);
	std::string trajFileName() const;
	unsigned frame() const;
	std::string fullName() const
	{
		if (_frame == 0 && _topologyFileName == _trajFileName) {
			return *_trajFileName;
		}
		return *_topologyFileName + "," + *_trajFileName + "_"
		       + std::to_string(_frame);
	}

private:
	std::shared_ptr<const std::string> _topologyFileName, _trajFileName;
	unsigned _frame = 0;
};

template <typename T>
inline void hash_combine(std::uint16_t &seed, const T &val)
{
	seed ^= std::hash<T>{}(val) + 0x9e37U + (seed << 3) + (seed >> 1);
}

template <typename T>
inline void hash_combine(std::uint32_t &seed, const T &val)
{
	seed ^= std::hash<T>{}(val) + 0x9e3779b9U + (seed << 6) + (seed >> 2);
}

template <typename T>
inline void hash_combine(std::uint64_t &seed, const T &val)
{
	seed ^= std::hash<T>{}(val) + 0x9e3779b97f4a7c15LLU + (seed << 12)
		+ (seed >> 4);
}

namespace std
{
template <> struct hash<FrameDescriptor> {
	size_t operator()(const FrameDescriptor &desc) const
	{

		size_t seed = hash<unsigned>()(desc._frame);
		if (desc._topologyFileName) {
			// TODO: use last 32 characters only for performance
			hash_combine(seed, *(desc._topologyFileName));
		}
		if (desc._trajFileName) {
			hash_combine(seed, *(desc._trajFileName));
		}
		return seed;
	}
};
} // namespace std
Q_DECLARE_METATYPE(FrameDescriptor)
#endif // FRAMEDESCRIPTOR_H
