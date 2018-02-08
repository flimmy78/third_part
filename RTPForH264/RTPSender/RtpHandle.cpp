#include "RtpHandle.h"
#include "stdio.h"

unsigned short CRtpHandle::seq_num = 0;

static FILE* fH264 = NULL;
static char startBit[4] = {0x00, 0x00, 0x00, 0x01};
static void WriteH264(char *data, int len, int addStartBit)
{
	if(fH264 == NULL)
	{
		fH264 = fopen("NALU_H264.264","wb");
		printf("=====>>>>> open file\n");
	}
	if(data == NULL)
	{
		fclose(fH264);
		printf("=====>>>>> close file\n");
	}
	else
	{
		if(addStartBit != 0)
		{
			fwrite(startBit, 1, 4, fH264);
		}
		fwrite(data, 1, len, fH264);
		printf("=====>>>>> fwrite file n->len:%d\n",len);
	}
}


CRtpHandle::CRtpHandle(void)
{
	ts_current = 0;
	total_bytes=0;
	total_sent=0;
}


CRtpHandle::~CRtpHandle(void)
{
}

BOOL CRtpHandle::InitiateWinsock()
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

BOOL CRtpHandle::Connet(char* ip, int port)
{
	struct sockaddr_in server;
	int len =sizeof(server);

	server.sin_family=AF_INET;									//AF_INET define internet protocal
	server.sin_port=htons(port);								//Assign port number to socket          
	server.sin_addr.s_addr=inet_addr(ip);						//Assign ip address to socket 

	socket1=socket(AF_INET,SOCK_DGRAM,0);
	connect(socket1, (const SOCKADDR*)&server, len) ;           //Connect to server socket

	return true;
}

BOOL CRtpHandle::PacketSend(NALU_t *n, float framerate)
{
	NALU_HEADER* nalu_hdr;
	FU_INDICATOR* fu_ind;
	FU_HEADER* fu_hdr;
	char* nalu_payload;  
	int	bytes=0;

	timestamp_increse = (unsigned int)(90000.0 / framerate);      //+0.5);

	memset(sendbuf,0,1500);									//Clear sendbuf;the previous timestamp will also be cleared，so we need ts_current to store previous timestamp
	RTP_FIXED_HEADER* rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];                //rtp\ fixed header, it's 12 bytes long,here we assign the address of sendbuf to rtp_hdr so that when we write
		                                                    //the header information to rtp_hdr,it auomatically stored in sendbuf.
	rtp_hdr->payload     = H264;							//Payload type
	rtp_hdr->version     = 2;                               //Payload version
	rtp_hdr->marker      = 0;							    //Marker sign
	rtp_hdr->ssrc        = htonl(10);					    //Randomly set to 10,it is unique in this rtp session

	//When NALU is less than 1400 bytes，send one RTP packet
	if(n->len<=1400)
	{	
		total_bytes+=n->len;
		rtp_hdr->marker = 1;
		rtp_hdr->seq_no     = htons(seq_num++);            //Sequence number,it will increase one each sending
		nalu_hdr =(NALU_HEADER*)&sendbuf[12];               //Assign address of sendbuf[12] to nalu_hdr，the value of nalu_hdr will be stored in sendbuf
		nalu_hdr->F=n->forbidden_bit;
		nalu_hdr->NRI=n->nal_reference_idc>>5;              //The useful data is in 6,7 bit of n->nal_reference_idc, so we need to shif right 5 bits to assign it to nalu_hdr->NRI。
		nalu_hdr->TYPE=n->nal_unit_type;

		nalu_payload=&sendbuf[13];                          //Assign sendbuf[13] to nalu_payload
		memcpy(nalu_payload,n->buf+1,n->len-1);             //Drop nalu header, store the rest nalu to sendbuf[13]。
		ts_current=ts_current+timestamp_increse;
		rtp_hdr->timestamp=htonl(ts_current);
		bytes=n->len + 12 ;						           //sendbuf length, consist of NALU length (including NALU header but excluding start prefix) plus rtp_header 12 bytes
		send( socket1, sendbuf, bytes, 0 );                //Send RTP packet	
		//WriteH264(n->buf, 1, 1);
		//WriteH264(n->buf+1, n->len-1, 0);
		total_sent+=bytes;
		Sleep(1);
	}	
	else if(n->len>1400)
	{
		int k=0,l=0;
		int t=0;                                           //Index of segment of RTP packet
		k=n->len/1400;                                     //Number of RTP packets needed
		l=n->len%1400;                                     //The length of data in the last RTP packet
		total_bytes+=n->len;
		ts_current=ts_current+timestamp_increse;
		rtp_hdr->timestamp=htonl(ts_current);
		while(t<=k)
		{
			rtp_hdr->seq_no = htons(seq_num ++);           //Sequence number,it will increase one each sending
			//first 
			if(!t)
			{
				rtp_hdr->marker=0;
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F=n->forbidden_bit;
				fu_ind->NRI=n->nal_reference_idc>>5;
				fu_ind->TYPE=28;

				fu_hdr =(FU_HEADER*)&sendbuf[13];
				fu_hdr->E=0;
				fu_hdr->R=0;
				fu_hdr->S=1;
				fu_hdr->TYPE=n->nal_unit_type;

				nalu_payload=&sendbuf[14];
				memcpy(nalu_payload,n->buf+1,1400);        //Drop NALU header
				bytes=1400+14;						       //sendbuf length, consist of NALU length (including NALU header but excluding start prefix) plus rtp_header->fu_ind->fu_hdr 14 bytes
				send( socket1, sendbuf, bytes, 0 );
				//WriteH264(n->buf, 1, 1);
				//WriteH264(n->buf+1, 1400, 0);
				t++;
				total_sent+=bytes;
			}
			else if(k==t)//发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）。
			{
				rtp_hdr->marker=1;
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; 
				fu_ind->F=n->forbidden_bit;
				fu_ind->NRI=n->nal_reference_idc>>5;
				fu_ind->TYPE=28;

				fu_hdr =(FU_HEADER*)&sendbuf[13];
				fu_hdr->R=0;
				fu_hdr->S=0;
				fu_hdr->TYPE=n->nal_unit_type;
				fu_hdr->E=1;

				nalu_payload=&sendbuf[14];
				memcpy(nalu_payload,n->buf+t*1400+1,l-1);//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。
				bytes=l-1+14;		//获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节
				send( socket1, sendbuf, bytes, 0 );
				//WriteH264(n->buf+t*1400+1, l-1, 0);
				t++;
				printf("Last seg length : %d\n",bytes);
				total_sent+=bytes;
			}
			else if(t<k&&0!=t)
			{
				rtp_hdr->marker=0;
				fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F=n->forbidden_bit;
				fu_ind->NRI=n->nal_reference_idc>>5;
				fu_ind->TYPE=28;

				fu_hdr =(FU_HEADER*)&sendbuf[13];
				//fu_hdr->E=0;
				fu_hdr->R=0;
				fu_hdr->S=0;
				fu_hdr->E=0;
				fu_hdr->TYPE=n->nal_unit_type;

				nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload
				memcpy(nalu_payload,n->buf+t*1400+1,1400);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。
				bytes=1400+14;						//获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节
				send( socket1, sendbuf, bytes, 0 );
				//WriteH264(n->buf+t*1400+1, 1400, 0);
				t++;
				total_sent+=bytes;
			}
			Sleep(1);
		}
		printf("Total bytes : %d\n",total_bytes);
	}
	printf("Total bytes : %d\n",total_bytes);
	return true;
}