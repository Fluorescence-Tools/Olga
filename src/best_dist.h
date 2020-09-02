#ifndef BEST_DIST_H
#define BEST_DIST_H

#include "chisqdist.hpp"
#include "spline.hpp"

#include <pteros/pteros.h>
#include "theobald_rmsd.h"
#include "center.h"

#include <Eigen/Dense>

#include <vector>
#include <cstdint>
#include <thread>


// TODO:
// 1) Utilize symmetry of the RMSD and Chi2 Matrices.
// 2) Try using half-float.

template <typename Lambda>
void mtFor(int start, int end, Lambda &&func, int numThreads = -1)
{
	numThreads = numThreads < 1 ? std::thread::hardware_concurrency()
				    : numThreads;
	const int numIter = end - start;
	numThreads = std::min(numThreads, numIter);

	std::vector<std::thread> threads(numThreads);
	const int grainSize = (numIter + numThreads - 1) / numThreads; // ceil
	for (int t = 0; t < numThreads; ++t) {
		const int threadStart = start + grainSize * t;
		const int threadEnd = std::min(threadStart + grainSize, end);
		threads[t] = std::thread([&, threadStart, threadEnd] {
			for (int i = threadStart; i < threadEnd; ++i) {
				func(i);
			}
		});
	}
	for (auto &&t : threads) {
		t.join();
	}
}

Eigen::MatrixXf sliceCols(const Eigen::MatrixXf &M,
			  const std::vector<unsigned> &indexes)
{
	const unsigned cols = indexes.size();
	Eigen::MatrixXf res(M.rows(), cols);
	for (unsigned i = 0; i < cols; ++i) {
		res.col(i) = M.col(indexes[i]);
	}
	return res;
}

Spline<2> chiSqRTSpline(unsigned ndof, unsigned Npieces,
			const float tolerance = 0.0001,
			const unsigned sampling = 10)
{
	// Approximate the chisqRTcdf function by a second degree polynomial

	assert(ndof > 0);
	assert(Npieces > 2);
	Npieces = std::max(Npieces, 3u);
	double xMax = ndof - 0.65; //~=CHISQINV(0.5,ndof)
	while (chisqRTcdf(xMax, ndof) > tolerance) {
		xMax += -0.1 * std::log(tolerance)
			* std::sqrt(ndof); //~width of chi-squared distribution
	}

	const unsigned nSamp = sampling * Npieces;
	Eigen::Matrix2Xf xy(2, nSamp);
	const float dx = xMax / (nSamp - 2 * sampling);
	const float xmin = -dx * sampling;
	for (unsigned i = 0; i < sampling; ++i) {
		float x = xmin + dx * (i + 1);
		xy(0, i) = x;
		xy(1, i) = 1.0f;
	}
	for (unsigned i = sampling; i < nSamp - sampling; ++i) {
		float x = dx * (i - sampling);
		xy(0, i) = x;
		xy(1, i) = chisqRTcdf(x, ndof);
	}
	for (unsigned i = nSamp - sampling; i < nSamp; ++i) {
		float x = dx * (i - sampling);
		xy(0, i) = x;
		xy(1, i) = 0.0f;
	}

	auto res = Spline<2>::fromSorted(xy, Npieces);
	const unsigned iWorst = res.maxAbsDiffArg(xy);
	const float diff = res.value_unsafe(xy(0, iWorst)) - xy(1, iWorst);
	using std::fabs;
	// assert(fabs(diff)<tolerance || ndof==1);
	return res;
}

template <typename Derived1, typename Derived2>
float rmsdColMean(const Eigen::MatrixBase<Derived1> &rmsd,
		  const Eigen::MatrixBase<Derived2> &chi2,
		  const Spline<2> &weightFunc, const float diagWeight)
{
	constexpr int batchSize = 2048; // cache performance optimization

	const int N = int(rmsd.size());
	assert(N == chi2.size());
	assert(chi2.cols() == 1);

	float sum = 0.0, prodSum = 0.0f;
	int i;
	for (i = 0; i < N - batchSize; i += batchSize) {
		auto chi2Batch = chi2.template segment<batchSize>(i);
		auto sf = weightFunc.values_unsafe(chi2Batch);
		auto rmsdBatch = rmsd.template segment<batchSize>(i);
		prodSum += sf.cwiseProduct(rmsdBatch).sum();
		sum += sf.sum();
	}
	const int nLast = N - i;
	auto sf = weightFunc.values_unsafe(chi2.tail(nLast));
	prodSum += sf.cwiseProduct(rmsd.tail(nLast)).sum();
	sum += sf.sum();
	return prodSum / (sum - 1.0f + diagWeight);
}

float rmsdMeanMeanAdd(const Eigen::MatrixXf &rmsds, const Eigen::MatrixXf &chi2,
		      const Eigen::VectorXf &Eadd, const float err,
		      const Spline<2> &spl, const float diagWeight)
{
	const float invErrSq = 1.0f / (err * err);

	const unsigned N = rmsds.rows();
	Eigen::VectorXf ave(N);
	for (unsigned c = 0; c < N; ++c) {
		auto dChi = (Eadd.array() - Eadd[c]).abs2().matrix() * invErrSq;
		auto chi2New = chi2.col(c) + dChi;
		ave[c] = rmsdColMean(rmsds.col(c), chi2New, spl, diagWeight);
	}
	return ave.mean();
}

float rmsdMeanMean(const Eigen::MatrixXf &rmsds, const Eigen::MatrixXf &chi2,
		   unsigned Ndof, const float diagWeight)
{
	assert(rmsds.rows() == rmsds.cols());

	const Spline<2> spl = chiSqRTSpline(Ndof, 64);
	const unsigned N = rmsds.rows();
	Eigen::VectorXf ave(N);
	for (unsigned col = 0; col < N; ++col) {
		ave[col] = rmsdColMean(rmsds.col(col), chi2.col(col), spl,
				       diagWeight);
	}
	return ave.mean();
}

Eigen::MatrixXf chiSquared(const Eigen::MatrixXf &Effs, const float err)
{
	const unsigned nConf = Effs.rows();
	Eigen::MatrixXf M = Eigen::MatrixXf::Zero(nConf, nConf);
	const float invErrSq = 1.0f / (err * err);
	const unsigned N = Effs.rows();
	for (unsigned pair = 0; pair < Effs.cols(); ++pair) {
		auto E = Effs.col(pair);
		for (unsigned col = 0; col < N; ++col) {
			M.col(col) += ((E.array() - E[col])).abs2().matrix()
				      * invErrSq;
		}
	}
	assert(M.minCoeff() >= 0.0f);
	return M;
	/*for(unsigned i=0; i<nConf; ++i) {
		M.col(i) = (Effs.rowwise() -
	Effs.row(i)).cwiseAbs2().rowwise().sum() * invErrSq;
	}*/
}

unsigned bestPair(const Eigen::MatrixXf &Effs, const Eigen::MatrixXf &RMSDs,
		  const float err, const float diagWeight,
		  const std::vector<unsigned> &selPairs, const bool uniqueOnly)
{
	using Eigen::MatrixXf;
	using Eigen::VectorXf;

	MatrixXf x2;
	{
		MatrixXf Esel = sliceCols(Effs, selPairs);
		x2 = chiSquared(Esel, err);
	}

	std::vector<float> rmsdAve(Effs.cols(), -1.0f);
	const unsigned nDof = std::max(int(selPairs.size()) - 1, 1);
	const Spline<2> spl = chiSqRTSpline(nDof, 64);
	mtFor(0, Effs.cols(), [&](int i) {
		rmsdAve[i] = rmsdMeanMeanAdd(RMSDs, x2, Effs.col(i), err, spl,
					     diagWeight);
	});

	if (uniqueOnly) {
		// TODO: optimize, this is slow in some cases
		for (int i : selPairs) {
			rmsdAve[i] = std::numeric_limits<float>::max();
		}
	}

	auto it = std::min_element(rmsdAve.begin(), rmsdAve.end());
	assert(*it >= 0.0f);
	return std::distance(rmsdAve.begin(), it);
}

std::vector<unsigned>
greedySelection(const float err, const Eigen::MatrixXf &Effs,
		const Eigen::MatrixXf &RMSDs, const int maxPairs,
		std::atomic<float> &fractionDone, const bool uniqueOnly)
{
	std::vector<unsigned> selPairs;
	for (int pairsDone = 0; pairsDone < maxPairs; ++pairsDone) {
		unsigned best =
			bestPair(Effs, RMSDs, err, 0.99f, selPairs, uniqueOnly);
		selPairs.push_back(best);
		fractionDone = pairsDone / maxPairs;
	}
	return selPairs;
}

std::vector<unsigned> greedySelection(const float err,
				      const Eigen::MatrixXf &Effs,
				      const Eigen::MatrixXf &RMSDs,
				      const int maxPairs, const bool uniqueOnly)
{
	std::atomic<float> fractionDone;
	assert(!Effs.hasNaN());
	return greedySelection(err, Effs, RMSDs, maxPairs, fractionDone,
			       uniqueOnly);
}

std::vector<float> sys2xyz(const pteros::System &s)
{
	std::vector<float> vec;
	vec.reserve(s.num_atoms() * s.num_frames() * 3);
	for (int fr = 0; fr < s.num_frames(); ++fr) {
		const float *beg = s.frame(fr).coord.data()->data();
		vec.insert(vec.end(), beg, beg + 3 * s.num_atoms());
		/*
		for(int at=0; at<s.num_atoms(); ++at) {
			const float* beg=s.Frame_data(fr).coord.at(at).data();
			vec.insert(vec.end(),beg,beg+3);
		}*/
	}
	return vec;
}

Eigen::MatrixXf rmsd2d(const pteros::System &traj,
		       std::atomic<float> &fractionDone)
{
	const int numFrames = traj.num_frames();
	const int nAtoms = traj.num_atoms();
	std::vector<float> xyz = sys2xyz(traj);
	std::vector<float> traces(numFrames);
	inplace_center_and_trace_atom_major(xyz.data(), traces.data(),
					    numFrames, nAtoms);
	Eigen::MatrixXf RMSDs(numFrames, numFrames);

	const int64_t numRmsds = int64_t(numFrames) * numFrames / 2;
	std::atomic<std::int64_t> rmsdsDone{0};

	std::vector<std::thread> threads(std::thread::hardware_concurrency());
	const int grainSize = numFrames / threads.size() + 1;
	for (int t = 0; t < threads.size(); ++t) {
		threads[t] = std::thread([=, &RMSDs, &xyz, &rmsdsDone,
					  &fractionDone] {
			const int maxFr =
				std::min((t + 1) * grainSize, numFrames);
			for (int fr = t * grainSize; fr < maxFr; ++fr) {
				for (int j = fr; j < numFrames; ++j) {
					const float *xyz_fr =
						xyz.data() + 3 * fr * nAtoms;
					const float *xyz_j =
						xyz.data() + 3 * j * nAtoms;
					RMSDs(fr, j) = msd_atom_major(
						nAtoms, nAtoms, xyz_fr, xyz_j,
						traces[fr], traces[j], 0,
						nullptr);
					RMSDs(j, fr) = RMSDs(fr, j);
				}
				rmsdsDone += numFrames - fr;
				fractionDone =
					float(rmsdsDone) / float(numRmsds);
			}
		});
	}
	for (auto &&t : threads) {
		t.join();
	}
	RMSDs = RMSDs.cwiseSqrt() * 10.0f;
	return RMSDs;
}

Eigen::MatrixXf rmsd2d(const pteros::System &traj)
{
	std::atomic<float> stub;
	return rmsd2d(traj, stub);
}

std::vector<unsigned> thresholdNansPerCol(const Eigen::MatrixXf &E,
					  double maxNanFraction)
{
	std::vector<unsigned> goodIdxs;

	const int maxNan = int(maxNanFraction * E.rows()) + 1;
	Eigen::VectorXi numNanCol =
		E.array().isNaN().cast<int>().colwise().sum();
	for (unsigned i = 0; i < E.cols(); ++i) {
		if (numNanCol[i] < maxNan) {
			goodIdxs.push_back(i);
		}
	}
	return goodIdxs;
}

Eigen::VectorXi fillNans(Eigen::MatrixXf &E, const Eigen::MatrixXf &RMSDs)
{
	// replace NaNs with Efficiencies from similar structures
	Eigen::VectorXi nansFilled = Eigen::VectorXi::Zero(E.cols());
	const float maxFloat = std::numeric_limits<float>::max();
	for (unsigned col = 0; col < E.cols(); ++col) {
		const Eigen::VectorXf filter =
			E.col(col).array().isNaN().matrix().cast<float>()
			* maxFloat;
		for (unsigned row = 0; row < E.rows(); ++row) {
			if (not std::isnan(E(row, col))) {
				continue;
			}
			auto fRMSDS = RMSDs.col(row).cwiseMax(filter);
			unsigned similarConf;
			fRMSDS.minCoeff(&similarConf);
			E(row, col) = E(similarConf, col);
			nansFilled(col)++;
		}
	}
	return nansFilled;
}

Eigen::VectorXf precisionDecay(const std::vector<unsigned> &pairIdxs,
			       const Eigen::MatrixXf &E,
			       const Eigen::MatrixXf &RMSDs, double err)
{
	Eigen::VectorXf decay(pairIdxs.size());
	std::vector<unsigned> tmpPairs;
	for (int i = 0; i < pairIdxs.size(); ++i) {
		tmpPairs.push_back(pairIdxs[i]);
		auto chi2 = chiSquared(sliceCols(E, tmpPairs), err);
		decay[i] = rmsdMeanMean(RMSDs, chi2, i + 1, 0.99f);
	}
	return decay;
}


/*
template <typename M>
M load_tsv(const std::string &path, const unsigned skipRows = 0,
	   const unsigned skipCols = 0)
{
	using namespace Eigen;
	using scalar_type = typename M::Scalar;
	std::ifstream indata;
	indata.open(path);
	std::string line;
	std::vector<scalar_type> values;
	uint64_t rows = 0;
	const auto maxStreamSize = std::numeric_limits<std::streamsize>::max();
	for (unsigned i = 0; i < skipRows; ++i) {
		indata.ignore(maxStreamSize, '\n');
	}
	while (std::getline(indata, line)) {
		std::stringstream lineStream(line);
		std::string tmp;
		for (unsigned i = 0; i < skipCols; ++i) {
			lineStream.ignore(maxStreamSize, '\t');
		}
		while (lineStream.good()) {
			lineStream >> tmp;
			values.push_back(std::stof(tmp));
		}
		++rows;
	}
	const uint64_t cols = values.size() / rows;
	assert(rows * cols == values.size());
	return Map<const Matrix<scalar_type, M::RowsAtCompileTime,
				M::ColsAtCompileTime, RowMajor>>(
		values.data(), rows, values.size() / rows);
}

std::vector<std::string> loadPairNames(const std::string &path)
{
	std::vector<std::string> pairNames;

	std::ifstream indata;
	indata.open(path);
	std::string line, tmp;
	std::getline(indata, line);
	std::stringstream lineStream(line);
	// ignore "structure"
	lineStream.ignore(std::numeric_limits<std::streamsize>::max(), '\t');
	while (lineStream.good()) {
		lineStream >> tmp;
		pairNames.push_back(tmp);
	}
	return pairNames;
}


void selectFromFiles()
{
	using std::remove_if;

	auto RMSDs = load_tsv<Eigen::MatrixXf>("RMSDs.dat");

	const std::string effsPath = "effs.dat";
	auto Effs = load_tsv<Eigen::MatrixXf>(effsPath, 1, 1);
	std::vector<std::string> pairNames = loadPairNames(effsPath);

	const double maxNanFraction = 0.2;
	const int maxNan = int(maxNanFraction * Effs.rows());
	std::vector<unsigned> keepIdxs;
	for (unsigned i = 0; i < Effs.cols(); ++i) {
		int numNan = Effs.array().col(i).isNaN().cast<int>().sum();
		if (numNan < maxNan) {
			keepIdxs.push_back(i);
		} else {
			pairNames[i].erase();
		}
	}
	Effs = sliceCols(Effs, keepIdxs);
	auto isEmptyStr = [](const std::string &s) { return s.empty(); };
	auto it = remove_if(pairNames.begin(), pairNames.end(), isEmptyStr);
	pairNames.erase(it, pairNames.end());

	assert(pairNames.size() == Effs.cols());

	const float maxFloat = std::numeric_limits<float>::max();
	for (unsigned col = 0; col < Effs.cols(); ++col) {
		const Eigen::VectorXf filter =
			Effs.col(col).array().isNaN().matrix().cast<float>()
			* maxFloat;
		for (unsigned row = 0; row < Effs.rows(); ++row) {
			if (not std::isnan(Effs(row, col))) {
				continue;
			}
			auto fRMSDS = RMSDs.col(row).cwiseMax(filter);
			unsigned similarConf;
			fRMSDS.minCoeff(&similarConf);
			Effs(row, col) = Effs(similarConf, col);
		}
	}

	const float err = 0.06;
	std::vector<unsigned> selPairs; //={9,8,7,6,5,4};
	for (unsigned i = 0; i < 5; ++i) {
		unsigned best = bestPair(Effs, RMSDs, err, 0.99f, selPairs);
		selPairs.push_back(best);
		auto chi2 = chiSquared(sliceCols(Effs, selPairs), err);
		float rmsdAve = rmsdMeanMean(RMSDs, chi2, i + 1, 0.99f);
		std::cout << i << "\t" << pairNames[best] << "\t" << rmsdAve
			  << std::endl;
	}
}

void benchmark()
{
	constexpr unsigned Nconf = 4000;
	constexpr unsigned Npairs = 100;

	using Eigen::MatrixXf;
	using Eigen::VectorXf;
	MatrixXf Effs = MatrixXf::Random(Nconf, Npairs);

	Effs.array() += 1.0f;
	Effs *= 0.5f;
	Effs.col(0).setLinSpaced(Effs.rows(), 0.1f, 0.9f);
	Effs.col(1).setLinSpaced(Effs.rows(), 0.0f, 1.0f);

	MatrixXf RMSDs =
		Effs.col(1) * Eigen::VectorXf::Ones(Nconf).transpose() * 10.0f;
	RMSDs -= RMSDs.transpose().eval();
	RMSDs = RMSDs.cwiseAbs();

	assert(Effs.array().isNaN().sum() == 0);
	assert(RMSDs.rows() == Effs.rows());

	const float err = 0.06;
	std::vector<unsigned> selPairs; //={9,8,7,6,5,4};
	for (unsigned i = 0; i < 5; ++i) {
		auto start = std::chrono::steady_clock::now();
		unsigned best = bestPair(Effs, RMSDs, err, 0.99f, selPairs);
		auto diff = std::chrono::steady_clock::now() - start;
		double dtMs =
			std::chrono::duration<double, std::milli>(diff).count();

		selPairs.push_back(best);
		auto chi2 = chiSquared(sliceCols(Effs, selPairs), err);
		float rmsdAve = rmsdMeanMean(RMSDs, chi2, i + 1, 0.99f);
		std::cout << i << "\t" << rmsdAve
			  << " bestPair() took: " << dtMs << " ms" << std::endl;
	}
}
*/

#endif // BEST_DIST_H
