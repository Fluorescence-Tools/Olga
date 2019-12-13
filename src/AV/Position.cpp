#include "Position.h"
#include "Distance.h"
#include "PositionSimulation.h"

#include <map>
#include <iostream>
#include <string>
#include <future>

#include <QVariant>
#include <QJsonDocument>
#include <QCoreApplication>

Position::Position()
{
	_simulation = PositionSimulation::create(_simulationType);
}

Position::~Position()
{
	delete _simulation;
}

Position::Position(const Position &other)
    : _name(other.name()), _chainIdentifier(other._chainIdentifier),
      _residueSeqNumber(other.residueSeqNumber()),
      _residueName(other.residueName()), _atomName(other.atomName()),
      _simulationType(other.simulationType())
{
	_simulation = 0;
	if (other._simulation) {
		_simulation = other._simulation->Clone();
	}
}

Position &Position::operator=(const Position &other)
{
	_name = other.name();
	_chainIdentifier = other.chainIdentifier();
	_residueSeqNumber = other.residueSeqNumber();
	_residueName = other.residueName();
	_atomName = other.atomName();
	if (other._simulation) {
		_simulation = other._simulation->Clone();
	}
	return *this;
}

Position::Position(Position &&o)
    : _name(std::move(o._name)),
      _chainIdentifier(std::move(o._chainIdentifier)),
      _residueSeqNumber(o._residueSeqNumber),
      _residueName(std::move(o._residueName)),
      _atomName(std::move(o._atomName)),
      _simulationType(std::move(o._simulationType))
{
	_simulation = o._simulation;
	o._simulation = 0;
}

Position &Position::operator=(Position &&o)
{
	_name = std::move(o._name);
	_chainIdentifier = std::move(o._chainIdentifier);
	_residueSeqNumber = o._residueSeqNumber;
	_residueName = std::move(o._residueName);
	_atomName = std::move(o._atomName);
	_simulationType = std::move(o._simulationType);
	_simulation = o._simulation;
	o._simulation = 0;
	return *this;
}

Position::Position(const std::string &name) : Position()
{
	_name = name;
}

QMap<QString, double> loadvdWRadii(const QString &fileName)
{
	QMap<QString, double> map = {{"H", 0.1},   {"C", 0.17},   {"N", 0.1625},
				     {"O", 0.149}, {"S", 0.1782}, {"P", 0.1}};
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		std::cerr << "could not load " + fileName.toStdString() + "\n"
			  << std::endl;
		return map;
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	if (doc.isNull()) {
		std::cerr << "could parse " + fileName.toStdString() + "\n"
			  << std::endl;
		return map;
	}
	QVariantMap varMap = doc.toVariant().toMap();
	using VarMapCI = QVariantMap::const_iterator;
	for (VarMapCI i = varMap.begin(); i != varMap.end(); ++i) {
		map[i.key()] = i.value().toDouble();
	}
	return map;
}
float pterosVDW(const pteros::System &system, int i)
{
	// TODO: This is a hack. One should define a corresponding function in
	// pteros::System
	QString path =
		QCoreApplication::applicationDirPath() + "/vdWRadii.json";
	static QMap<QString, double> vdWRMap = loadvdWRadii(path);
	return vdWRMap.value(QString::fromStdString(system.atom(i).name), 0.15);
}

std::vector<Eigen::Vector4f> coordsVdW(const pteros::System &system)
{
	// iterate over atoms and fill x,y,z,vdw
	int nAtoms = system.num_atoms();
	std::vector<Eigen::Vector4f> xyzw;
	xyzw.reserve(nAtoms);

	// Fill coordinates
	const pteros::Frame &frame = system.frame(0);
	for (int i = 0; i < nAtoms; i++) {
		xyzw.emplace_back(frame.coord.at(i)[0] * 10.0f,
				  frame.coord.at(i)[1] * 10.0f,
				  frame.coord.at(i)[2] * 10.0f,
				  pterosVDW(system, i) * 10.0f);
	}
	return xyzw;
}

PositionSimulationResult Position::calculate(const pteros::System &system) const
{
	std::vector<Eigen::Vector4f> xyzW;
	Eigen::Vector3f refPos = atomXYZ(system);
	std::string stripExpr = stripExpression();
	if (stripExpr.empty()) {
		xyzW = coordsVdW(system);
	} else {
		try {
			pteros::System stripped = system;
			pteros::Selection rmSel = stripped.select(stripExpr);
			if (rmSel.size() > 0) {
				stripped.remove(stripExpr);
			}
			xyzW = coordsVdW(stripped);
		} catch (pteros::Pteros_error err) {
			std::cerr << "stripping failed: "
					     + std::string(err.what()) + ": '"
					     + stripExpr + "'\n"
				  << std::flush;
			xyzW = coordsVdW(system);
		}
	}
	return calculate(refPos, xyzW);
}


std::pair<QString, QVariant> Position::setting(int row) const
{
	using Setting = std::pair<QString, QVariant>;
	switch (row) {
	case 0:
		return Setting{"chain_identifier",
			       QString::fromStdString(_chainIdentifier)};
	case 1:
		return Setting{"residue_seq_number",
			       static_cast<int>(_residueSeqNumber)};
	case 2:
		return Setting{"residue_name",
			       QString::fromStdString(_residueName)};
	case 3:
		return Setting{"atom_name", QString::fromStdString(_atomName)};
	case 4:
		return Setting{"simulation_type",
			       QVariant::fromValue(_simulationType)};
	case 5:
		return Setting{"strip_mask",
			       QString::fromStdString(_stripMask)};
	case 6:
		return Setting{"allowed_sphere_radius", _allowedSphereRadius};
	case 7:
		return Setting{"anchor_atoms",
			       QString::fromStdString(_anchorAtoms)};
	default:
		return _simulation->setting(row - _localSettingCount);
	}
	return Setting();
}

bool isUpper(const std::string &s)
{
	return std::all_of(s.begin(), s.end(),
	                   [](unsigned char c) { return std::isupper(c); });
}

void Position::setSetting(int row, const QVariant &val)
{
	switch (row) {
	case 0:
		_chainIdentifier = val.toString().toStdString();
		return;
	case 1:
		_residueSeqNumber = val.toInt();
		return;
	case 2:
		_residueName = val.toString().toStdString();
		if (!isUpper(_residueName)) {
			std::cout << "Info: residue name '" + _residueName
			                     + "' is not uppercase.\n";
		}
		return;
	case 3:
		_atomName = val.toString().toStdString();
		return;
	case 4:
		setSimulationType(val.value<SimulationType>());
		return;
	case 5:
		_stripMask = val.toString().toStdString();
		return;
	case 6:
		_allowedSphereRadius = val.toDouble();
		return;
	case 7:
		_anchorAtoms = val.toString().toStdString();
		return;
	default:
		_simulation->setSetting(row - _localSettingCount, val);
		return;
	}
}

int Position::settingsCount() const
{
	return _localSettingCount
	       + (_simulation ? _simulation->settingsCount() : 0);
}

const std::string &Position::name() const
{
	return _name;
}

void Position::setName(const std::string &name)
{
	_name = name;
}

std::string Position::chainIdentifier() const
{
	return _chainIdentifier;
}
unsigned Position::residueSeqNumber() const
{
	return _residueSeqNumber;
}

void Position::setResidueSeqNumber(const unsigned &residueSeqNumber)
{
	_residueSeqNumber = residueSeqNumber;
}
std::string Position::residueName() const
{
	return _residueName;
}

void Position::setResidueName(const std::string &residueName)
{
	_residueName = residueName;
}
std::string Position::atomName() const
{
	return _atomName;
}

void Position::setAtomName(const std::string &atomName)
{
	_atomName = atomName;
}
Position::SimulationType Position::simulationType() const
{
	return _simulationType;
}

void Position::setSimulationType(const Position::SimulationType &simulationType)
{
	if (simulationType == _simulationType) {
		return;
	}
	_simulationType = simulationType;
	if (_simulation) {
		delete _simulation;
	}
	_simulation = PositionSimulation::create(_simulationType);
}

std::vector<Position> Position::fromLegacy(const std::string &labelingFileName,
					   const std::string &pdbFileName)
{
	std::vector<Position> positions;
	std::ifstream infile;
	infile.open(labelingFileName.c_str(), std::ifstream::in);
	if (!infile.is_open()) {
		return std::vector<Position>();
	}

	std::string str;
	while (getline(infile, str)) {
		if (str.size() == 0) {
			continue;
		}
		if (str.at(0) == '#') // comment
		{
			continue;
		}
		Position p;
		p.setFromLegacy(str, pdbFileName);
		if (p.simulationType() == SimulationType::NONE) {
			std::cerr
				<< "Could not parse position " + p.name() + "\n"
				<< std::flush;
			continue;
		}
		positions.push_back(std::move(p));
	}
	return positions;
}

Eigen::Vector3f Position::atomXYZ(const pteros::System &system) const
{

	pteros::Selection select;
	try {
		select.modify(system, selectionExpression());
	} catch (const pteros::Pteros_error &err) {
		std::cerr << err.what() << std::endl;
	}

	int selectedCount = select.size();

	if (selectedCount != 1) {
		std::cerr <<"Position \""+ _name +
			    "\". Attachment atom could not be selected. Specified selection defines "+
			    std::to_string(selectedCount)+
			    " atoms instead of one: '" +
			    selectionExpression() +"'\n"<< std::flush;
		const double nan = std::numeric_limits<float>::quiet_NaN();
		return Eigen::Vector3f(nan, nan, nan);
	}
	return select.xyz(0) * 10.0f;
}

PositionSimulationResult
Position::calculate(const Eigen::Vector3f &attachmentAtomPos,
		    const std::vector<Eigen::Vector4f> &store) const
{
	return _simulation->calculate(attachmentAtomPos, store);
}

void Position::setFromLegacy(const std::string &entry,
			     const std::string &pdbFileName)
{
	std::istringstream iss(entry);
	std::string buf;
	std::string simtypeStr;
	iss >> _name >> buf >> buf >> simtypeStr;
	_simulationType = simulationType(simtypeStr);
	delete _simulation;
	_simulation = PositionSimulation::create(_simulationType);
	if (!_simulation) {
		return;
	}
	int pdbId = _simulation->loadLegacy(iss);
	QFile data(QString::fromStdString(pdbFileName));
	if (data.open(QFile::ReadOnly)) {
		QTextStream in(&data);
		QString str(in.readAll());
		QString selection = QString("[\n^]ATOM *%1").arg(pdbId);
		int pos = str.indexOf(QRegularExpression(selection));
		str = str.mid(pos);
		QTextStream stream(&str);
		QString tmp;
		stream >> tmp >> tmp >> tmp;
		_atomName = tmp.toStdString();
		stream >> tmp;
		_residueName = tmp.toStdString();
		stream >> tmp;
		bool ok;
		_residueSeqNumber = tmp.toInt(&ok);
		if (!ok) {
			_chainIdentifier = tmp.toStdString();
			stream >> _residueSeqNumber;
		}
	}
}

std::string Position::selectionExpression() const
{
	std::string expr;
	if (_chainIdentifier != "") {
		expr += "chain " + _chainIdentifier + " and ";
	}
	expr += "resid " + std::to_string(_residueSeqNumber) + " and ";
	if (_residueName != "") {
		expr += "resname " + _residueName + " and ";
	}
	expr += "name " + _atomName + " ";
	return expr;
}

std::string Position::stripExpression() const
{
	if (_allowedSphereRadius <= 0.0) {
		return _stripMask;
	}
	// TODO: should use std::to_chars instead
	std::string num = std::to_string(_allowedSphereRadius * 0.1);
	std::replace(num.begin(), num.end(), ',', '.');
	std::string expr = "(within " + num + " noself of ("
			   + selectionExpression() + "))";
	if (!_stripMask.empty()) {
		expr += " or (" + _stripMask + ")";
	}
	return expr;
}

void Position::setChainIdentifier(const std::string &chainIdentifier)
{
	_chainIdentifier = chainIdentifier;
}
