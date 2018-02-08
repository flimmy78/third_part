// WinRtp.c : A program to realize rtp transmission.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <fcntl.h>
#include <share.h>
#include <io.h>
#include "h264.h"

#include "H264Handle.h"
#include "RtpHandle.h"
//Load ws2_32 library which are used to link the windows api methods
#pragma comment(linker,"/defaultlib:ws2_32.lib")

char *input=NULL;								   //Input file address
int fh;
char *ip=NULL;									   //Destination IP address
int port=0;									       //Destination application port

//static int info2=0, info3=0;

//Output length of NALU and type of NALU
void dump(NALU_t *n)
{
	if (!n)return;
	printf("Length : %5d  ", n->len);
	printf("NALU Type : %x\n", n->nal_unit_type);
}

//Show help information
void Help()
{
	printf("Usage : \n");
	printf("WinRtp [-f InputFile] [-o DestAddress/Port] [-p framerate=30] [-h]\n");
	printf("-f		InputFile to be transmitted.\n");
	printf("-o		Destination address and port of the data.\n");
	printf("-o		Frame rate of the video.\n");
	printf("-h		Show help information");
	printf("\n");
	printf("Example:\n");
	printf("WinRtp -f test.264 -o 127.0.0.1/1234 -p framerate=30\n");
	printf("WinRtp -f test.264\n");
	printf("WinRtp -o 127.0.0.1/1234\n");

	exit(0);
}

static FILE* _fH264 = NULL;
static char _startBit[4] = {0x00, 0x00, 0x00, 0x01};
static void _WriteH264(char *data, int len, int addStartBit)
{
	if(_fH264 == NULL)
	{
		_fH264 = fopen("NALU_H264_frame.264","wb");
		printf("=====>>>>> open file\n");
	}
	if(data == NULL)
	{
		fclose(_fH264);
		printf("=====>>>>> close file\n");
	}
	else
	{
		if(addStartBit != 0)
		{
			fwrite(_startBit, 1, 4, _fH264);
		}
		fwrite(data, 1, len, _fH264);
		printf("=====>>>>> fwrite file n->len:%d\n",len);
	}
}

CH264Handle* pH264Handle;
CRtpHandle* pRtpHandle;
NALU_t *n;
float framerate=30;										//Frame rate of the output file	
void RtpTranmissonInit(char* ip, int port)
{
	if(pRtpHandle == NULL)
	{
		pRtpHandle = new CRtpHandle();
	}
	pRtpHandle->InitiateWinsock();
	if(pH264Handle == NULL)
	{
		pH264Handle = new CH264Handle();
	}
	pRtpHandle->Connet(ip, port);
	
	if(n == NULL)
	{
		n = pH264Handle->AllocNALU(80000000);    
	}
}
void RtpTranmissonSend(unsigned char* frame, int frameSize)
{
	int sendLength = 0;
	while(sendLength < frameSize)
	{
		int remaidLen = frameSize-sendLength;
		int r = CH264Handle::GetAnnexbNALU(frame+sendLength, remaidLen, n);
		if(r > 0)
		{
			//_WriteH264(n->buf, n->len, 1);
			if(n->nal_unit_type == 7) //sps
			{
				CH264Handle::SaveSPS((unsigned char*)n->buf, n->len);
			}
			else if(n->nal_unit_type == 8)//pps
			{
				CH264Handle::SavePPS((unsigned char*)n->buf, n->len);
			}
					
			if(n->nal_unit_type != 9 && n->nal_unit_type != 6)//ignor type 06 and 09
			{
				if(pRtpHandle != NULL)
				{
					pRtpHandle->PacketSend(n, framerate);
				}
				if(n != NULL)
				{
					dump(n);												//Output NALU length and type
				}
			}
		}
		sendLength += r;
	}
}
void RtpTranmissonDestory()
{
	if(n != NULL)
	{
		CH264Handle::FreeNALU(n);
	}
	if(pH264Handle != NULL)
	{
		delete pH264Handle;
		pH264Handle = NULL;
	}
	if(pRtpHandle != NULL)
	{
			delete pRtpHandle;
		pRtpHandle = NULL;
	}
}

//Main entry for the program
int main(int argc, char* argv[])
{
	double start;
	double end;
	unsigned char* frame = (unsigned char*)calloc (80000000 , sizeof(char));

	int _len = 0;

	//Check the user inputs
	if(argc>7)
		Help();
	else
	{
		int i;	
		for(i=1;i<argc;i++)
		{
			char *p;
			p=argv[i];
			if(*p=='-')
			{
				switch(*++p)
				{
				case 'h':Help();break;
				case 'f':input=argv[i+1];break;
				case 'o':{
					char *temp=NULL;
					temp=(char*)malloc(16*sizeof(char));
					ip=temp;
					while(*argv[i+1]!='/')
					{
						*temp++=*argv[i+1]++;
					}
					*temp='\0';
					port=atoi(++argv[i+1]);
						 }
						 break;
				case 'p':while(*argv[i+1]++!='='){}
						 framerate=atof(argv[i+1]);
						 break;
				default:Help();break;
				}
			}
		}
	}

	printf("Input file is %s\n",input);
	printf("Destination IP is : %s\n",ip);
	printf("Destination port is : %d\n",port);
	printf("Frame rate is : %f\n",framerate);
	
	RtpTranmissonInit(ip, port);
	while(true/*!pH264Handle->BitStreamFeof()*/) 
	{
		start = GetTickCount();  //记录开始时间戳
		int frameSize = CH264Handle::GetAnnexbFrame(frame, _len);
		if(frameSize<=0)
		{
			break;
		}
		else
		{
			RtpTranmissonSend(frame, frameSize);
		}
		
		end = GetTickCount();  //记录开始时间戳
		//计算上述花了多长时间
		printf("1000/framerate:%f end-start：%f\n",1000.0/framerate, (float)(end-start)/1000.0);
		Sleep(1000.0/framerate - (float)(end-start)/1000.0);
	}
	RtpTranmissonDestory();


	unsigned char* tmp = NULL;
	int length = 0;
	CH264Handle::GetSPS(tmp, length);
	if(length > 0)
	{
		for(int i=0; i< length; i++)
		{
			printf(" %2x",tmp[i]);
		}
	}
	printf("\n\n====================\n\n");
	length = 0;
	CH264Handle::GetPPS(tmp, length);
	if(length > 0)
	{
		for(int i=0; i< length; i++)
		{
			printf("%2x ",tmp[i]);
		}
	}
	tmp = NULL;
	printf("\n\n====================\n\n");
	free(frame);
	return 0;
}
