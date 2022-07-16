#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>

#include<iostream>
#include<cmath>


using namespace cv;
using namespace std;

//double calAng(Point2f d1, Point2f d2,Point2f d3,  Point2f d4);

/// 修改参数：
/// 根据摄像头像素修改  搜索：  pm1  
int main()
{
	VideoCapture vid(0);

	Mat inPut;
	Mat hsv, mask, can, dil;

	int hmin{ 90 }, hmax{ 129 }, smin{ 133 }, smax{ 255 }, vmin{ 30 }, vmax{ 234 };
	Scalar lower(hmin, smin, vmin);
	Scalar upper(hmax, smax, vmax);

	//		string path="/home/rnm/桌面/test/01.jpg";
	//		inPut=imread(path);
	//		resize(inPut,inPut,Size(600,600));
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));

	//	namedWindow("bars", (600, 300));
		//for test-----------------------------
		//int maxAreaRatio{ 54 }, minAreaRatio{ 44 };
		//createTrackbar("Rmax", "bars", &maxAreaRatio, 70);
		//createTrackbar("Rmin", "bars", &minAreaRatio, 60);

		//createTrackbar("hmax", "bars", &hmax, 179);
		//createTrackbar("hmin", "bars", &hmin, 179);
		//createTrackbar("smax", "bars", &smax, 255);
		//createTrackbar("smin", "bars", &smin, 255);
		//createTrackbar("vmax", "bars", &vmax, 255);
		//createTrackbar("vmin", "bars", &vmin, 255);
		//--------------------------------------over
	while (true)
	{
		waitKey(20);
		vid.read(inPut);/*********视频输入**********/
//		resize(inPut,inPut,Size(),0.5,0.5);/**********图片输入，太大***********/

		cvtColor(inPut, hsv, COLOR_BGR2HSV);
		inRange(hsv, lower, upper, mask);/**************第一次二值化
		路标牌规整且锐利，预处理先简化，以后再说吧**************/
		Canny(mask, can, 25, 75);
		dilate(can, dil, kernel);
		imshow("t1", dil);

		vector<vector<Point>> contourDil;/***************dil的边缘集***********/
		vector<vector<Point>> contourOut;/************标和框的点集************/
		vector<Vec4i> pos1;
		findContours(dil, contourDil, pos1, RETR_LIST, CHAIN_APPROX_SIMPLE);

		double areaMax = 0, p;
		int iCir{ -1 }, iArr;
		vector<int> areaRec;/*****************所有边缘的面积，用以提取标和框***************/
		vector<size_t> seri;//序号集

		for (size_t ia = 0; ia < contourDil.size(); ia++)
		{
			p = fabs(contourArea(contourDil[ia], true));
			if (p > 500)//pm1
			{
				areaRec.push_back(p);
				seri.push_back(ia);
			}
			if (p > areaMax)
			{
				areaMax = p;
				iCir = seri.size() - 1;//序号集当前项的序号
			}/**************提取最大边缘，假设他作为框***************/
		}
		if (areaMax < 1500)//pm1
		{
			imshow("t2", inPut);
			continue;/*************最大轮廓也太小，判定图像中只有噪点************/
		}
		//for test--------------------------------------------
		//		double maxRatio{ (double)maxAreaRatio / 10 }, minRatio{ (double)minAreaRatio / 10 };
				//---------------------------------------over
				/****************判断外边框是方的还是圆的，设置标和框的面积比，并且获取外边框几何中心*************/
		double maxRatio{ 0 }, minRatio{ 0 };
		double maxRatioS{ 0 }, minRatioS{ 0 };
		if (!(iCir == -1))//有外边框
		{
			RotatedRect rectOrNot{ minAreaRect(contourDil[seri[iCir]]) };//外框的最小旋转矩形
			double s{ fabs(contourArea(contourDil[seri[iCir]], true)) };//外框面积
			if (rectOrNot.size.area() / s < 1.05 && rectOrNot.size.area() / s >0.95)//外框是矩形
			{
				maxRatio = { 5.4 };
				minRatio = { 4.4 };
				maxRatioS = { 6.35 };
				minRatioS = { 5.25 };
			}
			else
			{

				if (rectOrNot.size.area() / s < (4.15 / (4 * atan(1))) && rectOrNot.size.area() / s >(3.85) / (4 * atan(1)))//外框是圆形
				{
					maxRatio = { 6.2 };
					minRatio = { 5.0 };
					//					maxRatioS = { };
					//					minRatioS = { };
					//圆形直行牌没有，数据没有
				}

			}
		}

		if (maxRatio)//上一步if执行到了
		{
			bool findOrNot{ false };
			for (size_t i = 0; i < areaRec.size(); i++)
			{
				if ((maxRatioS > fabs((double)areaMax / (double)areaRec[i])) && fabs(((double)areaMax / (double)areaRec[i] > minRatioS)))//用假设的外框依照面积比去匹配标
				{
					iArr = i;
					contourOut.push_back(contourDil[seri[iCir]]);//框
					contourOut.push_back(contourDil[seri[iArr]]);//直行标
					findOrNot = true;
					//for test----------------------------------
					//cout << contourOut[0] << "##" << contourOut[1] << endl;
					//----------------------------------over
					break;
				}
				if ((maxRatio > fabs((double)areaMax / (double)areaRec[i])) && fabs(((double)areaMax / (double)areaRec[i] > minRatio)))
				{
					iArr = i;
					contourOut.push_back(contourDil[seri[iCir]]);//框
					contourOut.push_back(contourDil[seri[iArr]]);//转弯标
					findOrNot = true;
					//for test----------------------------------
					//cout << contourOut[0] << "##" << contourOut[1] << endl;
					//----------------------------------over
					break;
				}
			}
			if (findOrNot == false)
			{
				imshow("t2", inPut);
				continue;/**************没识别到标***********/
			}
			else
			{
				//cout << fabs(contourArea(contourOut[0], true)) / fabs(contourArea(contourOut[1], true)) << endl;
				//drawContours(inPut, contourOut, -1, Scalar(0, 255, 255), 2);
				int answer{ 0 };
				Rect arr{ boundingRect(contourOut[1]) };
				Rect con{ boundingRect(contourOut[0]) };//取标和框的最小正立矩形
				if (con.width / arr.width > 1.5)
				{
					answer = 3;//直行
					continue;
				}
				vector<Point> hull;
				vector<Point2f> triAng;
				convexHull(contourOut[1], hull);
				minEnclosingTriangle(hull, triAng);//将拐弯箭头化为三角形
				double d1, d2, d3;
				int handleAng{ -1 }, arrayAng{ -1 };
				d1 = fabs(triAng[0].x - triAng[1].x);
				d2 = fabs(triAng[1].x - triAng[2].x);
				d3 = fabs(triAng[2].x - triAng[0].x);
				//找到三角形靠近箭头的头的点和柄底或弯的点
				if (d1 > d2)
				{
					if (d2 > d3)
					{
						handleAng = 2;
						arrayAng = 1;
					}
					else
					{
						handleAng = 2;
						arrayAng = 0;
					}
				}
				else
				{
					if (d1 < d3)
					{
						handleAng = 1;
						arrayAng = 2;
					}
					else
					{
						handleAng = 2;
						arrayAng = 1;
					}
				}
				if (triAng[handleAng].x < triAng[arrayAng].x)//屏幕里箭头向右
				{
					answer = 1;//左拐
				}
				else
				{
					answer = 2;//右拐
				}
				switch (answer)
				{
				case 1:
					cout << "L" << endl;
					break;
				case 2:
					cout << "R" << endl;
					break;
				case 3:
					cout << "S" << endl;
					break;
				default:
					break;
				}
			}
		}
		imshow("t2", inPut);
	}
}

//double calAng(Point2f d1, Point2f d2,Point2f d3,Point2f d4)
//{
//	Point2f l1 = d1 - d2;
//	double p1 = sqrt(pow((d1.x - d2.x),2)+pow((d1.y - d2.y),2));
//	Point2f l2 = d3 - d4;
//	double p2 = sqrt(pow((d3.x - d4.x), 2) + pow((d3.y - d4.y), 2));
//
//
//	return ((l1.ddot(l2))/(p1*p2));
//}
