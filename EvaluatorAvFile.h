#ifndef EVALUATORAVFILE_H
#define EVALUATORAVFILE_H

#include "AbstractEvaluator.h"
#include "EvaluatorPositionSimulation.h"

class EvaluatorAvFile : public AbstractEvaluator
{
private:
    EvalId _av;
    std::string _name, _writeDirPath;
public:
    EvaluatorAvFile(const TaskStorage& storage, const std::string& name):
        AbstractEvaluator(storage),_name(name) {}
    virtual Task makeTask(const FrameDescriptor &frame) const noexcept;
    virtual std::string name() const
    {
        return _name;
    }
    virtual std::string className() const {
        return "AV File";
    }
    virtual std::string columnName(int) const
    {
        return name();
    }
    virtual int columnCount() const
    {
        return 0;
    }
    virtual int settingsCount() const
    {
        return 2;
    }
    virtual Setting setting(int row) const override
    {
        switch(row)
        {
        case 0:
        {
            EvalId id=_storage.isValid(_av)?_av:_storage.evaluatorPositionSimulation;
            return {"position_name",QVariant::fromValue(id)};
        }
        case 1:
            return {"write_dir",QString::fromStdString(_writeDirPath)};
        return {"",""};
        }
    }
    virtual void setSetting(int row, const QVariant& val)
    {
        switch(row)
        {
        case 1:
            _writeDirPath=val.toString().toStdString();
            return;
        case 0:
        {
            auto tmpId=val.value<EvalId>();
            if (tmpId==EvalId(-1)) {
                std::cerr<<"Can not set position\n"<<std::flush;
                return;
            }
            _av=tmpId;
            return;
        }
        }
    }
    virtual void setName(const std::string& name)
    {
        _name=name;
    }
private:
    virtual std::shared_ptr<AbstractCalcResult>
        calculate(const PositionSimulationResult& av,
                  const std::string& fname) const;
};

#endif // EVALUATORAVFILE_H
