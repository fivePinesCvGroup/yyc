#include<opencv2/imgcodecs.hpp>
#include<opencv2/imgproc.hpp>
#include<opencv2/highgui.hpp>

#include<iostream>
#include<cmath>

using namespace cv;
using namespace std;

//double calAng(Point2f d1, Point2f d2,Point2f d3,  Point2f d4);

void cleanVec(vector<RotatedRect>& vec, const Mat offerAxes);
//调整参数,pm1,pm2,pm3,pm4,pm5
//二维码识别还是会出错，仿佛是因为二维码被画歪了，具体为什么没明白
int main()
{
	VideoCapture vid(0);

	Mat inPut;
	Mat hsv, mask, can, dil;
	Mat imgcut, inPut2;

	int hmin{ 10 }, hmax{ 69 }, smin{ 0 }, smax{ 218 }, vmin{ 94 }, vmax{ 255 };
	Scalar lower(hmin, smin, vmin);
	Scalar upper(hmax, smax, vmax);
	//		string path="/home/rnm/桌面/test/01.jpg";
	//		inPut=imread(path);
	//		resize(inPut,inPut,Size(600,600));
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));

	//for test-----------------------------
//	namedWindow("bars", (600, 300));
	//int maxAreaRatio{ 54 }, minAreaRatio{ 44 };
	//createTrackbar("Rmax", "bars", &maxAreaRatio, 70);
	//createTrackbar("Rmin", "bars", &minAreaRatio, 60);
	//int ratio{ 100 };
	//createTrackbar("r", "bars", &ratio, 4500);

	//createTrackbar("hmax", "bars", &hmax, 179);
	//createTrackbar("hmin", "bars", &hmin, 179);
	//createTrackbar("smax", "bars", &smax, 255);
	//createTrackbar("smin", "bars", &smin, 255);
	//createTrackbar("vmax", "bars", &vmax, 255);
	//createTrackbar("vmin", "bars", &vmin, 255);
	//--------------------------------------over
	while (true)
	{
		waitKey(40);
		vid.read(inPut);/*********视频输入**********/
//		resize(inPut,inPut,Size(),0.5,0.5);/**********图片输入，太大***********/
		cvtColor(inPut, hsv, COLOR_BGR2HSV);
		inRange(hsv, lower, upper, mask);/**************第一次二值化*/
		
		Canny(mask, can, 25, 75);
		dilate(can, dil, kernel);
		imshow("t1", mask);

		vector<vector<Point>> contourDil;/***************dil的边缘集***********/
		vector<Vec4i> pos1;
		findContours(dil, contourDil, pos1, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);//取外轮廓，确定矿石位置所在

		double areaMax = 0, p;
		int iRect{ -1 };
		vector<int> areaRec;
		vector<size_t> seri;//序号集

		for (size_t ia = 0; ia < contourDil.size(); ia++)
		{
			p = fabs(contourArea(contourDil[ia], true));
			if (p > 2000)//pm1
			{
				areaRec.push_back(p);
				seri.push_back(ia);
			}
		}//去掉小噪点

		for (size_t im = 0; im < seri.size(); im++)
		{
			RotatedRect rect1{ minAreaRect(contourDil[seri[im]]) };
			if (areaRec[im]/fabs(rect1.size.area())  > 0.85 && areaRec[im] / fabs(rect1.size.area())< 1.15)//pm2
				//面积比例过滤噪点和朝向不正(取没有严重近大远小的矩形)
			{
				p = areaRec[im];
//				drawContours(inPut, contourDil,seri[im], Scalar(0, 0, 255), 3);
				if (p > areaMax)
				{
					areaMax = p;
					iRect = seri[im];//序号集当前项的序号
				}//找到正确的矩形

			}
		}//取矩形轮廓
		if (iRect == -1)
		{
			imshow("t2", inPut);
			continue;//没有取到矩形
		}

		RotatedRect rectMax{ minAreaRect(contourDil[iRect]) };//最大矩形轮廓
		float heiRatioWidth{ rectMax.size.height / rectMax.size.width };
		//认为矿石是有一定颜色过滤后图像当中最大的那个正方形
		if (!(heiRatioWidth>1.1|| heiRatioWidth<0.91))
		{
			Rect rangeForCut;
			rangeForCut = rectMax.boundingRect();//矿石位置区域
//			cout << rangeForCut << endl;
			if (rangeForCut.tl().x < 0 || rangeForCut.tl().y < 0 ||
				(rangeForCut.width + rangeForCut.tl().x)>dil.size().width || (rangeForCut.height + rangeForCut.tl().y)>dil.size().height)
			{
				imshow("t2", inPut);
//				cout << "error" << endl;
				continue;//得到错误的rectForCut
				//有时计算出的矿石位置区域会有一部分在画布外面
			}
			imgcut=dil(rangeForCut);//单独裁下矿石
			erode(imgcut, inPut2, kernel);
			contourDil.clear();
			pos1.clear();

			findContours(inPut2, contourDil, pos1, RETR_TREE, CHAIN_APPROX_SIMPLE);//找到内部所有轮廓，画面细节

			double arc{ 0 };// r{ (double)ratio / 100000 };
			int psize{ imgcut.size().area() };//画布大小
			vector<RotatedRect> cornerRect;//角点里的小正方形
			vector<RotatedRect> cornerAng;//角点里的，，
			int countCenter{ 0 };//二维码条的数量
			double ps{ 0 };//二维码面积
			for (size_t i{ 0 }; i < contourDil.size(); i++)
			{
				p = fabs(contourArea(contourDil[i], true));
				//把小噪点过滤掉
				if (p > pow(imgcut.cols/20,2))//pm3
				{
					RotatedRect perRect{ minAreaRect(contourDil[i]) };
					bool rect{ (perRect.size.area() / p) < 1.2 };//pm4
					bool rectOrNot{ rect &&
						(perRect.size.height / perRect.size.width) > 0.80 && 1.25 > (perRect.size.height / perRect.size.width) };//正方形
					//认为一定范围面积的正方形是角点里的矩形
					if (rectOrNot &&p*15< psize&&p*50>psize)//pm5
					{
//						drawContours(imgcut, contourDil, i, Scalar(255), FILLED);
						cornerRect.push_back(perRect);
					}
					else
					{//&& p * 30 < psize && p * 100 > psize
						Rect re{ perRect.boundingRect() };
						bool cros{ (fabs((re.x + re.width / 2) - imgcut.cols / 2) < imgcut.cols / 6) ||
							(fabs((re.y + re.height / 2) - imgcut.rows / 2) < imgcut.rows / 6) };//最小正立矩形在画布的横或竖中间
						bool rect_2{ (perRect.size.area() / p) < 1.3 };//得到的轮廓像一个矩形（1.3较大）pm4
						if ( rect_2&&cros)
						{
							if (perRect.size.area() * 4 < imgcut.size().area())//过滤掉矿石本身及其它可能的干扰
							{
//								drawContours(imgcut, contourDil, i, Scalar(255), FILLED);
								ps += p;
								countCenter += 1;
							}
						}
						else
						{
							bool angOrNot{ (re.size().area() / p) < 2.5 && (re.size().area() / p) > 1.5 &&
							(re.height/re.width<1.25&&re.height/re.width>0.8)};
							//最小正立矩形和轮廓面积比在一定范围内并且长宽比在一定范围内
							if (angOrNot)
							{
								vector<Point> approx;
								double arc{ arcLength(contourDil[i], true) };
								approxPolyDP(contourDil[i],approx,0.045*arc,true);
//								cout << approx.size() << endl;
								if ( approx.size() < 8&&approx.size()>5&&p*40<psize&&p*100>psize)
									//近似多边形有六七个角点并且轮廓本身面积在一定范围pm5
								{
//									drawContours(imgcut, contourDil, i, Scalar(255), FILLED);
//									cout << (re.size().height / re.size().width) << "##" << p << endl;
									cornerAng.push_back(perRect);
								}
							}
						}
						
					}
				}
			}//获得角标中的正方形
			imshow("t4", imgcut);
			cleanVec(cornerRect, imgcut);//去掉几乎重叠的轮廓
			cleanVec(cornerAng, imgcut);
//			cout << ps<<"---------------------------------------------------"<< cornerRect.size() << cornerAng .size()<< endl;
			int answer{ 0 };
			if (cornerRect.size() == 1 && cornerAng.size() == 3)
			{
				answer = 1;//r面
			}
			else
			{
				if (cornerRect.size() == 2 && cornerAng.size() == 2)
				{
					if (ps > 0.05 * imgcut.size().area()||countCenter>4)
					{
						answer = 2;//二维码
					}
					else
					{
						answer = 3;//blank
					}
				}
			}
			switch (answer)
			{
			case 1:
				cout << "R_side" << endl;
				break;
			case 2:
				cout << "QR_side" << endl;
				break;
			case 3:
				cout << "blank_side" << endl;
				break;
			default:
				break;
			}
		}
		else
		{
			imshow("t2", inPut);
			continue;//没有识别到正确的矩形
		}
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

void cleanVec(vector<RotatedRect>& vec, const Mat offerAxes)
{
	if (vec.size() > 1)
	{
			for (size_t i{ 0 }; i < vec.size() - 1; i++)
			{
				for (size_t m{ i + 1 }; m < vec.size(); m++)
				{
					if (fabs(vec[i].center.x - vec[m].center.x) < (offerAxes.cols / 2)&&
						fabs(vec[i].center.y - vec[m].center.y) < (offerAxes.rows / 2))
					{
						swap(vec[m], vec[vec.size() - 1]);
						vec.pop_back();
					}
				}
			}
	}
}
