#include <list>
#include <limits.h>
#include <string>

#include "pather_optimizer.h"

namespace mgl {

Scalar pather_optimizer::DISTANCE_THRESHOLD = 0.2;

void pather_optimizer::addPath(const OpenPath& path, const PathLabel& label) {
	//we don't consider paths of a single point
	if(path.size() > 1) {
		myPaths.push_back(LabeledOpenPath(label, path));
	} else {
		Exception mixup("Attempted to add degenerate path to optimizer");
		throw mixup;
	}
}

void pather_optimizer::addPath(const Loop& loop, const PathLabel& label) {
	//we only consider loops that are polygons, not lines or points
	if(loop.size() > 2) {
		myLoops.push_back(LabeledLoop(label, loop));
	} else {
		Exception mixup("Attempted to add degenerate loop to optimizer");
		throw mixup;
	}
}

void pather_optimizer::addBoundary(const OpenPath& path) {
	//boundaries are broken down into linesegments
	if(path.size() > 1) {
		for(OpenPath::const_iterator iter = path.fromStart(); 
				iter != path.end(); 
				++iter) {
			boundaries.push_back(path.segmentAfterPoint(iter));
		}
	} else {
		Exception mixup("Attempted to add degenerate path to optimizer boundary");
		throw mixup;
	}
}

void pather_optimizer::addBoundary(const Loop& loop) {
		//boundaries are broken down into linesegments
	if(loop.size() > 2) {
		for(Loop::const_finite_cw_iterator iter = loop.clockwiseFinite(); 
				iter != loop.clockwiseEnd(); 
				++iter) {
			boundaries.push_back(loop.segmentAfterPoint(iter));
		}
	} else {
		Exception mixup("Attempted to add degenerate loop to optimizer boundary");
		throw mixup;
	}
}

void pather_optimizer::clearBoundaries() {
	boundaries.clear();
}

void pather_optimizer::clearPaths() {
	myLoops.clear();
	myPaths.clear();
}

void pather_optimizer::optimizeInternal(
		abstract_optimizer::LabeledOpenPaths& labeledpaths) {
	Point2Type lastPoint;
	LabeledOpenPath currentClosest;
	if(!myLoops.empty())
		lastPoint = *(myLoops.begin()->myPath.entryBegin());
	else if(!myPaths.empty())
		lastPoint = *(myPaths.begin()->myPath.entryBegin());
	while(!myLoops.empty() || !myPaths.empty()) {
		try {
			while(closest(lastPoint, currentClosest)) {
				lastPoint = *(currentClosest.myPath.fromEnd());
				labeledpaths.push_back(currentClosest);
			}
		} catch(Exception mixup) {
            if(jsonErrors) {
                    exceptionToJson(Log::severe(), mixup, true);
            } else {
                Log::severe() << "WARNING: " << mixup.what() << std::endl;
            }
		}
	}
	//if moves don't cross boundaries, ok to extrude them
	if(!boundaries.empty()) {
		link(labeledpaths);
	}
}

LabeledOpenPath pather_optimizer::closestLoop( 
		LabeledLoopList::iterator loopIter, 
		Loop::entry_iterator entryIter) {
	//we have found a closest loop
	LabeledOpenPath retLabeled;
	Loop::cw_iterator cwIter = loopIter->myPath.clockwise(*entryIter);
	Loop::ccw_iterator ccwIter = loopIter->myPath.counterClockwise(*entryIter);
	if(cwIter == loopIter->myPath.clockwiseEnd() || 
			ccwIter == loopIter->myPath.counterClockwiseEnd()) {
		std::stringstream msg;
		msg << "Invalid entryIterator in pather_optimizer::closestLoop" 
				<< std::endl;
		Exception mixup(msg.str());
		throw mixup;
	}
	LoopPath lp(loopIter->myPath, 
			cwIter, 
			ccwIter);
	//turn into into a path
	retLabeled.myPath.appendPoints(lp.fromStart(), lp.end());
	retLabeled.myLabel = loopIter->myLabel;
	if(retLabeled.myPath.size() < 3) {
		std::stringstream msg;
		msg << "Degenerate path of size " <<retLabeled.myPath.size() << 
				" from loop of size " << loopIter->myPath.size() << std::endl;
		Exception mixup(msg.str());
		myLoops.erase(loopIter);
		throw mixup;
	}
	//remove it from our "things to optimize"
	myLoops.erase(loopIter);
	return retLabeled;
}

LabeledOpenPath pather_optimizer::closestPath( 
		LabeledPathList::iterator pathIter, 
		OpenPath::entry_iterator entryIter) {
	//we have found a closest path
	LabeledOpenPath retLabeled;
	//extract it in the right order
	if(*entryIter == *(pathIter->myPath.fromStart())) {
		retLabeled.myPath = pathIter->myPath;
	} else {
		retLabeled.myPath.appendPoints(
				pathIter->myPath.fromEnd(), pathIter->myPath.rend());
	}
	retLabeled.myLabel = pathIter->myLabel;
	if(retLabeled.myPath.size() < 2) {
		std::stringstream msg;
		msg << "Degenerate path of size " <<retLabeled.myPath.size() << 
				" from path of size " << pathIter->myPath.size() << std::endl;
		Exception mixup(msg.str().c_str());
		myPaths.erase(pathIter);
		throw mixup;
	}
	//remove it from our "things to optimize"
	myPaths.erase(pathIter);
	return retLabeled;
}

void pather_optimizer::findClosestLoop(const Point2Type& point, 
		LabeledLoopList::iterator& loopIter, 
		Loop::entry_iterator& entryIter) {
	
	if(myLoops.empty()) {
		loopIter = myLoops.end();
		return;
	}
	loopIter = myLoops.begin();
	entryIter = loopIter->myPath.entryBegin();
	Scalar closestDistance = (point - *entryIter).magnitude();
	int closestValue = loopIter->myLabel.myValue;
	
	for(LabeledLoopList::iterator currentIter = myLoops.begin(); 
			currentIter != myLoops.end(); 
			++currentIter) {
		for(Loop::entry_iterator currentEntry = 
				currentIter->myPath.entryBegin(); 
				currentEntry != currentIter->myPath.entryEnd(); 
				++currentEntry) {
			Scalar distance = (point - 
					*(currentEntry)).magnitude();
			int value = currentIter->myLabel.myValue;
			if(value > closestValue || 
					(value == closestValue && 
					tlower(distance, closestDistance, 
					DISTANCE_THRESHOLD))) {
				closestDistance = distance;
				entryIter = currentEntry;
				loopIter = currentIter;
				closestValue = value;
			}
		}
	}
}

void pather_optimizer::findClosestPath(const Point2Type& point, 
		LabeledPathList::iterator& pathIter, 
		OpenPath::entry_iterator& entryIter) {
	if(myPaths.empty()) {
		pathIter = myPaths.end();
		return;
	}
	pathIter = myPaths.begin();
	entryIter = pathIter->myPath.entryBegin();
	Scalar closestDistance = (point - *entryIter).magnitude();
	int closestValue = pathIter->myLabel.myValue;
	
	for(LabeledPathList::iterator currentIter = myPaths.begin(); 
			currentIter != myPaths.end(); 
			++currentIter) {
		for(OpenPath::entry_iterator currentEntry = 
				currentIter->myPath.entryBegin(); 
				currentEntry != currentIter->myPath.entryEnd(); 
				++currentEntry) {
			Scalar distance = (point - 
					*(currentEntry)).magnitude();
			int value = currentIter->myLabel.myValue;
			if(value > closestValue || 
					(value == closestValue && 
					tlower(distance, closestDistance, 
					DISTANCE_THRESHOLD))) {
				closestDistance = distance;
				entryIter = currentEntry;
				pathIter = currentIter;
			}
		}
	}
}

bool pather_optimizer::closest(const Point2Type& point, LabeledOpenPath& result) {
	LabeledLoopList::iterator loopIter;
	Loop::entry_iterator loopEntry;
	LabeledPathList::iterator pathIter;
	OpenPath::entry_iterator pathEntry;
	
	findClosestLoop(point, loopIter, loopEntry);
	findClosestPath(point, pathIter, pathEntry);
	
	int loopVal = loopIter->myLabel.myValue;
	int pathVal = pathIter->myLabel.myValue;
	
	if(loopIter != myLoops.end() && pathIter != myPaths.end()) {
		//pick best
		Scalar loopDistance = (point - *loopEntry).magnitude();
		Scalar pathDistance = (point - *pathEntry).magnitude();
		if(loopVal > pathVal || (loopVal == pathVal && 
				tlower(loopDistance, pathDistance, 
				DISTANCE_THRESHOLD))) {
			//loop wins
			result = closestLoop(loopIter, loopEntry);
		} else {
			result = closestPath(pathIter, pathEntry);
		}
	} else if(loopIter != myLoops.end()) {
		//pick loop
		result = closestLoop(loopIter, loopEntry);
	} else if(pathIter != myPaths.end()) {
		//pick path
		result = closestPath(pathIter, pathEntry);
	} else {
		return false;
	}
	return true;
}

bool pather_optimizer::crossesBoundaries(const Segment2Type& seg) {
	//test if this linesegment crosses any boundaries
	for(BoundaryList::const_iterator iter = 
			boundaries.begin(); 
			iter != boundaries.end(); 
			++iter) {
		const Segment2Type& currentBoundary = *iter;
		if(seg.intersects(currentBoundary))
			return true;
	}
	return false;
}

void pather_optimizer::link(
		abstract_optimizer::LabeledOpenPaths& labeledpaths) {
	//connect paths if between them the movement not crosses boundaries
	typedef abstract_optimizer::LabeledOpenPaths::iterator iterator;
	for(iterator iter = labeledpaths.begin(); 
			iter != labeledpaths.end(); 
			++iter) {
		iterator lastIter;
		if(iter != labeledpaths.begin()) {
			lastIter= iter;
			--lastIter;
			LabeledOpenPath& last = *lastIter;
			LabeledOpenPath& current = *iter;
			Point2Type lastPoint = *(last.myPath.fromEnd());
			Point2Type currentPoint = *(current.myPath.fromStart());
			if(current.myLabel.myType == PathLabel::TYP_CONNECTION || 
					last.myLabel.myType == PathLabel::TYP_CONNECTION)
				continue;
			if(lastPoint == currentPoint)
				continue;
			Segment2Type transition(lastPoint, currentPoint);
			if(crossesBoundaries(transition))
				continue;
			LabeledOpenPath connection;
			connection.myPath.appendPoint(lastPoint);
			connection.myPath.appendPoint(currentPoint);
			connection.myLabel.myType = PathLabel::TYP_CONNECTION;
			connection.myLabel.myOwner = PathLabel::OWN_MODEL;
			connection.myLabel.myValue = 0;
			if(last.myLabel == current.myLabel && false) {
				//naive case
				//concatenate paths of same label
				last.myPath.appendPoints(current.myPath.fromStart(), 
						current.myPath.end());
				iter = labeledpaths.erase(iter);
				--iter;
			} else {
				//smart case
				//properly identify this as connectivity
				iter = labeledpaths.insert(iter, connection);
			}
		}
	}
}

}



