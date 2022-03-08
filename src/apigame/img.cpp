
#include "img.h"

void file_to_datapng(char* file_name, char* data, int size, int &len)
{
	FILE* f = fopen(file_name, "rb");
	len = fread(data, 1, size, f);
	fclose(f);
}

void datapng_to_array(char* data, int lenin, unsigned char* array, int &width, int &height)
{
	//Mat imgRaw( 1, lenin, CV_8UC3, (void*)data );
	//Mat img = imdecode(imgRaw, IMREAD_UNCHANGED);
	
	std::vector<char> datav(data, data + lenin);
	Mat img = imdecode(Mat(datav), 1);
	
	height = img.rows;
	width = img.cols;
	
	memcpy(array, img.data, height*width*3*sizeof(unsigned char));
}

void datapng_to_array32F(char* data, int lenin, float* array, int &width, int &height)
{
	//Mat imgRaw( 1, lenin, CV_8UC3, (void*)data );
	//Mat img = imdecode(imgRaw, IMREAD_UNCHANGED);
	
	std::vector<char> datav(data, data + lenin);
	Mat img = imdecode(Mat(datav), 1);

	img.convertTo(img, CV_32FC3);

	height = img.rows;
	width = img.cols;
	
	memcpy(array, img.data, height*width*3*sizeof(float));
}



void array32FC1_to_file(float* array, int width, int height, char* file)
{
	Mat img(height, width, CV_32FC1, array);
	imwrite(file, img);
}
void array32FC3_to_file(float* array, int width, int height, char* file)
{
	Mat img(height, width, CV_32FC3, array);
	imwrite(file, img);
}
void array_to_file(unsigned char* array, int width, int height, char* file)
{
	Mat img(height, width, CV_8UC3, array);
	imwrite(file, img);
}

void resize_img_array(unsigned char* array, int width, int height, unsigned char* array_out, int new_width, int new_height)
{
	Mat img1(height, width, CV_8UC3, array);
	Mat img2;

	cv::resize(img1, img2, Size(new_height,new_width));
	
	memcpy(array_out, img2.data, new_height*new_width*3*sizeof(unsigned char));
}
