#ifndef _img_h_
#define _img_h_

#include <iostream>
/*
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
*/
#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/highgui/highgui.hpp>
#include <opencv4/opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


void file_to_datapng(char* file_name, char* data, int size, int &len);
void datapng_to_array(char* data, int lenin, unsigned char* array, int &width, int &height);
void datapng_to_array32F(char* data, int lenin, float* array, int &width, int &height);
void array32FC1_to_file(float* array, int width, int height, char* file);
void array32FC3_to_file(float* array, int width, int height, char* file);
void array_to_file(unsigned char* array, int width, int height, char* file);
void resize_img_array(unsigned char* array, int width, int height, unsigned char* array_out, int new_width, int new_height);

#endif
