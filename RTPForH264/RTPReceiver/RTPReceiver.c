// RTPReceiver.c : A program to realize rtp receiver functionality.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include "h264.h"
//Load ws2_32 library which are used to link the windows api methods
#pragma comment(linker,"/defaultlib:ws2_32.lib")

typedef struct
{
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
	unsigned max_size;            //! NAL Unit Buffer size
	int forbidden_bit;            //! Should always be FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx    
	char *buf;                    //! contains the first byte followed by the EBSP
	unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;

char *output=NULL;								   //Output file address
FILE *bits = NULL;							       //!< Output file
char *ip=DEST_IP;								   //Destination IP address
int port=DEST_PORT;							       //Destination application port
float framerate=15;								   //Frame rate of the output file	
static int FindStartCode2 (unsigned char *Buf);    //Search for start char 0x000001
static int FindStartCode3 (unsigned char *Buf);    //Search for start char 0x00000001

static int info2=0, info3=0;
RTP_FIXED_HEADER        *rtp_hdr;

NALU_HEADER		*nalu_hdr;
FU_INDICATOR	*fu_ind;
FU_HEADER		*fu_hdr;

BOOL InitiateWinsock()
{
	int Error;
	WORD Version=MAKEWORD(2,2);
	WSADATA WsaData;
	Error=WSAStartup(Version,&WsaData);	      //Start up WSA	  
	if(Error!=0)
		return FALSE;
	else
	{
		if(LOBYTE(WsaData.wVersion)!=2||HIBYTE(WsaData.wHighVersion)!=2)
		{
			WSACleanup();
			return FALSE;
		}

	}
	return TRUE;
}

//Allocate memory for NALU_t structure
NALU_t *AllocNALU(int buffersize)
{
	NALU_t *n;

	if ((n = (NALU_t*)calloc (1, sizeof(NALU_t))) == NULL)
	{
		printf("AllocNALU Error: Allocate Meory To NALU_t Failed ");
		exit(0);
	}

	n->max_size=buffersize;									//Assign buffer size 

	if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (n);
		printf ("AllocNALU Error: Allocate Meory To NALU_t Buffer Failed ");
		exit(0);
	}

	return n;
}
//Release NALU Memory
void FreeNALU(NALU_t *n)
{
	if (n)
	{
		if (n->buf)
		{
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
}

void OpenBitstreamFile (char *fn)
{
	if (NULL == (bits=fopen(fn, "wb")))
	{
		printf("Error: Open input file error\n");
		exit(0);
	}
}
//这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，获取他的长度，填充F,IDC,TYPE位。
//并且返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
NALU_t* SetAnnexbNALU (char *buf)
{
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;
	NALU_t *n;
	n = AllocNALU(8000000);

	if(strlen(buf)<12)
	{
		printf("No RTP header for this parcket");
	}
	/*
		if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
		printf ("GetAnnexbNALU Error: Could not allocate Buf memory\n");
    */
	/*
	nalu->startcodeprefix_len=3;      //Initialize the bits prefix as 3 bytes

	if (3 != fread (Buf, 1, 3, bits))//Read 3 bytes from the input file
	{
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);    //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		//If Buf is not 0x000001,then read one more byte
		if(1 != fread(Buf+3, 1, 1, bits))
		{
			free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);//Check whether Buf is 0x00000001
		if (info3 != 1)              //If not the return -1
		{ 
			free(Buf);
			return -1;
		}
		else 
		{
			//If Buf is 0x00000001,set the prefix length to 4 bytes
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	} 
	else
	{
		//If Buf is 0x000001,set the prefix length to 3 bytes
		pos = 3;
		nalu->startcodeprefix_len = 3;
	}
	//Search for next character sign bit
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;

	while (!StartCodeFound)
	{
		if (feof (bits))
		{
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;     // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   // 5 bit--00011111
			free(Buf);
			return pos-1;
		}
		Buf[pos++] = fgetc (bits);                       //Read one char to the Buffer
		info3 = FindStartCode3(&Buf[pos-4]);		     //Check whether Buf is 0x00000001
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);           //Check whether Buf is 0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (bits, rewind, SEEK_CUR))						//Set the file position to the end of previous NALU
	{
		free(Buf);
		printf("GetAnnexbNALU Error: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
	nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//Copy one complete NALU excluding start prefix 0x000001 and 0x00000001
	nalu->forbidden_bit = nalu->buf[0] & 0x80;                     //1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit
	free(Buf);
	*/
	return n;                                           //Return the length of bytes from between one NALU and the next NALU
}
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

//Parse user commands
void ParseCommand(int argc,char *argv[])
{
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
				case 'f':output=argv[i+1];break;
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

	printf("Input file is %s\n",output);
	printf("Destination IP is : %s\n",ip);
	printf("Destination port is : %d\n",port);
	printf("Frame rate is : %f\n",framerate);
}

FILE* fH264 = NULL;
static char startBit[4] = {0x00, 0x00, 0x00, 0x01};
void WriteH264(char *data, int len, int addStartBit)
{
	if(fH264 == NULL)
	{
		fH264 = fopen("NALU_H264_RECV.264","wb");
		printf("=====>>>>> open file\n");
	}
	if(data == NULL)
	{
		fclose(fH264);
		printf("=====>>>>> close file\n");
	}
	else if(len > 0)
	{
		if(addStartBit != 0)
		{
			fwrite(startBit, 1, 4, fH264);
		}
		fwrite(data, 1, len, fH264);
		printf("=====>>>>> fwrite file n->len:%d\n",len);
	}
}

//Main entry for the program
int main(int argc, char* argv[])
{
	RTP_FIXED_HEADER *rtp_hdr;
	NALU_HEADER *nalu_hdr;
	NALU_t *n;
	RTPpacket_t *p;

	//NALU_t *n;
	char* nalu_payload;  
	char recvbuf[1500];

	unsigned short seq_num =0;
	int	bytes=0;

	SOCKET    socket1;
	struct sockaddr_in client;
	int len =sizeof(client);
	
	unsigned int timestamp_increse=0,ts_current=0;
	unsigned int first_seq_no=0,current_seq_no=0;
	
	int first_seg=1,end_seg=0;
	unsigned int prefix[4]={0,0,0,1};
	
	//Testing variables
	int total_bytes=0,total_recved=0;

	InitiateWinsock();									        //Initialize Win Socket
	ParseCommand(argc,argv);
	timestamp_increse=(unsigned int)(90000.0 / framerate);      //+0.5);
	
	if(output!=NULL)
		OpenBitstreamFile(output);                               //Open input file
	else
	{
		printf("Error:Wrong input file name.Please check whether the name is correct or not\n");
		exit(-1);
	}

	socket1=socket(AF_INET,SOCK_DGRAM,0);
	//connect(socket1, (const SOCKADDR*)&server, len) ;           //Connect to server socket

	client.sin_family=AF_INET;
	client.sin_addr.s_addr=inet_addr(ip);
	client.sin_port=htons(port);

	if(bind(socket1,(struct sockaddr*)&client,sizeof(client))==-1)
	{
		printf("Bind to local machine error.\n");
		WSACleanup();
		exit(-1);
	}
	else
		printf("Bind to local machine.\n");
		
	//p=(RTPpacket_t*)&recvbuf[0];
	if ((p=(RTPpacket_t*)malloc (sizeof (RTPpacket_t)))== NULL)
		printf ("GetRTPNALU-1");
	if ((p->packet=(byte*)malloc (MAXRTPPACKETSIZE))== NULL)
		printf ("GetRTPNALU-2");
	if ((p->payload=(byte*)malloc (MAXRTPPACKETSIZE))== NULL)
		printf ("GetRTPNALU-3");

	while((bytes=recvfrom(socket1,recvbuf,1500,0,(struct sockaddr *)&client,&len))>0)
	{
		rtp_hdr =(RTP_FIXED_HEADER*)&recvbuf[0]; 
		//printf("Bytes RTP version : %5d \n",rtp_hdr->version);
		p->v=rtp_hdr->version;
		//printf("Bytes RTP sequence number : %5d \n",rtp_hdr->seq_no);
		p->seq=rtp_hdr->seq_no;
		//printf("RTP Marker : %d.\n",rtp_hdr->marker);
		p->m=rtp_hdr->marker;
		p->x=rtp_hdr->extension;
		p->p=rtp_hdr->padding;

		//printf("recvfrom .bytes:%d\n", bytes);
		//WriteH264(&recvbuf[13], bytes, 0);
		
		n = AllocNALU(80000); 

		nalu_hdr =(NALU_HEADER*)&recvbuf[12];
		n->forbidden_bit=nalu_hdr->F;
		//printf("Fobbiden bit : %d\n",nalu_hdr->F);
		n->nal_reference_idc=nalu_hdr->NRI<<5;
		n->nal_unit_type=nalu_hdr->TYPE;
		n->buf=&recvbuf[13];
		n->len=bytes-12;
		//printf("Nalu length : %5d .\n",bytes);
		//p->packet=rtp_hdr->payload;
		//p->packlen=rtp_hdr->

		//printf("Packet length : %d .\n",pkt->packlen);
			//DecomposeRTPpacket (rtp_packet);
			//DumpRTPHeader (rtp_packet);

		//n=SetAnnexbNALU (recvbuf);
		//printf("Bytes RTP sequence number : %5d \n",rtp_hdr->seq_no);
		if(current_seq_no!=-1)
			current_seq_no=rtp_hdr->seq_no;
		if(current_seq_no<first_seq_no)
			first_seq_no=current_seq_no;

		if(bytes>12)
		{
			printf("Market bit : %d\n",p->m);
			if(p->m==0)
			{
				if(first_seg==1)
				{
					unsigned char nh;
					putc(0x00, bits);
					putc(0x00, bits);
					putc(0x00, bits);
					putc(0x01, bits);

					total_bytes+=4;
					fu_ind=(FU_INDICATOR*)&recvbuf[12];
					n->forbidden_bit=fu_ind->F;
					n->nal_reference_idc=fu_ind->NRI<<5;

					fu_hdr=(FU_HEADER*)&recvbuf[13];
					n->nal_unit_type=fu_hdr->TYPE;

					nh=(n->forbidden_bit|n->nal_reference_idc|n->nal_unit_type)&0xff;
					putc(nh,bits);									//Put the NALU HEADER into the output file
					
					total_bytes+=1;
					memcpy(p->payload,&recvbuf[14],bytes-14);
				    p->paylen=bytes-14;
					first_seg=0;
					end_seg=1;
				}
				else
				{
					memcpy(p->payload,&recvbuf[14],bytes-14);
					p->paylen=bytes-14;
				}
			}
			else
			{
				if(end_seg==1)
				{
					memcpy(p->payload,&recvbuf[14],bytes-14);
					p->paylen=bytes-14;
					first_seg=1;
					end_seg=0;
				}
				else
				{
					putc(0x00, bits);
					putc(0x00, bits);
					putc(0x00, bits);
					putc(0x01, bits);

					total_bytes+=4;
					memcpy(p->payload,&recvbuf[13],bytes-13);	
					p->paylen=bytes-13;
					fwrite(nalu_hdr,1,1,bits);
					fflush(bits);
					total_bytes+=1;
				}
			}
			fwrite(p->payload,1,p->paylen,bits);
			fflush(bits);
			total_bytes+=p->paylen;
		}
		else
			printf("Bytes length < 12\n");

		total_recved+=bytes;

		memset(recvbuf,0,1500);									//Reset receive buffer
		printf("Total bytes : %d\n",total_bytes);
		printf("Total bytes received : %d\n",total_recved);
	}
	free(p);
	//free(n);
	fclose(bits);
	FreeNALU(n);

	return 0;
}

static int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0;              //Check whether buf is 0x000001
	else return 1;
}

static int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//Check whether buf is 0x00000001
	else return 1;
}

int DecomposeRTPpacket (RTPpacket_t *p)
{
  // consistency check
  assert (p->packlen < 65536 - 28);  // IP, UDP headers
  assert (p->packlen >= 12);         // at least a complete RTP header
  assert (p->payload != NULL);
  assert (p->packet != NULL);

  // Extract header information

  p->v  = (p->packet[0] >> 6) & 0x03;
  p->p  = (p->packet[0] >> 5) & 0x01;
  p->x  = (p->packet[0] >> 4) & 0x01;
  p->cc = (p->packet[0] >> 0) & 0x0F;

  p->m  = (p->packet[1] >> 7) & 0x01;
  p->pt = (p->packet[1] >> 0) & 0x7F;

  memcpy (&p->seq, &p->packet[2], 2);
  p->seq = ntohs((uint16)p->seq);

  memcpy (&p->timestamp, &p->packet[4], 4);// change to shifts for unified byte sex
  p->timestamp = ntohl(p->timestamp);
  memcpy (&p->ssrc, &p->packet[8], 4);// change to shifts for unified byte sex
  p->ssrc = ntohl(p->ssrc);

  // header consistency checks
  if (     (p->v != 2)
        || (p->p != 0)
        || (p->x != 0)
        || (p->cc != 0) )
  {
    printf ("DecomposeRTPpacket, RTP header consistency problem, header follows\n");
    DumpRTPHeader (p);
    return -1;
  }
  p->paylen = p->packlen-12;
  memcpy (p->payload, &p->packet[12], p->paylen);
  return 0;
}

void DumpRTPHeader (RTPpacket_t *p)

{
  int i;
  for (i=0; i< 30; i++)
    printf ("%02x ", p->packet[i]);
  printf ("Version (V): %d\n", (int) p->v);
  printf ("Padding (P): %d\n", (int) p->p);
  printf ("Extension (X): %d\n", (int) p->x);
  printf ("CSRC count (CC): %d\n", (int) p->cc);
  printf ("Marker bit (M): %d\n", (int) p->m);
  printf ("Payload Type (PT): %d\n", (int) p->pt);
  printf ("Sequence Number: %d\n", (int) p->seq);
  printf ("Timestamp: %d\n", (int) p->timestamp);
  printf ("SSRC: %d\n", (int) p->ssrc);
}

int WriteRTPPacket (RTPpacket_t *p, FILE *f)
{
  int intime = -1;

  assert (f != NULL);
  assert (p != NULL);

  if (1 != fwrite (&p->packlen, 4, 1, f))
    return -1;
  if (1 != fwrite (&intime, 4, 1, f))
    return -1;
  if (1 != fwrite (p->packet, p->packlen, 1, f))
    return -1;
  return 0;
}