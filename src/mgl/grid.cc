/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

 */

#include <set>
#include <map>

#include "grid.h"
#include "log.h"
#include <limits>
#include <list>
#include <vector>

namespace mgl {


using namespace std;


const Scalar GRID_RANGE_TOL = 0.0;
const Scalar GRID_CROSSING_COARSENESS = 0.0;

ostream& operator <<(std::ostream &os, const ScalarRange &p) {
	cout << "[" << p.min << ", " << p.max << "]";
	return os;
}


// local types, methods and functions

void scalarRangesFromIntersections(const std::set<Scalar> &lineCuts, std::vector<ScalarRange> &ranges) {
	ranges.reserve(lineCuts.size());
	bool inside = false;
	Scalar xBegin = 0; // initial value is not used
	Scalar xEnd = 0; // initial value is not used
	for (std::set<Scalar>::iterator it = lineCuts.begin(); it != lineCuts.end(); it++) {
		Scalar intersection = *it;
		if (inside) {
			xEnd = intersection;
			// gridSegments.push_back(LineSegment2(Point2Type(xBegin,y), Point2Type(xEnd,y)));
			ranges.push_back(ScalarRange(xBegin, xEnd));
            inside = false;
		} else {
			xBegin = intersection;
            inside = true;
		}
	}

	if (inside) {
		// this is not good. xMax should be outside the object
		GridException messup("Ray has been cast outside the model mesh.");

	}
}

void scalarRangesFromIntersections(std::vector<Scalar> &lineCuts, std::vector<ScalarRange> &ranges) {
    std::sort(lineCuts.begin(), lineCuts.end());
	ranges.reserve(lineCuts.size());
	bool inside = false;
	Scalar xBegin = 0; // initial value is not used
	Scalar xEnd = 0; // initial value is not used
	for (std::vector<Scalar>::iterator it = lineCuts.begin(); 
            it != lineCuts.end(); ++it) {
		Scalar intersection = *it;
		if (inside) {
			xEnd = intersection;
			// gridSegments.push_back(LineSegment2(Point2Type(xBegin,y), Point2Type(xEnd,y)));
			ranges.push_back(ScalarRange(xBegin, xEnd));
            inside = false;
		} else {
			xBegin = intersection;
            inside = true;
		}
	}

	if (inside) {
		// this is not good. xMax should be outside the object
		GridException messup("Ray has been cast outside the model mesh.");

	}
}

void rayCastAlongX(const std::list<Loop>& outlineLoops,
		Scalar y,
		Scalar xMin,
		Scalar xMax,
		std::vector<ScalarRange> &ranges) {
	std::vector<Scalar> lineCuts;

	//iterate over every loop
	for (std::list<Loop>::const_iterator j = outlineLoops.begin(); 
			j != outlineLoops.end(); 
			++j) {
		const Loop& currentLoop = *j;
		//iterate over each line segment in the loop
		if(!currentLoop.empty()) {
			for(Loop::const_finite_cw_iterator iter(currentLoop.clockwiseFinite()); 
					iter != currentLoop.clockwiseEnd(); 
					++iter) {
				Segment2Type segment = currentLoop.segmentAfterPoint(iter);
				Scalar intersectionX, intersectionY;
                intersectionY = y;
                if(segment.a.y < y + GRID_CROSSING_COARSENESS) {
                    if(segment.b.y > y - GRID_CROSSING_COARSENESS) {
                        Scalar t = (y - segment.a.y) / 
                                (segment.b.y - segment.a.y);
                        intersectionX = t * (segment.b.x - segment.a.x) + segment.a.x;
                    } else {
                        continue;
                    }
                } else if(segment.a.y > y - GRID_CROSSING_COARSENESS) {    // segment.a.y > y
                    if(segment.b.y < y + GRID_CROSSING_COARSENESS) {
                        Scalar t = (y - segment.b.y) / 
                                (segment.a.y - segment.b.y);
                        intersectionX = t * (segment.a.x - segment.b.x) + segment.b.x;
                    } else {
                        continue;
                    }
                }
				if (intersectionX >= xMin && intersectionX < xMax) {
					lineCuts.push_back(intersectionX);
				}
			}
		}
	}
	scalarRangesFromIntersections(lineCuts, ranges);
}

void rayCastAlongY(const std::list<Loop>& outlineLoops,
		Scalar x,
		Scalar yMin,
		Scalar yMax,
		std::vector<ScalarRange> &ranges) {
	std::vector<Scalar> lineCuts;

	// iterate over every loop
	for (std::list<Loop>::const_iterator j = outlineLoops.begin(); 
			j != outlineLoops.end(); 
			++j) {
		const Loop& currentLoop = *j;
		if(!currentLoop.empty()){
			for (Loop::const_finite_cw_iterator it(currentLoop.clockwiseFinite()); 
					it != currentLoop.clockwiseEnd(); 
					it++) {
				Segment2Type segment = currentLoop.segmentAfterPoint(it);
				Scalar intersectionX, intersectionY;
                intersectionX = x;
                if(segment.a.x < x + GRID_CROSSING_COARSENESS) {
                    if(segment.b.x > x - GRID_CROSSING_COARSENESS) {
                        Scalar t = (x - segment.a.x) / 
                                (segment.b.x - segment.a.x);
                        intersectionY = t * (segment.b.y - segment.a.y) + segment.a.y;
                    } else {
                        continue;
                    }
                } else if(segment.a.x > x - GRID_CROSSING_COARSENESS) {    // segment.a.y > y
                    if(segment.b.x < x + GRID_CROSSING_COARSENESS) {
                        Scalar t = (x - segment.b.x) / 
                                (segment.a.x - segment.b.x);
                        intersectionY = t * (segment.a.y - segment.b.y) + segment.b.y;
                    } else {
                        continue;
                    }
                }
				if (intersectionY >= yMin && intersectionY < yMax) {
					lineCuts.push_back(intersectionY);
				}
			}
		}
	}
	scalarRangesFromIntersections(lineCuts, ranges);
}

void castRaysOnSliceAlongX(const std::list<Loop> &outlineLoops,
		const std::vector<Scalar> &yValues,
		Scalar xMin,
		Scalar xMax,
		ScalarRangeTable &rangeTable) {
	assert(rangeTable.size() == 0);
	rangeTable.resize(yValues.size());

	for (size_t i = 0; i < rangeTable.size(); i++) {
		Scalar y = yValues[i];
		std::vector<ScalarRange> &ranges = rangeTable[i];
		rayCastAlongX(outlineLoops, y, xMin, xMax, ranges);
	}
}

void castRaysOnSliceAlongY(const std::list<Loop> &outlineLoops,
		const std::vector<Scalar> &values, // x
		Scalar min,
		Scalar max,
		ScalarRangeTable &rangeTable) {
	assert(rangeTable.size() == 0);
	rangeTable.resize(values.size());

	for (size_t i = 0; i < rangeTable.size(); i++) {
		Scalar value = values[i];
		std::vector<ScalarRange> &ranges = rangeTable[i];
		rayCastAlongY(outlineLoops, value, min, max, ranges);
	}
}


bool crossesOutlines(const Segment2Type &seg,
					 const LoopList &outlines) {
	for (LoopList::const_iterator loop = outlines.begin();
		 loop != outlines.end(); loop++) {

		//this is bad but it works for now
		LoopPath lp(*loop, loop->clockwise(), loop->counterClockwise());
		for (LoopPath::iterator point = lp.fromStart();
			 point != lp.end(); point++) {
			Segment2Type border = loop->segmentAfterPoint(point);
			Point2Type intersection;
			if (segmentSegmentIntersection(seg, border, intersection))
				return true;
		}
	}

	return false;
}

void Grid::gridRangesToOpenPaths(const ScalarRangeTable &rays,
								 const std::vector<Scalar> &values,
								 const axis_e axis,
								 OpenPathList &paths) const {

	std::vector<Scalar>::const_iterator value = values.begin();
	ScalarRangeTable::const_iterator ray = rays.begin();

	for (;
		 ray != rays.end() && value != values.end(); 
		 ++value, ++ray) {
		for (vector<ScalarRange>::const_iterator range = ray->begin();
			 range != ray->end(); ++range) {
			paths.push_back(OpenPath());

			OpenPath &path = paths.back();

			if (axis == X_AXIS) {
				path.appendPoint(Point2Type(range->min, *value));
				path.appendPoint(Point2Type(range->max, *value));
			} else {
				path.appendPoint(Point2Type(*value, range->min));
				path.appendPoint(Point2Type(*value, range->max));
			}
		}
	}
}


typedef map<int, int> PointMap;
typedef PointMap::iterator PointIter;


void pathsFromScalarRangesAlongAxis( const ScalarRangeTable &rays,	   // the ranges along this axis, multiple per lines
									 const std::vector<Scalar> &values, // the opposite axis values for each line
									 const axis_e axis,
									 const LoopList &outlines,
									 OpenPathList &paths)  // the output
{
	if (rays.size() == 0) return;

	PointMap points_remaining;

	//Convert ray ranges to segments and map endpoints
	vector<Point2Type> points;
	for (size_t i = 0; i < rays.size(); i++) {
		const vector<ScalarRange> &ray = rays[i];

		if (ray.size() == 0) continue;

		Scalar val = values[i];

		for (vector<ScalarRange>::const_iterator j = ray.begin();
				j != ray.end(); j++) {

			assert(j->min != j->max);

			if (axis == X_AXIS) {
				points.push_back(Point2Type(j->min, val));
				points.push_back(Point2Type(j->max, val));
			} else {
				points.push_back(Point2Type(val, j->min));
				points.push_back(Point2Type(val, j->max));
			}

			points_remaining[points.size() - 2] = points.size() - 1;
			points_remaining[points.size() - 1] = points.size() - 2;
		}
	}

	if (points.size() == 0) return;

	int endpoint = points_remaining.begin()->first;
	
	OpenPath currentPath;

	while (!points_remaining.empty()) {
		points_remaining.erase(endpoint);

		//handle last point
		if (points_remaining.empty()) break;

		Scalar closest_dist = numeric_limits<int>::max();
		int closest;

		//find the remaining point closest to this point
		for (PointIter close_i = points_remaining.begin();
				close_i != points_remaining.end(); close_i++) {

			int close = close_i->first;

			Scalar dist = Segment2Type(points[endpoint], points[close])
					.squaredLength();

			if (dist < closest_dist) {
				closest = close;
				closest_dist = dist;
			}
		}


		if (crossesOutlines(Segment2Type(points[endpoint], points[closest]),
							outlines)) {
			if(currentPath.size() > 1)
				paths.push_back(currentPath);
			currentPath.clear();
		}

		int connected = points_remaining[closest];
		currentPath.appendPoint(points[closest]);
		currentPath.appendPoint(points[connected]);
		endpoint = points_remaining[closest];
		points_remaining.erase(closest);
	}
	if(currentPath.size() > 1) 
		paths.push_back(currentPath);

}

bool intersectRange(Scalar a, Scalar b, Scalar c,
		Scalar d, Scalar &begin, Scalar &end) {
	assert(b >= a);
	assert(d >= c);

	if (a >= d) {
		return false;
	}
	if (c >= b) {
		return false;
	}
	if (a >= c) {
		begin = a;
	} else {
		begin = c;
	}
	if (b >= d) {
		end = d;
	} else {
		end = b;
	}
	return true;
}

vector< ScalarRange >::const_iterator subRangeTersect(
		const ScalarRange &range,
		vector< ScalarRange >::const_iterator it,
		vector< ScalarRange >::const_iterator itEnd,
		vector< ScalarRange > &result) {

	while (it != itEnd) {
		const ScalarRange &currentRange = *it;
		if ((currentRange.min >= range.max)) {
			// cout << " subrange done" << endl; // << currentRange << endl;
			return it;
		}

		ScalarRange intersection;
		// cout << " second="<< currentRange << endl;
		if (intersectRange(range.min, range.max, currentRange.min, currentRange.max, intersection.min, intersection.max)) {
			// cout << " Intersect: [" << range.min << ", " << range.max << "]"<< endl;
			result.push_back(intersection);
		}
		it++;
	}
	return it;
}

void rangeTersection(const vector< ScalarRange > &oneLine,
		const vector< ScalarRange > &twoLine,
		vector< ScalarRange > &boolLine) {
	//	static int toto = 0;
	//	toto ++;
	//	if(toto == 5007)
	//		cout << toto << endl;

	vector< ScalarRange >::const_iterator itOne = oneLine.begin();
	vector< ScalarRange >::const_iterator itTwo = twoLine.begin();
	while (itOne != oneLine.end()) {
		const ScalarRange &range = *itOne;
		//Log::finest << string(" range=") << range << endl;
		itTwo = subRangeTersect(range, itTwo, twoLine.end(), boolLine);
		if (itTwo == twoLine.end()) {
			itOne++;
			if (itOne != oneLine.end()) {
				if (twoLine.size() > 0) {
					const ScalarRange &lastRange = twoLine.back();
					//Log::finest << string(" lastRange= [") << lastRange.min << ", " << lastRange.max << "]" << endl;
					subRangeTersect(lastRange, itOne, oneLine.end(), boolLine);
				}
			}
			return;
		}
		itOne++;
	}
}


// return false if the ranges don't intersect
//

bool scalarRangeUnion(const ScalarRange& range0,
		const ScalarRange& range1, ScalarRange &resultRange) {
	//cout << " union( " << range0 << ", " << range1 << ")=";
	if ((range1.min > range0.max) || (range0.min > range1.max)) {
		// cout << "0" << endl;
		return false;
	}

	resultRange.min = range0.min < range1.min ? range0.min : range1.min;
	resultRange.max = range0.max > range1.max ? range0.max : range1.max;

	// cout << resultRange<<endl;
	return true;
}

// removes diffRange from srcRange. The result is put in resultRange, and srcRange is updated
// returns false if there is no resultRange

vector< ScalarRange >::const_iterator subRangeUnion(const ScalarRange &initialRange,
		vector< ScalarRange >::const_iterator it,
		vector< ScalarRange >::const_iterator itEnd,
		vector< ScalarRange > &result) {

	ScalarRange range(initialRange);
	//cout << endl <<"-- [subRangeUnion] --" << endl;
	while (it != itEnd) {
		const ScalarRange &itRange = *it;
		if ((itRange.min > range.max)) {
			// cout << " -PUSH" << range << endl;
			result.push_back(range);
			//cout << " -PUSHx " << range << endl;
			return it;
		}

		ScalarRange newRange;
		bool u = scalarRangeUnion(range, itRange, newRange);
		// cout << "scalarRangeUnion=" << u << endl;
		if (u) {
			range = newRange;
			//cout << " -RANGE_Extended =" << range << endl;
		} else {
			//cout << " -PUSH" << itRange << endl;
			result.push_back(itRange);
			//cout << " ++ " << itRange;

		}
		it++;
	}
	// cout << " -done!" <<endl;
	result.push_back(range);
	//cout << " +++ " << range << endl;

	//cout << "-- subRangeUnion end --" << endl;
	return it;
}

void rangeUnion(const vector< ScalarRange > &firstLine,
		const vector< ScalarRange > &secondLine,
		vector< ScalarRange > &unionLine) {
	vector< ScalarRange >::const_iterator itOne = firstLine.begin();
	vector< ScalarRange >::const_iterator itTwo = secondLine.begin();

	// the first line is empty... return the second one
	if (itOne == firstLine.end()) {
		unionLine = secondLine;
		return;
	}

	while (itOne != firstLine.end()) {
		const ScalarRange &range = *itOne;
		// cout << "first_range=" << range << endl;
		// check that the last range has not advanced beyond the firstLine
		if (unionLine.size() > 0) {
			ScalarRange &lastUnion = unionLine.back();
			// cout << "LAST RANGE UPDATE COMPARE: last=" << lastUnion << " range=" << range;
			if (range.min <= lastUnion.max && lastUnion.max >= range.max) {
				//cout << " !UPDATE ONLY" << endl;
				lastUnion.max = range.max;
				itOne++;
				continue;
			}
		}
		// cout << " !no update" << endl;
		if (itTwo == secondLine.end()) {
			unionLine.push_back(range);
			//cout << " + " << range << endl;
		} else {
			itTwo = subRangeUnion(range, itTwo, secondLine.end(), unionLine);
		}
		itOne++;
	}

	//cout << " rangeUnionDone" << endl;
}

bool scalarRangeDifference(const ScalarRange& diffRange,
		ScalarRange& srcRange,
		ScalarRange &resultRange) {
	//cout << srcRange << " - " << diffRange << " = ";
	// the diffRange is left of srcRange ... no result
	if (diffRange.max <= srcRange.min) {
		//cout << "0 (before)" << endl;
		return false;
	}

	// the diff covers the src
	// the src is (partially) occluded
	if (diffRange.min <= srcRange.min) {
		if (diffRange.max >= srcRange.max) {
			srcRange.min = srcRange.max;
			//cout << "0 (occlusion)" << endl;
			return false;
		}
		// else... adjust the srcRange and make it smaller
		srcRange.min = diffRange.max;
		//cout << "0 partial occlusion, leftover = " << srcRange << endl;
		return false;

	}

	// intersection of the ranges
	if ((diffRange.min >= srcRange.min)) {
		resultRange.min = srcRange.min;
		resultRange.max = diffRange.min;

		// left over on the right side
		if (diffRange.max <= srcRange.max) {
			srcRange.min = diffRange.max;
		} else {
			srcRange.min = srcRange.max;
		}
		// cout << resultRange << " (intersection!) leftover " <<  srcRange << endl;
		if (tequals(resultRange.max, resultRange.min, GRID_RANGE_TOL)) {
			return false;
		}

		return true;
	}

	// srcRange is not occluded by diffRange which
	// is right of scrRange
	// there is nothing to remove: the result is the range
	if (diffRange.min >= srcRange.max) {
		resultRange = srcRange;

		// remove srcRange so it is
		// not used twice
		srcRange.min = srcRange.max;
		// cout << resultRange << " (all in!) leftover " <<  srcRange << endl;
		return true;
	}

	cout << "PROBLEM!" << endl;
	assert(0);
	return false;
}

vector< ScalarRange >::const_iterator subRangeDifference(
		const ScalarRange &initialRange,
		vector< ScalarRange >::const_iterator it,
		vector< ScalarRange >::const_iterator itEnd,
		vector< ScalarRange > &result) {
	ScalarRange range(initialRange);
	// cout << "subRangeDifference from " << range << endl;
	while (it != itEnd) {
		const ScalarRange &itRange = *it;
		// cout << " itRange=" << itRange << endl;
		if ((itRange.min >= range.max)) {
			if (!tequals(range.min, range.max, GRID_RANGE_TOL))
				result.push_back(range);
			//cout << "subRangeDifference return" << endl;
			return it;
		}

		ScalarRange difference;
		if (scalarRangeDifference(itRange, range, difference)) {
			// cout << " PUSHx " <<  difference << endl;
			result.push_back(difference);
		}
		if (range.min >= range.max) // the leftover range has no length
		{
			// cout << "no left over" << endl;
			return it;
		}

		it++;
	}
	// add the left over (if any)
	if (range.max > range.min && !tequals(range.min, range.max, GRID_RANGE_TOL)) {
		// cout << "add_left_over =" << range << endl;
		result.push_back(range);
	}
	return it;
}

void rangeDifference(const vector< ScalarRange > &srcLine,
		const vector< ScalarRange > &delLine,
		vector< ScalarRange > &diffLine) {
	vector< ScalarRange >::const_iterator itOne = srcLine.begin();
	vector< ScalarRange >::const_iterator itTwo = delLine.begin();
	while (itOne != srcLine.end()) {
		const ScalarRange &range = *itOne;
		// cout << "src_range =" << range << endl;

		if (itTwo == delLine.end()) {
			//cout << "delLine done" << endl;
			// nothing to delete... copy source
			// cout << " PUSH " << range << endl;
			diffLine.push_back(range);

		} else {
			itTwo = subRangeDifference(range, itTwo, delLine.end(), diffLine);
		}
		itOne++;
	}
}

// computes the difference between the ranges of two layers

void rangeTableDifference(const ScalarRangeTable &src,
		const ScalarRangeTable &del,
		ScalarRangeTable &diff) {

	size_t lineCount = src.size();
	if (lineCount != del.size()) {
		size_t delSize = del.size();
		assert(lineCount == delSize);
	}
	diff.resize(lineCount);

	for (size_t i = 0; i < src.size(); i++) {
		const vector<ScalarRange> &lineRangeSrc = src[i];
		const vector<ScalarRange> &lineRangeDel = del[i];
		vector<ScalarRange> &lineRangeDiff = diff[i];

		rangeDifference(lineRangeSrc, lineRangeDel, lineRangeDiff);
	}


}

void rangeTableIntersection(const ScalarRangeTable &a,
		const ScalarRangeTable &b,
		ScalarRangeTable &result) {
	size_t lineCount = a.size();

	if (a.size() == 0) {
		result.resize(b.size());
		return;
	}
	else if (b.size() == 0) {
		result.resize(a.size());
		return;
	}
	
	assert(lineCount == b.size());
	result.resize(lineCount);

	for (size_t i = 0; i < lineCount; i++) {
		const vector<ScalarRange> &lineRange0 = a[i];
		const vector<ScalarRange> &lineRange1 = b[i];
		vector<ScalarRange> &lineRangeRes = result[i];
		// cout << "rangeTableIntersection " << i << endl;
		rangeTersection(lineRange0, lineRange1, lineRangeRes);
	}
}

void rangeTableUnion(const ScalarRangeTable &a,
		const ScalarRangeTable &b,
		ScalarRangeTable &result) {
	size_t lineCount = a.size();
	// cout << " rangeTableUnion " << lineCount << " vs " << b.size() << endl;

	if (a.size() == 0) {
		result = b;
		return;
	}
	else if (b.size() == 0) {
		result = a;
		return;
	}

	assert(lineCount == b.size());
	result.resize(lineCount);

	for (size_t i = 0; i < a.size(); i++) {
		const vector<ScalarRange> &lineRange0 = a[i];
		const vector<ScalarRange> &lineRange1 = b[i];
		vector<ScalarRange> &lineRangeRes = result[i];
		if (lineRangeRes.size() > 0) {
			assert(0);
		}
		rangeUnion(lineRange0, lineRange1, lineRangeRes);
	}
}

// Grid class implementation

Grid::Grid() {
}

Grid::Grid(const Limits &limits, Scalar gridSpacing) {
	init(limits, gridSpacing);
}

void Grid::init(const Limits &limits, Scalar gridSpacing) {

	Scalar deltaY = limits.yMax - limits.yMin;
	Scalar deltaX = limits.xMax - limits.xMin;

	gridOrigin[0] = limits.xMin + 0.5 * deltaX;
	gridOrigin[1] = limits.yMin + 0.5 * deltaY;

	// round to nearest odd number
	unsigned int yGridSize = (unsigned int) (deltaY / gridSpacing) + 1;
	unsigned int xGridSize = (unsigned int) (deltaX / gridSpacing) + 1;

	yValues.resize(yGridSize);
	for (unsigned int i = 0; i < yGridSize; i++) {
		Scalar v = gridOrigin[1] - 0.5 * deltaY + i * gridSpacing;
		yValues[i] = v;
	}

	xValues.resize(xGridSize);
	for (unsigned int i = 0; i < xGridSize; i++) {
		Scalar v = gridOrigin[0] - 0.5 * deltaX + i * gridSpacing;
		xValues[i] = v;
	}
}

//void Grid::createGridRanges(const SegmentTable &loops,
//		GridRanges &outGridRanges) const {
//	Scalar xMin = xValues[0];
//	Scalar xMax = xValues.back();
//	castRaysOnSliceAlongX(loops, yValues, xMin, xMax, outGridRanges.xRays);
//
//	Scalar yMin = yValues[0];
//	Scalar yMax = yValues.back();
//	castRaysOnSliceAlongY(loops, xValues, yMin, yMax, outGridRanges.yRays);
//}

void Grid::createGridRanges(const std::list<Loop>& loops,
		GridRanges& outGridRanges) const {
	Scalar xMin = xValues[0];
	Scalar xMax = xValues.back();
	castRaysOnSliceAlongX(loops, yValues, xMin, xMax, outGridRanges.xRays);

	Scalar yMin = yValues[0];
	Scalar yMax = yValues.back();
	castRaysOnSliceAlongY(loops, xValues, yMin, yMax, outGridRanges.yRays);
}

void Grid::subSample(const GridRanges &gridRanges, 
		size_t skipCount, 
		GridRanges &result) const {
	assert(result.xRays.size() == 0);
	assert(result.yRays.size() == 0);

	result.xRays.resize(gridRanges.xRays.size());
	result.yRays.resize(gridRanges.yRays.size());

	for (size_t i = 0; i < gridRanges.xRays.size(); i++) {
		result.xRays[i] = gridRanges.xRays[i]; // deep copy of the ranges for the selected lines
		i += skipCount; // skip lines depending on selected infill density
	}

	for (size_t i = 0; i < gridRanges.yRays.size(); i++) {
		result.yRays[i] = gridRanges.yRays[i]; // deep copy of the ranges for the selected lines
		i += skipCount; // skip lines depending on selected infill density
	}
}

void Grid::pathsFromRanges(const GridRanges &gridRanges,
						   const LoopList &outlines,
						   const bool xDirection, OpenPathList &paths) const
{
	assert(paths.size() == 0);
	if(xDirection)
	{
		pathsFromScalarRangesAlongAxis(gridRanges.xRays, yValues, X_AXIS,
									   outlines, paths);
	}
	else
	{
		pathsFromScalarRangesAlongAxis(gridRanges.yRays, xValues, Y_AXIS,
										  outlines, paths);
	}
}

void Grid::gridRangeUnion(const GridRanges& a, 
		const GridRanges &b, 
		GridRanges &result) const {
	rangeTableUnion(a.xRays, b.xRays, result.xRays);
	rangeTableUnion(a.yRays, b.yRays, result.yRays);
}

void Grid::gridRangeDifference(const GridRanges& src, 
		const GridRanges &del, 
		GridRanges &result) const {
	rangeTableDifference(src.xRays, del.xRays, result.xRays);
	rangeTableDifference(src.yRays, del.yRays, result.yRays);

}

void Grid::gridRangeIntersection(const GridRanges& a, 
		const GridRanges &b, 
		GridRanges &result) const {
	rangeTableIntersection(a.xRays, b.xRays, result.xRays);
	rangeTableIntersection(a.yRays, b.yRays, result.yRays);
}

void rangeTrim(const vector<ScalarRange> &src, 
		Scalar cutOff, vector<ScalarRange> &result) {
	assert(result.size() == 0);
	// cout << "rangeTrim" << endl;
	result.reserve(src.size());
	for (size_t i = 0; i < src.size(); i++) {
		const ScalarRange range = src[i];
		if (!tequals(range.max, range.min, cutOff)) {
			result.push_back(range);
		}

	}
	// cout << endl;
}

void rangeTableTrim(const ScalarRangeTable &src, 
		Scalar cutOff, 
		ScalarRangeTable &result) {
	//cout << "rangeTableTrim" << endl;
	assert(result.size() == 0);
	result.resize(src.size());
	for (size_t i = 0; i < src.size(); i++) {
		const vector<ScalarRange> &lineSrc = src[i];
		vector<ScalarRange> &lineTrims = result[i];
		rangeTrim(lineSrc, cutOff, lineTrims);
	}
}

void dumpRangeTable(const ScalarRangeTable &table) {
	cout << "Rays " << table.size() << ":";
	for (ScalarRangeTable::const_iterator ranges = table.begin();
		 ranges != table.end(); ranges++) {
		cout << ranges->size() << ", ";
	}

	cout << endl;
}

void Grid::trimGridRange(const GridRanges& src, 
		Scalar cutOff, 
		GridRanges &result) const {
	rangeTableTrim(src.xRays, cutOff, result.xRays);
	rangeTableTrim(src.yRays, cutOff, result.yRays);

	//	result.xRays.resize(src.xRays.size());
	//	result.yRays.resize(src.yRays.size());
	//
	//	for (size_t i = 0 ; i < src.xRays.size(); i++)
	//	{
	//		const  vector<ScalarRange>& sRanges = src.xRays[i];
	//		const  vector<ScalarRange>& dRanges = result.xRays[i];
	//		for(size_t j=0; j < ranges.size(); j ++)
	//		{
	//			ScalarRange r
	//		}
	//	}

}

}


