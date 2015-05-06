#ifndef PTEROSSYSTEMLOADER_H
#define PTEROSSYSTEMLOADER_H

#include "FrameDescriptor.h"

#include <async++.h>
#include <pteros/pteros.h>

class PterosSystemLoader
{
public:
	using PterosSysTask=async::shared_task<pteros::System>;
	PterosSystemLoader();
	~PterosSystemLoader();
	PterosSysTask makeTask(const FrameDescriptor &frame) const
	{
		return async::spawn(_threadpool,[frame,this]{
				return load(frame);
			}).share();
	}
private:
	static async::threadpool_scheduler _threadpool;
	pteros::System load(const FrameDescriptor &frame) const;

};

#endif // PTEROSSYSTEMLOADER_H
