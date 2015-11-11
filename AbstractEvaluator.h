#ifndef ABSTRACTEVALUATOR_H
#define ABSTRACTEVALUATOR_H
#include "AbstractCalcResult.h"
#include "FrameDescriptor.h"
#include "TaskStorage.h"

#include <pteros/pteros.h>

#include <async++.h>

#include <QVariant>

#include <vector>
#include <memory>

class AbstractEvaluator
{
protected:
	const TaskStorage& _storage;
public:
	virtual ~AbstractEvaluator() {}

	AbstractEvaluator(const TaskStorage& storage):_storage(storage) {}

	virtual QString settingName(int row) const final
	{
		return this->setting(row).first;
	}
	virtual QVariant settingValue(int row) const final
	{
		return this->setting(row).second;
	}

	using Task=async::shared_task<std::shared_ptr<AbstractCalcResult>>;
	using PterosSysTask=async::shared_task<pteros::System>;
	using Setting=std::pair<QString,QVariant>;
//pure virtual:
	virtual Task makeTask(const FrameDescriptor &frame) const noexcept=0;
	virtual std::string columnName(int i) const = 0;
	virtual int columnCount() const = 0;
	virtual std::string className() const = 0;

	virtual Setting setting(int row) const = 0;
	virtual void setSetting(int row, const QVariant& val) = 0;
	virtual int settingsCount() const = 0;
	virtual std::string name() const = 0;
	virtual void setName(const std::string& name) = 0;


protected:
	Task getTask(const FrameDescriptor &desc, const EvalId& evId, bool persistent) const;
	PterosSysTask getSysTask(const FrameDescriptor &frame) const;
};
#endif // ABSTRACTEVALUATOR_H
