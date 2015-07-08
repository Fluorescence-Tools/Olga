#ifndef ABSTRACTEVALUATOR_H
#define ABSTRACTEVALUATOR_H
//#include "TaskStorage.h"
#include "AbstractCalcResult.h"
#include "FrameDescriptor.h"

#include <pteros/pteros.h>

#include <async++.h>

#include <QVariant>

#include <vector>
#include <memory>

class TaskStorage;
class AbstractEvaluator
{
protected:
	const TaskStorage& _storage;
public:
	virtual ~AbstractEvaluator() {}
	using EvalPtr=std::shared_ptr<const AbstractEvaluator>;

	AbstractEvaluator(const TaskStorage& storage):_storage(storage) {}

	virtual QString settingName(int row) const final
	{
		return this->setting(row).first;
	}
	virtual QVariant settingValue(int row) const final
	{
		return this->setting(row).second;
	}
	/*virtual QVariantMap settings() const final
	{
		QVariantMap map;
		for(int i=0; i<settingsCount(); ++i) {
			map.insert(settingName(i),settingValue(i));
		}
		return map;
	}*/
//pure virtual:
	//virtual bool inline operator==(AbstractEvaluator& o) const =0;
	using Task=async::shared_task<std::shared_ptr<AbstractCalcResult>>;
	using PterosSysTask=async::shared_task<pteros::System>;
	virtual Task makeTask(const FrameDescriptor &frame) const=0;

	virtual std::string columnName(int i) const = 0;
	virtual std::string className() const = 0;

	using Setting=std::pair<QString,QVariant>;
	virtual Setting setting(int row) const = 0;
	virtual void setSetting(int row, const QVariant& val) = 0;
	virtual int settingsCount() const = 0;
	virtual std::string name() const = 0;
	virtual void setName(const std::string& name) = 0;


protected:
	Task getTask(const FrameDescriptor &desc, const std::weak_ptr<const AbstractEvaluator> eval, bool persistent) const;
	PterosSysTask getSysTask(const FrameDescriptor &frame) const;
};
Q_DECLARE_METATYPE(std::shared_ptr<const AbstractEvaluator>)
Q_DECLARE_METATYPE(std::vector<std::shared_ptr<const AbstractEvaluator>>)
#endif // ABSTRACTEVALUATOR_H
