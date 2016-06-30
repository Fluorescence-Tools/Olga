#include "AV/fretAV.h"


#include <vector>
#include <queue>
#include <set>

#include <Eigen/Dense>

using edge_t=std::pair<int,float>;

inline int edgeL2center(int edgeL)
{
	//Return the center position given the cube's edge length
	return (edgeL+1)/2;
}

inline int index(const int& x, const int& y, const int& z, const int& edgeL)
{
	//convert 3-D coordinates to 1-D offest
	return z+edgeL*(y+x*edgeL);
}

inline int index(const Eigen::Vector4f& rf, const float& discretizationStep,
		 const int& center, const int& edgeL)
{
	//convert real space 3-D coordinates to 1-D offest
	Eigen::Vector4i r=(rf/discretizationStep).cast <int>();
	r+=Eigen::Vector4i(center,center,center,0);
	return index(r[0],r[1],r[2],edgeL);
}

std::vector<edge_t> deltaIlist(const int& delta, const int& edgeL)
{
	//returns the list of 1-D represenataion offsets for the
	//nearest 3D-neigbours within the radius of delta
	std::vector<edge_t> diList;
	const int maxSq=delta*delta;
	diList.reserve(std::pow(delta,3));
	for (int dx=-delta; dx<=delta; ++dx) {
		for (int dy=-delta; dy<=delta; ++dy) {
			for (int dz=-delta; dz<=delta; ++dz) {
				int dSq=(dx*dx+dy*dy+dz*dz);
				if(dSq<=maxSq) {
					int di=dz+edgeL*(dy+dx*edgeL);
					diList.emplace_back(di,sqrt(float(dSq)));
				}
			}
		}
	}
	diList.shrink_to_fit();
	return diList;
}

float maxRadius(const std::vector<Eigen::Vector4f> &xyzR)
{
	using Eigen::Vector4f;
	return (*std::max_element(xyzR.begin(),xyzR.end(),
				  [](const Vector4f& l, const Vector4f& r) {
		return l[3]<r[3];}))[3];
}

std::vector<bool> xyzr2occupancy(const std::vector<Eigen::Vector4f>& xyzR,
				 const Eigen::Vector4f& rSource,
				 const float& maxLength,
				 const float& discretizationStep)
{
	//map xyzR to clash/occupancy map in discrete space
	using Eigen::Vector4f;
	using std::vector;
	const float maxR=maxRadius(xyzR);
	const int iR=1+(maxLength+maxR*2.0f)/discretizationStep;
	const int edgeL=2*iR+1;
	const int center=edgeL2center(edgeL);
	const int vol=std::pow(edgeL,3);
	const float maxLengthSq=std::pow(maxLength+maxR,2.0f);


	vector<vector<edge_t>> deltaILists;
	deltaILists.reserve(maxR/discretizationStep+1);
	for (int di=0; di<=maxR/discretizationStep; ++di) {
		deltaILists.push_back(deltaIlist(di,edgeL));
	}
	std::vector<bool> occupancy(vol,false);
	for (const Vector4f& r0:xyzR) {
		Vector4f r=r0-rSource;
		r[3]=0.0f;
		float rSq=r.squaredNorm();
		if( rSq>maxLengthSq) {
			continue;
		}
		int i0=index(r,discretizationStep,center,edgeL);
		int maxDi=r0[3]/discretizationStep;
		for (const auto& pair: deltaILists[maxDi]) {
			const int& di=pair.first;
			occupancy[i0+di]=true;
		}
	}
	return occupancy;
}

std::vector<bool> expandedOccupancy(const std::vector<bool>& occ, const int& iClash)
{
	//expand the given occupancy map by iClash radius in all directions
	std::vector<bool> expanded(occ);
	const int edgeL=std::nearbyint(std::cbrt(occ.size()));
	const auto& clashList=deltaIlist(iClash,edgeL);
	for(size_t i=0; i<occ.size(); ++i) {
		if(occ[i]) {
			for (const auto& pair: clashList) {
				const int& di=pair.first;
				expanded[i+di]=true;
			}
		}
	}
	return expanded;
}

void ignoreSphere(std::vector<bool>& occupancy,const int& ignoreR)
{
	//remove obstacles closer than ignoreR<adius> from the center (source)
	const int edgeL=std::nearbyint(std::cbrt(occupancy.size()));
	const int center=edgeL2center(edgeL);
	const std::vector<edge_t> deltaIgnore=deltaIlist(ignoreR,edgeL);
	int i0=index(center,center,center,edgeL);
	for (const auto& pair: deltaIgnore) {
		const auto& di=pair.first;
		int i=i0+di;
		occupancy[i]=false;
	}
}

void blockOutside(std::vector<bool>& occupancy,const int& maxR)
{
	//block all vertices further away from source than maxR
	const int maxRSq=maxR*maxR;
	const int edgeL=std::nearbyint(std::cbrt(occupancy.size()));
	const int center=edgeL2center(edgeL);
	int i=0;
	for(int x=0; x<edgeL; ++x) {
		for(int y=0; y<edgeL; ++y) {
			for(int z=0; z<edgeL; ++z) {
				int dSq=(x-center)*(x-center)
					+(y-center)*(y-center)
					+(z-center)*(z-center);
				if(dSq>maxRSq) {
					occupancy[i]=true;
				}
				++i;
			}
		}
	}
}

std::vector<edge_t> essentialEdges(const int& edgeL) {
	//returns the list of 1-D edge_index_offsets which matter
	//for the path length determination (Dijkstra) algorithm.
	//The shorter this list is, the faster Dijstra algorithm will run
	//at the cost of lower path length precision.
	//For example, including edges to only 6 nearest neighbours will result
	//in isopath surfaces that are cubic instead of spherical.
	std::vector<edge_t> fullList=deltaIlist(3,edgeL);
	std::set<float> allowedDist;
	for (float dSq:{1.0f,2.0f,3.0f,4.0f,5.0f,6.0f}) { //good compromise
		allowedDist.insert(sqrt(float(dSq)));
	}
	std::vector<edge_t> diList;
	for (const edge_t& e:fullList) {
		if(allowedDist.count(e.second)>0) {
			diList.push_back(e);
		}
	}
	diList.shrink_to_fit();
	return diList;
}

inline void
setNeigbours(std::vector<edge_t>& neis, const int& source,
	     const std::vector<bool>& occupancy, const std::vector<edge_t>& allEssential)
{
	//from potential relevant(essential) neighbours select those
	//which are not blocked by obstacles
	neis.clear();
	for (edge_t n:allEssential) {
		n.first+=source;
		if(!occupancy[n.first]) {
			neis.push_back(std::move(n));
		}
	}
}

std::vector<float> pathLength(const std::vector<bool>& occupancyVdWL)
{
	//perform dijkstra algorithm
	using Eigen::Vector4f;
	using std::vector;

	const int edgeL=std::nearbyint(std::cbrt(occupancyVdWL.size()));
	const int center=edgeL2center(edgeL);
	const vector<edge_t>& allEssentialEdges=essentialEdges(edgeL);
	int sourceVertex=index(center,center,center,edgeL);
	vector<float> pathL(occupancyVdWL.size(), std::numeric_limits<float>::max());
	pathL[sourceVertex] = 0;

	//it is possible to improve performance 10-20%
	//by using more efficient heap/queue implementation like radix heap
	using queue_entry_t=std::pair<float, int> ;
	std::priority_queue<queue_entry_t, vector<queue_entry_t>, std::greater<queue_entry_t>> que;
	que.emplace(0.0f, sourceVertex);
	std::vector<edge_t> neigbours;
	while (!que.empty()) {
		const queue_entry_t qt=que.top();
		que.pop();
		if (qt.first > pathL[qt.second]) continue;
		setNeigbours(neigbours,qt.second,occupancyVdWL,allEssentialEdges);
		for (const edge_t &e : neigbours) {
			queue_entry_t t;
			t.second=e.first;//tv
			t.first=qt.first+e.second;//tp
			if (t.first < pathL[t.second]) {
				pathL[t.second] = t.first;
				que.push(std::move(t));
			}
		}
	}
	return std::move(pathL);
}

std::vector<Eigen::Vector3f>
path2points(const std::vector<float>& pathL,
	    const std::vector<bool>& occupancyVdWDye,
	    const Eigen::Vector4f& rSource,
	    const float& maxRealLength, const float& discretizationStep)
{
	//check dye Clashes and convert weights grid to a point array
	using Eigen::Vector3f;
	using std::vector;
	const int edgeL=std::nearbyint(std::cbrt(pathL.size()));
	const int center=edgeL2center(edgeL);

	const float maxVerthexL=maxRealLength/discretizationStep;
	vector<Vector3f> points;
	int vertex=0;
	for (int x=0; x<edgeL; ++x) {
		for (int y=0; y<edgeL; ++y) {
			for (int z=0; z<edgeL; ++z) {
				if(pathL[vertex]<=maxVerthexL &&
				   occupancyVdWDye[vertex]==false) {
					Vector3f r(x-center,y-center,
						   z-center);
					r*=discretizationStep;
					r+=rSource.head<3>();
					points.push_back(std::move(r));
				}
				++vertex;
			}
		}
	}
	return points;
}

std::vector<Eigen::Vector3f> calculateAV(const std::vector<Eigen::Vector4f> &xyzR,
					 Eigen::Vector4f rSource, float linkerLength,
					 float linkerWidth, float dyeRadius,
					 float discretizationStep)
{
	using std::vector;
	using Eigen::Vector4f;

	const float maxR=std::max(linkerWidth*0.5f,dyeRadius);
	auto occupancyVdW=xyzr2occupancy(xyzR,rSource,linkerLength+maxR,discretizationStep);

	auto occupancyVdWL=expandedOccupancy(occupancyVdW,linkerWidth*0.5f/discretizationStep);
	ignoreSphere(occupancyVdWL,linkerWidth*0.5f/discretizationStep);
	blockOutside(occupancyVdWL,linkerLength/discretizationStep);
	const auto& pathL=pathLength(occupancyVdWL);

	auto occupancyVdWDye=expandedOccupancy(occupancyVdW,dyeRadius/discretizationStep);
	const auto& ret=path2points(pathL,occupancyVdWDye,rSource,linkerLength,discretizationStep);
	return ret;
}

std::vector<Eigen::Vector3f> calculateAV3(const std::vector<Eigen::Vector4f> &xyzR,
					 Eigen::Vector4f rSource, float linkerLength,
					 float linkerWidth, Eigen::Vector3f dyeRadii,
					 float discretizationStep)
{
	using std::vector;
	using Eigen::Vector4f;
	const float maxR=std::max(linkerWidth*0.5f,dyeRadii.maxCoeff());
	auto occupancyVdW=xyzr2occupancy(xyzR,rSource,linkerLength+maxR,discretizationStep);
	auto occupancyVdWL=expandedOccupancy(occupancyVdW,linkerWidth*0.5f/discretizationStep);
	ignoreSphere(occupancyVdWL,linkerWidth*0.5/discretizationStep);
	blockOutside(occupancyVdWL,linkerLength/discretizationStep);
	const auto& pathL=pathLength(occupancyVdWL);

	std::vector<Eigen::Vector3f> points;
	for(int i=0; i<3; i++) {
		float dyeRadius=dyeRadii[i];
		auto occupancyVdWDye=expandedOccupancy(occupancyVdW,dyeRadius/discretizationStep);
		auto cur=path2points(pathL,occupancyVdWDye,rSource,linkerLength,discretizationStep);
		points.reserve(points.size()+cur.size());
		std::move(cur.begin(),cur.end(),std::back_inserter(points));
	}
	return points;
}
