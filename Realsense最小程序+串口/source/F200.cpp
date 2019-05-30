// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

//包含所有头文件
#include "iostream"
#include "stdafx.h"
#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <cv.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdlib.h>
using namespace std;
/***************************************************************************************************************
																	  全局变量常量
****************************************************************************************************************/
////改变用于扇叶车
const int ROI_D_Y1 = 40;
const int ROI_D_Y2 = 440;
const int ROI_D_X1 = 100;
const int ROI_D_X2 = 490;

//串口通讯
uchar baotou1 = 0x66;
uchar data_send_char_buff[5] = { 0 };
uchar baowei1 = 0x77;
uchar baowei2 = 0x88;

//extern收数的数据变量
CSerialPort mySerialPort;
int  usart_rt_flag;
uchar Flag_Rec_1 = 0, Flag_Rec_2 = 0, Flag_Rec_3 = 0;
uchar Flag_Rec_1_Old = 0;

//调试//时间计算
clock_t  time0_old = 0, time0 = 0;

Mat display;
float x_out, y_out, z_out;
//函数声明

//坐标显示函数
/*
void on_mouse(int event, int x, int y, int flag, void *param)
{
	if (event == CV_EVENT_MOUSEMOVE)
	{
		Point pt = Point(x, y);
		cout << x_out << ", " << y_out << ", " << z_out << endl;
	}
}
*/
/***************************************************************************************************************
																		初始化
****************************************************************************************************************/
int main(int argc, char * argv[]) try
{
	/**************************************串口初始化************************************/
	//实例化串口对象
	//wprintf_s(L"Searching for vaild SerialPort ...\n");
	//while (!mySerialPort.InitPort(3, 115200))
	//{
	//}
	//wprintf_s(L"Init mySerialPort succeed !\n");
	//if (!mySerialPort.OpenListenThread())
	//{
	//	wprintf_s(L"SerialPort Receive Init succeed !\n");
	//}

	//坐标显示
	//namedWindow("River", 1);
	//cvSetMouseCallback("River", on_mouse, NULL);

	/**************************************设备初始化************************************/
	//显示warn选项
    rs::log_to_console(rs::log_severity::warn);
    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");

    // Enumerate all devices//枚举设备
	rs::device * dev = ctx.get_device(0);   //初始化realsense
	printf("Using device 0, an %s\n", dev->get_name());
	printf("Serial number: %s\n", dev->get_serial());
	printf("Firmware version: %s\n", dev->get_firmware_version());

	//初始化参数
	dev->enable_stream(rs::stream::color, 640, 480, rs::format::bgr8, 60); //开启RGB数据
	dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 60);  //开启深度数据
	dev->start();

/***************************************************************************************************************
																	   程序主循环
****************************************************************************************************************/
	while (1)
	{
		/********************************************图片收取**********************************************/
		dev->wait_for_frames();

		//计算帧率
		time0 = clock();
		if (time0_old != 0)
			//printf("tsum_ms=  %d\n", (time0 - time0_old));
			time0_old = time0;

		//数据收取
		const uint16_t * color_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::color));   //获取RGB数据
		Mat color1(dev->get_stream_height(rs::stream::color), dev->get_stream_width(rs::stream::color), CV_8UC3, (void*)color_frame);
		const uint16_t * depth_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::depth));   //获取深度数据
		Mat depth1(dev->get_stream_height(rs::stream::depth), dev->get_stream_width(rs::stream::depth), CV_16UC1, (void*)depth_frame);
		/********************************************赛道提取*********************************************/
	

		

		/******************************************串口通讯逻辑******************************************/
		////Flag_GuoHe
		//data_send_char_buff[0] = Flag_GuoHe;
		////RobotDepth_Up
		//data_send_char_buff[1] = (unsigned char)(RobotDepth_Up / 100);
		//data_send_char_buff[2] = (unsigned char)(RobotDepth_Up - data_send_char_buff[1] * 100);
		////RobotDepth_River
		//data_send_char_buff[3] = (unsigned char)(RobotDepth_River / 100);
		//data_send_char_buff[4] = (unsigned char)(RobotDepth_River - data_send_char_buff[3] * 100);
		//mySerialPort.WriteData(&baotou1, 1);//发包头 
		//mySerialPort.WriteData(data_send_char_buff, 5);//发数据,注意需要与数组的长度相匹配
		//mySerialPort.WriteData(&baowei1, 1);//发包尾1
		//mySerialPort.WriteData(&baowei2, 1);//发包尾2 

		/********************************************循环尾处理*****************************************/
		waitKey(1);
	}
	    return EXIT_SUCCESS;
}

/***************************************************************************************************************
																	 设备错误报告
****************************************************************************************************************/
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
/***************************************************************************************************************
																        调用函数
****************************************************************************************************************/
