#ifndef GRAPH_H
#define GRAPH_H

#include "comman.h"
#include "type_que.h"

void getRegionNeighbour(Mat &regionNeighbour, const Mat &pixelRegion, const int regionCount) {

	regionNeighbour = Mat(regionCount, regionCount, CV_8UC1, Scalar(0));
	Mat neighbourPixelCount = Mat(regionCount, regionCount, CV_32SC1, Scalar(0));

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			Point nowP = Point(x,y);
			int regionIdx1 = pixelRegion.ptr<int>(y)[x];

			for (int k = 0; k < PIXEL_CONNECT; k++) {

				Point newP = nowP + dxdy[k];
				if (isOutside(newP.x, newP.y, pixelRegion.cols, pixelRegion.rows)) continue;
				int regionIdx2 = pixelRegion.ptr<int>(newP.y)[newP.x];

				if (regionIdx1 != regionIdx2) {
					int tmp1 = min(regionIdx1, regionIdx2);
					int tmp2 = max(regionIdx1, regionIdx2);
					neighbourPixelCount.ptr<int>(tmp1)[tmp2]++;
				}
			}
		}
	}

	for (int i = 0; i < regionCount; i++) {
		for (int j = i + 1; j < regionCount; j++) {
			if (neighbourPixelCount.ptr<int>(i)[j] > MIN_REGION_NEIGHBOUR) {
				regionNeighbour.ptr<uchar>(i)[j] = 1;
				regionNeighbour.ptr<uchar>(j)[i] = 1;
			} else {
				regionNeighbour.ptr<uchar>(i)[j] = 0;
				regionNeighbour.ptr<uchar>(j)[i] = 0;
			}
		}
	}

//	for (int i = 0; i < regionCount; i++) {
//		Mat tmp(pixelRegion.size(), CV_8UC3, Scalar(0));
//		for (int y = 0; y < tmp.rows; y++) {
//			for (int x = 0; x < tmp.cols; x++) {
//				int regionIdx = pixelRegion.ptr<int>(y)[x];
//				if (regionIdx == i) {
//					tmp.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//				}
//				if (regionNeighbour.ptr<uchar>(i)[regionIdx]) {
//					tmp.ptr<Vec3b>(y)[x] = Vec3b(0, 255, 0);
//				}
//			}
//		}
//		imshow("region", tmp);
//		waitKey(0);
//	}

	vector<bool> unreachableRegion(regionCount, true);
	bool unreachExist = false;
	for (int i = 0; i < regionCount; i++) {

		for (int j = 0; j < regionCount; j++) {
			if (regionNeighbour.ptr<uchar>(i)[j] == 1) {
				unreachableRegion[i] = false;
				break;
			}
		}
		if (unreachableRegion[i]) unreachExist = true;
	}

	if (!unreachExist) return;

	for (int y = 0; y < pixelRegion.rows; y++) {
		for (int x = 0; x < pixelRegion.cols; x++) {

			Point nowP = Point(x,y);
			int regionIdx1 = pixelRegion.ptr<int>(y)[x];

			if (!unreachableRegion[regionIdx1]) continue;

			for (int k = 0; k < PIXEL_CONNECT; k++) {

				Point newP = nowP + dxdy[k];
				if (isOutside(newP.x, newP.y, pixelRegion.cols, pixelRegion.rows)) continue;
				int regionIdx2 = pixelRegion.ptr<int>(newP.y)[newP.x];

				if (regionIdx1 != regionIdx2) {
					regionNeighbour.ptr<uchar>(regionIdx1)[regionIdx2] = 1;
					regionNeighbour.ptr<uchar>(regionIdx2)[regionIdx1] = 1;
				}
			}
		}
	}
}

void getRegionRelation(Mat &relation, const Mat &regionNeighbour, const Mat &tmpc,
					   const Mat &pixelRegion, const int regionCount) {

	TypeQue<int> &que = *(new TypeQue<int>);

	relation = Mat(regionCount, regionCount, CV_32SC1, Scalar(0));
	for (int i = 0; i < regionCount; i++) {

		int *reach = new int[regionCount];
		int *layer = new int[regionCount];
		int *max_layer = new int[regionCount];
		int *min_layer = new int[regionCount];
		memset(reach, 0, sizeof(int)*regionCount);
		memset(layer, 0, sizeof(int)*regionCount);
		memset(max_layer, 0, sizeof(int)*regionCount);
		memset(min_layer, 0, sizeof(int)*regionCount);

		que.clear();
		que.push(i);
		reach[i] = 1;

		while (!que.empty()) {

			int idx = que.front();
			que.pop();
			layer[idx] /= reach[idx];
			relation.ptr<int>(i)[idx] /= reach[idx];
			reach[idx] = -1;

			for (int j = 0; j < regionCount; j++) {

				if (reach[j] == -1) continue;
				if (regionNeighbour.ptr<uchar>(idx)[j] == 0) continue;

				int tmp = layer[idx] + tmpc.ptr<int>(idx)[j];
				layer[j] += tmp;
				max_layer[j] = max(max_layer[idx], tmp);
				min_layer[j] = min(min_layer[idx], tmp);
				relation.ptr<int>(i)[j] += abs(max_layer[j] - min_layer[j]);
				//relation.ptr<int>(i)[j] += tmp;

				if (reach[j] == 0) que.push(j);
				reach[j]++;
			}
		}

		delete[] reach;
		delete[] layer;
		delete[] max_layer;
		delete[] min_layer;

	}

	delete &que;
}

void buildRegionGraph(Mat &W, const Mat *pyramidRegion, const vector< vector<int> > *pyramidMap,
					  const vector<Vec3b> &regionColor, const double GAMA, const double PARAM1, const double PARAM2) {

	int baseRegionCount = pyramidMap[0].size();
	W = Mat(baseRegionCount, baseRegionCount, CV_64FC1, Scalar(0));
	Mat c(baseRegionCount, baseRegionCount, CV_32SC1, Scalar(0));

	int *baseRegionElementCount = new int[baseRegionCount];
	vector<Point> *baseRegionElement = new vector<Point>[baseRegionCount];
	for (int i = 0; i < baseRegionCount; i++) {
		baseRegionElementCount[i] = 0;
		baseRegionElement[i].clear();
	}
	getRegionElement(baseRegionElement, baseRegionElementCount, pyramidRegion[0]);

//	Mat baseRegionNeighbour;
//	getRegionNeighbour(baseRegionNeighbour, pyramidRegion[0], baseRegionCount);

//	// update c
//	for (int pyramidIdx = 0; pyramidIdx < PYRAMID_SIZE; pyramidIdx++) {

//		int regionCount = pyramidMap[pyramidIdx].size();

//		int *regionElementCount = new int[regionCount];
//		vector<Point> *regionElement = new vector<Point>[regionCount];
//		for (int i = 0; i < regionCount; i++) {
//			regionElementCount[i] = 0;
//			regionElement[i].clear();
//		}
//		getRegionElement(regionElement, regionElementCount, pyramidRegion[pyramidIdx]);

//		Mat regionOverlap(regionCount, regionCount, CV_32SC1, Scalar(0));
//		Mat baseRegionOverlap(regionCount, baseRegionCount, CV_32SC1, Scalar(0));

//		for ( int i = 0; i < regionCount; i++ ) {

//			vector<Point> regionBound;
//			convexHull( regionElement[i], regionBound );

//			vector<Point> horizontalBound;
//			getHorizontalBound(horizontalBound, regionBound);

//			getOverlap(regionOverlap, baseRegionOverlap, i, pyramidRegion[pyramidIdx], pyramidRegion[0], horizontalBound);
//		}

////		for (int i = 0; i < regionCount; i++) {
////			Mat tmp(pyramidRegion[0].size(), CV_8UC1, Scalar(0));
////			for (int y = 0; y < tmp.rows; y++) {
////				for (int x = 0; x < tmp.cols; x++) {
////					if (pyramidRegion[pyramidIdx].ptr<int>(y)[x] == i) {
////						tmp.ptr<uchar>(y)[x] = 255;
////					}
////				}
////			}

////			imshow("region", tmp);
////			waitKey(0);
////		}

//		Mat tmpc(regionCount, regionCount, CV_32SC1, Scalar(0));

//		for (int i = 0; i < regionCount; i++) {

//			for (int j = i + 1; j < regionCount; j++) {

//				double overlap0 = (double)regionOverlap.ptr<int>(i)[j] / regionElementCount[j];
//				double overlap1 = (double)regionOverlap.ptr<int>(j)[i] / regionElementCount[i];
//				int regionRelation = getCoveringValue(overlap0, overlap1);

//				switch (regionRelation) {
//				case -1:
//					tmpc.ptr<int>(i)[j] = -1;
//					break;
//				case 1:
//					tmpc.ptr<int>(i)[j] = 1;
//					break;
//				case 0:
//				case -2:
//					break;
//				}
//			}
//		}

//		// increase neighbour region c
//		for (int i = 0; i < regionCount; i++) {

//			vector<int> convexhullRegion;
//			int *baseRegionMap = new int[baseRegionCount];

//			for (int j = 0; j < regionCount; j++) {
//				for (size_t k = 0; k < pyramidMap[pyramidIdx][j].size(); k++) {
//					baseRegionMap[pyramidMap[pyramidIdx][j][k]] = j;
//				}
//			}

//			for (int j = 0; j < baseRegionCount; j++) {

//				if (tmpc.ptr<int>(i)[baseRegionMap[j]] < 1) continue;
//				if ((double)baseRegionOverlap.ptr<int>(i)[j] / baseRegionElementCount[j] > MIN_REGION_CONNECTED) {
//					convexhullRegion.push_back(j);
//				}
//			}

//			delete[] baseRegionMap;

//			int *replaceByIdx = new int[baseRegionCount];
//			for (int j = 0; j < baseRegionCount; j++) replaceByIdx[j] = j;

//			for (size_t j = 0; j < convexhullRegion.size(); j++) {
//				for (size_t k = j + 1; k < convexhullRegion.size(); k++) {

//					int regionIdx1 = convexhullRegion[j];
//					int regionIdx2 = convexhullRegion[k];

//					if (baseRegionNeighbour.ptr<uchar>(regionIdx1)[regionIdx2] == 0) continue;

//					int pa1 = getElementHead(regionIdx1, replaceByIdx);
//					int pa2 = getElementHead(regionIdx2, replaceByIdx);
//					replaceByIdx[pa1] = pa2;

//				}
//			}

//			for (size_t j = 0; j < convexhullRegion.size(); j++) {
//				for (size_t k = j + 1; k < convexhullRegion.size(); k++) {
//					int regionIdx1 = getElementHead(convexhullRegion[j], replaceByIdx);
//					int regionIdx2 = getElementHead(convexhullRegion[k], replaceByIdx);

//					if (regionIdx1 != regionIdx2) continue;
//					c.ptr<int>(convexhullRegion[j])[convexhullRegion[k]]++;
//					c.ptr<int>(convexhullRegion[k])[convexhullRegion[j]]++;
//				}
//			}

//			delete[] replaceByIdx;
//		}

//#ifdef DEBUG_DETAIL
//		for (int i = 0; i < baseRegionCount; i++) {
//			if (pyramidIdx < 4) break;
//			cout << "region " << i << endl;
//			Mat cc(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
//			int max_c = 0;
//			int min_c = INF;
//			for (int j = 0; j < baseRegionCount; j++) {
//				max_c = max(max_c, c.ptr<int>(i)[j]);
//				min_c = min(min_c, c.ptr<int>(i)[j]);
//			}

//			for (int y = 0; y < cc.rows; y++) {
//				for (int x = 0; x < cc.cols; x++) {
//					int regionIdx = pyramidRegion[0].ptr<int>(y)[x];
//					if (regionIdx == i) {
//						cc.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//					} else {
//						if (c.ptr<int>(i)[regionIdx] > 0) {
//							cc.ptr<Vec3b>(y)[x] = Vec3b(0, (double)(c.ptr<int>(i)[regionIdx]) / (max_c) * 255, 0);
//						} else {
//							cc.ptr<Vec3b>(y)[x] = Vec3b(0, 0, (double)(c.ptr<int>(i)[regionIdx]) / (min_c) * 255);
//						}
//					}
//				}
//			}
//			imshow("c", cc);
//			waitKey(0);
//		}
//#endif

//		// increase inside regions c
//		for (int i = 0; i < regionCount; i++) {
//			for (size_t j = 0; j < pyramidMap[pyramidIdx][i].size(); j++) {
//				for (size_t k = j + 1; k < pyramidMap[pyramidIdx][i].size(); k++) {

//					c.ptr<int>(pyramidMap[pyramidIdx][i][j])[pyramidMap[pyramidIdx][i][k]]++;
//					c.ptr<int>(pyramidMap[pyramidIdx][i][k])[pyramidMap[pyramidIdx][i][j]]++;
//				}
//			}
//		}

//#ifdef DEBUG_DETAIL
//		for (int i = 0; i < baseRegionCount; i++) {
//			if (pyramidIdx < 4) break;
//			Mat cc(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
//			int max_c = 0;
//			int min_c = INF;
//			for (int j = 0; j < baseRegionCount; j++) {
//				max_c = max(max_c, c.ptr<int>(i)[j]);
//				min_c = min(min_c, c.ptr<int>(i)[j]);
//			}

//			for (int y = 0; y < cc.rows; y++) {
//				for (int x = 0; x < cc.cols; x++) {
//					int regionIdx = pyramidRegion[0].ptr<int>(y)[x];
//					if (regionIdx == i) {
//						cc.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//					} else {
//						if (c.ptr<int>(i)[regionIdx] > 0) {
//							cc.ptr<Vec3b>(y)[x] = Vec3b(0, (double)(c.ptr<int>(i)[regionIdx]) / (max_c) * 255, 0);
//						} else {
//							cc.ptr<Vec3b>(y)[x] = Vec3b(0, 0, (double)(c.ptr<int>(i)[regionIdx]) / (min_c) * 255);
//						}
//					}
//				}
//			}
//			imshow("c", cc);
//			waitKey(0);
//		}
//#endif
//		//cout << c << endl;

//		// get region relation
//		Mat regionNeighbour;
//		getRegionNeighbour(regionNeighbour, pyramidRegion[pyramidIdx], regionCount);

//		Mat regionRelation;
//		getRegionRelation(regionRelation, regionNeighbour, tmpc, pyramidRegion[pyramidIdx], regionCount);
//		//cout << regionRelation << endl;

//		// decrease outside regions c
//		for (int i = 0; i < regionCount; i++) {
//			for (int j = i + 1; j < regionCount; j++) {

//				int r = regionRelation.ptr<int>(i)[j];
//				if (r == 0) continue;

//				for (size_t mapi_ele = 0; mapi_ele < pyramidMap[pyramidIdx][i].size(); mapi_ele++) {
//					for (size_t mapj_ele = 0; mapj_ele < pyramidMap[pyramidIdx][j].size(); mapj_ele++) {
//						c.ptr<int>(pyramidMap[pyramidIdx][i][mapi_ele])[pyramidMap[pyramidIdx][j][mapj_ele]] -= r;
//						c.ptr<int>(pyramidMap[pyramidIdx][j][mapj_ele])[pyramidMap[pyramidIdx][i][mapi_ele]] -= r;
//					}
//				}
//			}
//		}

//#ifdef DEBUG_DETAIL
//		for (int i = 0; i < baseRegionCount; i++) {
//			if (pyramidIdx < 4) break;
//			Mat cc(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
//			int max_c = 0;
//			int min_c = INF;
//			for (int j = 0; j < baseRegionCount; j++) {
//				max_c = max(max_c, c.ptr<int>(i)[j]);
//				min_c = min(min_c, c.ptr<int>(i)[j]);
//			}

//			for (int y = 0; y < cc.rows; y++) {
//				for (int x = 0; x < cc.cols; x++) {
//					int regionIdx = pyramidRegion[0].ptr<int>(y)[x];
//					if (regionIdx == i) {
//						cc.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
//					} else {
//						if (c.ptr<int>(i)[regionIdx] > 0) {
//							cc.ptr<Vec3b>(y)[x] = Vec3b(0, (double)(c.ptr<int>(i)[regionIdx]) / (max_c) * 255, 0);
//						} else {
//							cc.ptr<Vec3b>(y)[x] = Vec3b(0, 0, (double)(c.ptr<int>(i)[regionIdx]) / (min_c) * 255);
//						}
//					}
//				}
//			}
//			imshow("c", cc);
//			waitKey(0);
//		}
//#endif

//		//cout << c << endl;

//		delete[] regionElement;
//		delete[] regionElementCount;

//	}

//	for (int i = 0; i < baseRegionCount; i++) {
//		for (int j = i + 1; j < baseRegionCount; j++) {
//			c.ptr<int>(i)[j] = (c.ptr<int>(i)[j] + c.ptr<int>(j)[i]) >> 1;
//			c.ptr<int>(j)[i] = c.ptr<int>(i)[j];
//		}
//	}

	// region saliency


	// init W
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			double w = pow(e, -(double)colorDiff(regionColor[i], regionColor[j]) / (8));
			//cout << w << " " << colorDiff(regionColor[i], regionColor[j]) << endl;
			W.ptr<double>(i)[j] = w;
		}
	}

	//cout << W << endl;

	// update W with dist
	Mat regionDist;
	getRegionDist(regionDist, pyramidRegion[0], baseRegionCount);

	int width = pyramidRegion[0].cols;
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			double d = pow(e, -(double)regionDist.ptr<int>(i)[j] / (width/5));
			//cout << d << endl;
			W.ptr<double>(i)[j] *= d;
		}
	}

	// update W with size
	double sizeSigma = 0;
	for (int i = 0; i < baseRegionCount; i++) sizeSigma += baseRegionElementCount[i];
	sizeSigma /= baseRegionCount;
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			//double size = pow(e, -(double)sizeSigma / sqrt(baseRegionElementCount[i]*baseRegionElementCount[j]));
			double size = baseRegionElementCount[i] + baseRegionElementCount[j];
			//cout << baseRegionElementCount[i] << " " << baseRegionElementCount[j] << " " << size << endl;
			W.ptr<double>(i)[j] *= size;
		}
	}

	// update W with image center
	int *regionCenterBias = new int[baseRegionCount];
	memset(regionCenterBias, 0, sizeof(int)*baseRegionCount);
	int mid_x = pyramidRegion[0].cols / 2;
	int mid_y = pyramidRegion[0].rows / 2;
	for (int y = 0; y < pyramidRegion[0].rows; y++) {
		for (int x = 0; x < pyramidRegion[0].cols; x++) {

			int regionIdx = pyramidRegion[0].ptr<int>(y)[x];
			regionCenterBias[regionIdx] += abs(mid_x - x) + abs(mid_y - y);
		}
	}

	for (int i = 0; i < baseRegionCount; i++) regionCenterBias[i] /= baseRegionElementCount[i];

	for (int i = 0; i < baseRegionCount; i++) {
		//cout << regionCenterBias[i] << endl;
		for (int j = i + 1; j < baseRegionCount; j++) {
			double centerBias = pow(e, -(double)(abs(regionCenterBias[i]-regionCenterBias[j])) / 100);
			//cout << centerBias << endl;
			//W.ptr<double>(i)[j] *= centerBias;
		}
	}

	delete[] regionCenterBias;

	delete[] baseRegionElement;
	delete[] baseRegionElementCount;

	//Mat W0 = W.clone();

	// update W with c
	for (int i = 0; i < baseRegionCount; i++) {
		for (int j = i + 1; j < baseRegionCount; j++) {
			//W.ptr<double>(i)[j] = W.ptr<double>(i)[j] * pow(GAMA, c.ptr<int>(i)[j]);
			//W.ptr<double>(i)[j] = pow(GAMA, c.ptr<int>(i)[j]);
			W.ptr<double>(j)[i] = W.ptr<double>(i)[j];
		}
	}

#ifdef DEBUG
	for (int i = 0; i < baseRegionCount; i++) {
		Mat tmp(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
		Mat cc(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
		Mat wshow(pyramidRegion[0].size(), CV_8UC3, Scalar(0));
		double max_w = 0;
		double max_w0 = 0;
		int max_c = 0;
		int min_c = INF;
		for (int j = 0; j < baseRegionCount; j++) {
			max_w = max(max_w, W.ptr<double>(i)[j]);
			max_w0 = max(max_w0, W0.ptr<double>(i)[j]);
			max_c = max(max_c, c.ptr<int>(i)[j]);
			min_c = min(min_c, c.ptr<int>(i)[j]);
		}

		cout << min_c << " " << max_c << endl;
		//cout << regionCenterBias[i] << endl;

		for (int y = 0; y < tmp.rows; y++) {
			for (int x = 0; x < tmp.cols; x++) {
				int regionIdx = pyramidRegion[0].ptr<int>(y)[x];
				if (regionIdx == i) {
					tmp.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
					wshow.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
					cc.ptr<Vec3b>(y)[x] = Vec3b(255, 0, 0);
				} else {
					tmp.ptr<Vec3b>(y)[x] = Vec3b(0, max(W.ptr<double>(i)[regionIdx],W.ptr<double>(regionIdx)[i]) / max_w * 255, 0);
					wshow.ptr<Vec3b>(y)[x] = Vec3b(0, max(W0.ptr<double>(i)[regionIdx],W0.ptr<double>(regionIdx)[i]) / max_w0 * 255, 0);
					if (c.ptr<int>(i)[regionIdx] > 0) {
						cc.ptr<Vec3b>(y)[x] = Vec3b(0, (double)(c.ptr<int>(i)[regionIdx]) / (max_c) * 255, 0);
					} else {
						cc.ptr<Vec3b>(y)[x] = Vec3b(0, 0, (double)(c.ptr<int>(i)[regionIdx]) / (min_c) * 255);
					}
				}
			}
		}
		imshow("W0", wshow);
		imshow("W", tmp);
		imshow("c", cc);
		waitKey(0);
	}
#endif

	//cout << W << endl;


}

#endif // GRAPH_H
