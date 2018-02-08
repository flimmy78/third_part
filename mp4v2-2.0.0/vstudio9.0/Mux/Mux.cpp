/********************************************************************
filename:   Mux.cpp
created:    2016-08-06
author:     Donyj
purpose:    MP4编码器，基于开源库mp4v2实现（https://code.google.com/p/mp4v2/）。
*********************************************************************/
#include <stdio.h>
#include<Winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include "MP4Encoder.h"

FILE *fp_h264;
FILE *fp_AAC;
static int count_audio = 0;
int read_h264(unsigned char *buf, int buf_size)
{
	int true_size = fread(buf, 1, buf_size, fp_h264);
	if (true_size > 0){
		return true_size;
	}
	else{
		//fseek(fp_AAC, 0L, SEEK_SET);
		//return read_aac(buf, buf_size);
		return 0;
	}
}

int read_aac(unsigned char *buf, int buf_size)
{
	unsigned char aac_header[7];
	int true_size = 0;
	
	true_size = fread(aac_header, 1, 7, fp_AAC);
	if (true_size <= 0)
	{
		fseek(fp_AAC, 0L, SEEK_SET);
		return read_aac(buf, buf_size);
	}
	else
	{
		unsigned int body_size = *(unsigned int *)(aac_header + 3);
		body_size = ntohl(body_size); //Little Endian
		body_size = body_size << 6;
		body_size = body_size >> 19;

		true_size = fread(buf, 1, body_size - 7, fp_AAC);

		return true_size;
	}
}

int main(int argc, char** argv)
{
	//fp_h264 = fopen("test.h264", "rb");
	//fp_h264 = fopen("1080.h264", "rb");
	fp_h264 = fopen("bitstream.h264", "rb");
	fp_AAC = fopen("test.aac", "rb");

	MP4Encoder mp4Encoder;
	// convert H264 file to mp4 file
	mp4Encoder.MP4FileOpen("test.mp4", 1, 1);
	mp4Encoder.MP4FileWrite(read_h264, read_aac);
	mp4Encoder.MP4FileClose();

	fclose(fp_h264);
	fclose(fp_AAC);
}
