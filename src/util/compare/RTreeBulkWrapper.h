/*
 * RTreeBulkWrapper.h
 *
 *  Created on: Aug 22, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_COMPARE_RTREEBULKWRAPPER_H_
#define SRC_UTIL_COMPARE_RTREEBULKWRAPPER_H_

#include <boost/geometry/index/rtree.hpp>

namespace bg = boost::geometry;
namespace bgi = bg::index;
typedef bg::model::point<double, 3, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
typedef std::pair<box, size_t> idBox;

class RTreeBulkWrapper {
public:
	RTreeBulkWrapper();
	~RTreeBulkWrapper();

	void bulkLoad3DSpatialObject(std::vector<std::vector<double>>& entries);
private:
	static const size_t NODE_CAPACITY = 16;

	bgi::rtree<idBox, bgi::linear<NODE_CAPACITY> >* rtree;
};

#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <vector>
#include <iostream>

using namespace std;



RTreeBulkWrapper::RTreeBulkWrapper() : rtree(NULL) {}

RTreeBulkWrapper::~RTreeBulkWrapper() {
	if (rtree) {
		delete rtree;
	}
}

void RTreeBulkWrapper::bulkLoad3DSpatialObject(std::vector<std::vector<double>>& entries) {
	assert (entries.size() > 0);
	assert (entries[0].size() % 2 == 0);
	const size_t dim = entries[0].size() / 2;

	// transform to boost data types:
    std::vector<idBox> values;
    values.reserve(entries.size());
    for (unsigned e = 0; e < entries.size(); ++e) {
    	point lower(entries[e][0], entries[e][1], entries[e][2]);
    	point upper(entries[e][3], entries[e][4], entries[e][5]);
    	box b(lower, upper);
    	values.push_back(make_pair(b, e));
    }

    rtree = new bgi::rtree<idBox, bgi::linear<NODE_CAPACITY> > (values.begin(), values.end());
}

#endif /* SRC_UTIL_COMPARE_RTREEBULKWRAPPER_H_ */
