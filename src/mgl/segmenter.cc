/* 
 * File:   segmenter.cc
 * Author: Dev
 * 
 * Created on June 19, 2012, 2:05 PM
 */

#include "configuration.h"
#include "segmenter.h"
#include "mgl.h"

namespace mgl{

using namespace std;


Segmenter::Segmenter(const GrueConfig& config) 
        : zTapeMeasure(config.get_firstLayerZ(), 
        config.get_layerH(), config.get_layerWidthRatio()) {}
const SliceTable& Segmenter::readSliceTable() const{
	return sliceTable;
}
const LayerMeasure& Segmenter::readLayerMeasure() const{
	return zTapeMeasure;
}
const vector<Triangle3Type>& Segmenter::readAllTriangles() const{
	return allTriangles;
}
const Limits& Segmenter::readLimits() const{
	return limits;
}

void Segmenter::tablaturize(const Meshy& mesh){
	allTriangles = mesh.readAllTriangles();
	limits = mesh.readLimits();
	for(size_t i=0; i<allTriangles.size(); ++i)
		updateSlicesTriangle(i);
}
void Segmenter::updateSlicesTriangle(size_t newTriangleId){
	Triangle3Type t = allTriangles[newTriangleId];
	
	Point3Type a, b, c;
	t.zSort(a, b, c);

	unsigned int minSliceIndex = this->zTapeMeasure.zToLayerAbove(a.z);
	if (minSliceIndex > 0)
		minSliceIndex--;

	unsigned int maxSliceIndex = this->zTapeMeasure.zToLayerAbove(c.z);
	if (maxSliceIndex - minSliceIndex > 1)
		maxSliceIndex--;

	//		Log::often() << "Min max index = [" <<  minSliceIndex << ", "<< maxSliceIndex << "]"<< std::endl;
	//		Log::often() << "Max index =" <<  maxSliceIndex << std::endl;
	unsigned int currentSliceCount = sliceTable.size();
	if (maxSliceIndex >= currentSliceCount) {
		unsigned int newSize = maxSliceIndex + 1;
		sliceTable.resize(newSize); // make room for potentially new slices
		//			Log::often() << "- new slice count: " << sliceTable.size() << std::endl;
	}

	//		 Log::often() << "adding triangle " << newTriangleId << " to layer " << minSliceIndex  << " to " << maxSliceIndex << std::endl;
	for (size_t i = minSliceIndex; i <= maxSliceIndex; i++) {
		TriangleIndices &trianglesForSlice = sliceTable[i];
		trianglesForSlice.push_back(newTriangleId);
		//			Log::often() << "   !adding triangle " << newTriangleId << " to layer " << i  << " (size = " << trianglesForSlice.size() << ")" << std::endl;
	}
}



}

