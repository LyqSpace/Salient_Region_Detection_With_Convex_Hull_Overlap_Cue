#include "segment.h"

float getMinIntDiff(int SEGMENT_THRESHOLD, int regionSize) {
	return (float)SEGMENT_THRESHOLD / regionSize;
}

int Point2Index(Point u, int width) {
	return u.y * width + u.x;
}

void overSegmentation(Mat &pixelRegion, int &regionCount, const Mat &LABImg) {

	vector<TypeEdge> edges;
	Size imgSize = LABImg.size();
	pixelRegion = Mat::zeros( imgSize, CV_32SC1 );
	const int leftside[4] = {2, 3, 5, 7};

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			Point nowP = Point(x, y);

			for (int k = 0; k < 4; k++) {

				Point newP = nowP + dxdy[leftside[k]];
				if (isOutside(newP.x, newP.y, imgSize.width, imgSize.height)) continue;

				Vec3f nowColor = LABImg.ptr<Vec3f>(nowP.y)[nowP.x];
				Vec3f newColor = LABImg.ptr<Vec3f>(newP.y)[newP.x];
				double diff = colorDiff(nowColor, newColor);
				edges.push_back(TypeEdge(nowP, newP, diff));
			}
		}
	}

	sort(edges.begin(), edges.end(), cmpTypeEdge);

	int pixelCount = imgSize.height * imgSize.width;
	int *regionHead = new int[pixelCount];
	int *regionSize = new int[pixelCount];
	double *minIntDiff = new double[pixelCount];
	for (int i = 0; i < pixelCount; i++) {
		regionHead[i] = i;
		regionSize[i] = 1;
		minIntDiff[i] = getMinIntDiff(SEGMENT_THRESHOLD, 1);
	}

	for (size_t i = 0; i < edges.size(); i++) {

		int uIdx = Point2Index(edges[i].u, imgSize.width);
		int vIdx = Point2Index(edges[i].v, imgSize.width);
		int pu = getElementHead(uIdx, regionHead);
		int pv = getElementHead(vIdx, regionHead);
		if (pu == pv) continue;

		if ((edges[i].w <= minIntDiff[pu]) && (edges[i].w <= minIntDiff[pv])) {

			regionHead[pv] = pu;
			regionSize[pu] += regionSize[pv];
			minIntDiff[pu] = edges[i].w + getMinIntDiff(SEGMENT_THRESHOLD, regionSize[pu]);
		}
	}

	delete[] minIntDiff;

	for (size_t i = 0; i < edges.size(); i++) {

		int uIdx = Point2Index(edges[i].u, imgSize.width);
		int vIdx = Point2Index(edges[i].v, imgSize.width);
		int pu = getElementHead(uIdx, regionHead);
		int pv = getElementHead(vIdx, regionHead);
		if (pu == pv) continue;

		if ((regionSize[pu] < MIN_REGION_SIZE) || (regionSize[pv] < MIN_REGION_SIZE)) {
			regionHead[pv] = pu;
			regionSize[pu] += regionSize[pv];
		}
	}

	// get main region
	int idx = 0;
	regionCount = 0;

	int *regionIndex = new int[pixelCount];
	for (int i = 0; i < pixelCount; i++) regionIndex[i] = -1;

	for (int y = 0; y < imgSize.height; y++) {
		for (int x = 0; x < imgSize.width; x++) {

			int pIdx = getElementHead(idx, regionHead);
			if (regionIndex[pIdx] == -1) {
				pixelRegion.ptr<int>(y)[x] = regionCount;
				regionIndex[pIdx] = regionCount++;
			} else {
				pixelRegion.ptr<int>(y)[x] = regionIndex[pIdx];
			}
			idx++;
		}
	}

	delete[] regionHead;
	delete[] regionIndex;
	delete[] regionSize;

	//cout << regionCount << endl;

#ifdef SHOW_IMAGE
	writeRegionImageRandom(regionCount, pixelRegion, "debug_output/Segment_Image.png", 1, 1);
#endif

}

void segmentImage(Mat &W, Mat &pixelRegion, int &regionCount, const Mat &LABImg) {

	regionCount = 0;

	overSegmentation(pixelRegion, regionCount, LABImg);

	// get region represent color
	vector<Vec3f> regionColor;
	getRegionColor(regionColor, regionCount, pixelRegion, LABImg);

	W = Mat(regionCount, regionCount, CV_64FC1, Scalar(0));

	// update W with size
	int *regionElementCount = new int[regionCount];
	vector<Point> *regionElement = new vector<Point>[regionCount];
	for (int i = 0; i < regionCount; i++) {
		regionElementCount[i] = 0;
		regionElement[i].clear();
	}
	getRegionElement(regionElement, regionElementCount, pixelRegion);

	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			double size = regionElementCount[i] * regionElementCount[j];
			W.ptr<double>(i)[j] = size;
		}
	}
	delete[] regionElementCount;
	delete[] regionElement;

	// update W with color
	double sigma_color = 8;
	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			double w = exp(-(double)colorDiff(regionColor[i], regionColor[j]) / sigma_color);
			W.ptr<double>(i)[j] *= w;
		}
	}

	// update W with dist
	Mat regionDist;
	getRegionDist(regionDist, pixelRegion, regionCount);

	double sigma_width = 0.4;
	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			double d = exp(-regionDist.ptr<double>(i)[j] / sigma_width);
			W.ptr<double>(i)[j] *= d;
		}
	}

	// symmetric
	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			if (W.ptr<double>(i)[j] < FLOAT_EPS) W.ptr<double>(i)[j] = 0;
			W.ptr<double>(j)[i] = W.ptr<double>(i)[j];
		}
	}

}