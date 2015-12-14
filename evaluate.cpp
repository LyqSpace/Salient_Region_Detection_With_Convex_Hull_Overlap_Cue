#include "evaluate.h"

void getGroundTruth(map<string,Mat> &binaryMask, const char *dirName) {

	cout << "Ground Truth Input ..." << endl;
	binaryMask.clear();

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

	DIR *testDir = opendir(dirName);
	dirent *testFile;

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;

		char inputImgName[100];
		sprintf(inputImgName, fileNameFormat, testFile->d_name);

		string str(testFile->d_name);
		Mat maskMat = imread(inputImgName, 0);
		maskMat = maskMat(Rect(CROP_WIDTH, CROP_WIDTH, maskMat.cols-2*CROP_WIDTH, maskMat.rows-2*CROP_WIDTH));
		str = str.substr(0, str.length()-4);
		binaryMask[str] = maskMat;

	}
}

bool evaluateMap(double &precision, double &recall, const Mat &mask, const Mat &saliencyMap) {

	int area_saliency = sum(saliencyMap).val[0] / 255;
	int area_mask = sum(mask).val[0] / 255;
	int area_intersection = sum(saliencyMap & mask).val[0] / 255;

	double tmp_precision, tmp_recall;

	if (area_saliency > 0) {
		tmp_precision = (double)(area_intersection) / area_saliency;
		tmp_recall = (double)(area_intersection) / area_mask;
	} else {
		tmp_precision = 0;
		tmp_recall = 0;
	}

	precision += tmp_precision;
	recall += tmp_recall;

	if (tmp_precision < 0.8) return false;
		else return true;

}

void compMaskOthers_1K() {

	FILE *recall_precision_File = fopen("logs/recall_precision.txt", "w");

	int testNum = 0;

	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, "test/binarymask");

	for (int PARAM1 = 250; PARAM1 > 0; PARAM1 -= 5) {

		printf("%d\t%d", testNum, PARAM1);
		cout << endl;

		char dirName[100] = "Saliency";

		char fileNameFormat[100];
		memset(fileNameFormat, 0, sizeof(fileNameFormat));
		for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
		strcat(fileNameFormat, "/%s");



		DIR *testDir = opendir(dirName);
		dirent *testFile;

		map<string, double> precision, recall;
		map<string, int> methodCount;

		int c = 0;

		while ((testFile = readdir(testDir)) != NULL) {

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
			string str(testFile->d_name);
			if (str.find(".jpg") != string::npos) continue;
			int methodStr_st = str.find_last_of('_');
			int methodStr_ed = str.find_last_of('.');
			string methodStr = str.substr(methodStr_st + 1, methodStr_ed-methodStr_st-1);
			string imgId = str.substr(0, methodStr_st);

			if (methodStr != "CHO") continue;

			if (methodCount.find(methodStr) == methodCount.end()) {
				methodCount[methodStr] = 1;
				precision[methodStr] = 0;
				recall[methodStr] = 0;
			} else {
				methodCount[methodStr]++;
			}

			c++;
			//cout << c << " " << testFile->d_name << endl;

			char inputImgName[100];
			sprintf(inputImgName, fileNameFormat, testFile->d_name);

			Mat inputImg = imread(inputImgName, 0);
			Mat saliencyMap;
			if (methodStr == "CHO") {
				saliencyMap = inputImg;
			} else {
				saliencyMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH));
			}
			threshold(saliencyMap, saliencyMap, PARAM1, 255, THRESH_BINARY);

			evaluateMap(precision[methodStr], recall[methodStr], binaryMask[imgId], saliencyMap);

		}

		map<string,double>::iterator it;
		for (it = precision.begin(); it != precision.end(); it++) {

			string methodStr = it->first;
			double tmp_precision = precision[methodStr] / methodCount[methodStr];
			double tmp_recall = recall[methodStr] / methodCount[methodStr];

			fprintf(recall_precision_File, "%.4lf\t%.4lf\t", tmp_recall, tmp_precision);

			printf("%.4lf\t%.4lf\t", tmp_recall, tmp_precision);
			cout << methodStr << endl;

		}
		fprintf(recall_precision_File, "\n");
	}
	fclose(recall_precision_File);

}

void compMaskOthers_10K() {

	FILE *recall_precision_File = fopen("test/MSRA10K/recall_precision_10K_cue.txt", "w");

	int testNum = 0;

	map<string, Mat> binaryMask;
	getGroundTruth(binaryMask, "test/MSRA10K/GT");

	for (int PARAM1 = 250; PARAM1 >= -5; PARAM1 -= 5) {

		printf("%d\t%d", testNum, PARAM1);
		cout << endl;

		char dirName[100] = "test/MSRA10K/Saliency_Cue";

		char fileNameFormat[100];
		memset(fileNameFormat, 0, sizeof(fileNameFormat));
		for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
		strcat(fileNameFormat, "/%s");

		DIR *testDir = opendir(dirName);
		dirent *testFile;

		map<string, double> precision, recall;
		map<string, int> methodCount;

		int c = 0;

		while ((testFile = readdir(testDir)) != NULL) {

			if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
			string str(testFile->d_name);
			if (str.find(".jpg") != string::npos) continue;
			int methodStr_st = str.find_last_of('_');
			int methodStr_ed = str.find_last_of('.');
			string methodStr = str.substr(methodStr_st + 1, methodStr_ed-methodStr_st-1);
			string imgId = str.substr(0, methodStr_st);

//			if (methodStr != "FT" && methodStr != "RC" && methodStr != "SR" && methodStr != "CHO" &&
//				methodStr != "LC" && methodStr != "CB" && methodStr != "SEG" && methodStr != "HC") {
//				continue;
//			}

			if (methodCount.find(methodStr) == methodCount.end()) {
				methodCount[methodStr] = 1;
				precision[methodStr] = 0;
				recall[methodStr] = 0;
			} else {
				methodCount[methodStr]++;
			}

			c++;
			//cout << c << " " << testFile->d_name << endl;

			char inputImgName[100];
			sprintf(inputImgName, fileNameFormat, testFile->d_name);

			Mat inputImg = imread(inputImgName, 0);
			Mat saliencyMap;
			if (methodStr == "CHO" || methodStr == "MIX") {
				saliencyMap = inputImg;
			} else {
				saliencyMap = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH));
			}

			Mat saliencyObj;
			threshold(saliencyMap, saliencyObj, PARAM1, 255, THRESH_BINARY);

			evaluateMap(precision[methodStr], recall[methodStr], binaryMask[imgId], saliencyObj);
//			if (!flag) {
//				imwrite("test/MSRA10K/result/"+imgId+"_CHO.png", saliencyMap);
//				imwrite("test/MSRA10K/result/"+imgId+"_GT.bmp", binaryMask[imgId]);
//				Mat inputImg = imread("test/MSRA10K/input/"+imgId+".jpg");
//				imwrite("test/MSRA10K/result/"+imgId+"_input.jpg", inputImg);
//				imwrite("test/MSRA10K/negative/"+imgId+".jpg", inputImg);
//			}

		}

		map<string,double>::iterator it;
		for (it = precision.begin(); it != precision.end(); it++) {

			string methodStr = it->first;
			double tmp_precision = precision[methodStr] / methodCount[methodStr];
			double tmp_recall = recall[methodStr] / methodCount[methodStr];

			fprintf(recall_precision_File, "%.4lf\t%.4lf\t", tmp_recall, tmp_precision);

			printf("%.4lf\t%.4lf\t", tmp_recall, tmp_precision);
			cout << methodStr << endl;

		}
		fprintf(recall_precision_File, "\n");
	}
	fclose(recall_precision_File);

}

void compResults_10K() {

	int threshold_f = 200;

	char dirName[100] = "test/MSRA10K/input";

	char fileNameFormat[100];
	memset(fileNameFormat, 0, sizeof(fileNameFormat));
	for (size_t i = 0; i < strlen(dirName); i++) fileNameFormat[i] = dirName[i];
	strcat(fileNameFormat, "/%s");

	DIR *testDir = opendir(dirName);
	dirent *testFile;

	int exampleCount = 0;

	const string methodStrArr[8] = {"FT","RC","SR","CHO","LC","CB","SEG","HC"};

	while ((testFile = readdir(testDir)) != NULL) {

		if (strcmp(testFile->d_name, ".") == 0 || strcmp(testFile->d_name, "..") == 0) continue;
		string str(testFile->d_name);

		string originStr = (string)"test/MSRA10K/input/" + testFile->d_name;
		Mat originImg = imread(originStr);
		originImg = originImg(Rect(CROP_WIDTH, CROP_WIDTH, originImg.cols-2*CROP_WIDTH, originImg.rows-2*CROP_WIDTH));

		int methodStr_st = str.find_last_of('.');
		string imgId = str.substr(0, methodStr_st);

		map<string, double> precision, recall;
		map<string, Mat> saliencyMap;

		Mat binaryMask;

		for (int i = 0; i < 8; i++) {

			string methodStr = methodStrArr[i];

			string imgName = "test/MSRA10K/Saliency/" + imgId + "_" + methodStr + ".png";
			precision[methodStr] = 0;
			recall[methodStr] = 0;

			string maskName = "test/MSRA10K/GT/" + imgId + ".png";
			binaryMask = imread(maskName);
			binaryMask = binaryMask(Rect(CROP_WIDTH, CROP_WIDTH, binaryMask.cols-2*CROP_WIDTH, binaryMask.rows-2*CROP_WIDTH));


			Mat inputImg = imread(imgName);
			Mat saliencyObj;
			if (methodStr == "CHO" || methodStr == "MIX") {
				saliencyMap[methodStr] = inputImg;
			} else {
				saliencyMap[methodStr] = inputImg(Rect(CROP_WIDTH, CROP_WIDTH, inputImg.cols-2*CROP_WIDTH, inputImg.rows-2*CROP_WIDTH));
			}

			threshold(saliencyMap[methodStr], saliencyObj, threshold_f, 255, THRESH_BINARY);

			evaluateMap(precision[methodStr], recall[methodStr], binaryMask, saliencyObj);

		}


		if (precision["CHO"] < precision["RC"] + 0.2) continue;

		Size imgSize = originImg.size();
		Mat output(imgSize.height+20, imgSize.width * 10 + 90, CV_8UC3, Scalar(255, 255, 255));

		originImg.copyTo(output(Rect(0, 10, imgSize.width, imgSize.height)));
		saliencyMap["LC"].copyTo(output(Rect(10 + imgSize.width, 10, imgSize.width, imgSize.height)));
		saliencyMap["SR"].copyTo(output(Rect(10*2 + imgSize.width*2, 10, imgSize.width, imgSize.height)));
		saliencyMap["FT"].copyTo(output(Rect(10*3 + imgSize.width*3, 10, imgSize.width, imgSize.height)));
		saliencyMap["SEG"].copyTo(output(Rect(10*4 + imgSize.width*4, 10, imgSize.width, imgSize.height)));
		saliencyMap["CB"].copyTo(output(Rect(10*5 + imgSize.width*5, 10, imgSize.width, imgSize.height)));
		saliencyMap["HC"].copyTo(output(Rect(10*6 + imgSize.width*6, 10, imgSize.width, imgSize.height)));
		saliencyMap["RC"].copyTo(output(Rect(10*7 + imgSize.width*7, 10, imgSize.width, imgSize.height)));
		saliencyMap["CHO"].copyTo(output(Rect(10*8 + imgSize.width*8, 10, imgSize.width, imgSize.height)));
		binaryMask.copyTo(output(Rect(10*9 + imgSize.width*9, 10, imgSize.width, imgSize.height)));

		char outputName[100];
		sprintf(outputName, "test/MSRA10K/examples/%d.png", exampleCount);
		imwrite(outputName, output);

		exampleCount++;
		cout << exampleCount << endl;

	}
}