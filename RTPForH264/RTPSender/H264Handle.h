#pragma once
#include <stdio.h>

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

class CH264Handle
{
public:
	CH264Handle(char* pFile = "test.264");
	~CH264Handle(void);

	static int FindStartCode2 (unsigned char *Buf);
	static int FindStartCode3 (unsigned char *Buf);

	static int GetAnnexbFrame (unsigned char *frame, int &length);
	static int GetAnnexbNALU (unsigned char *frame, int &length, NALU_t *nalu);

	static int GetAnnexbNALU (NALU_t *nalu);
	
	bool BitStreamFeof();
	static NALU_t *AllocNALU(int buffersize);
	static void FreeNALU(NALU_t *n);
	static void OpenBitstreamFile (char *fn);
	static void SaveSPS (unsigned char *sps, int len);
	static void SavePPS (unsigned char *pps, int len);
	static void GetSPS (unsigned char* &sps, int& len);
	static void GetPPS (unsigned char* &pps, int& len);
private:

public:
	static int info2;
	static int info3;
	static unsigned char m_Sps[64];
	static int m_SpsLen;
	static unsigned char m_Pps[64];
	static int m_PpsLen;
};

