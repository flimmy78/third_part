#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "mp4v2/mp4v2.h"
#include "sps_pps_parser.h"
#include "Mp4Parser.h"

#define NL_LOGE printf

static unsigned const samplingFrequencyTable[16] = {
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0
};

CMP4Parser::CMP4Parser()
	:m_File(NULL),m_TrackId(0),m_FrameRate(0.0),m_Sps(NULL),m_Pps(NULL)
{

}
CMP4Parser::~CMP4Parser()
{
	if(m_File != NULL)
	{
		MP4Close(m_File);
        m_File = NULL;
	}
	if(m_Sps != NULL)
	{
		delete[] m_Sps;
		m_Sps = NULL;
	}
	if(m_Pps != NULL)
	{
		delete[] m_Pps;
		m_Pps = NULL;
	}
}

int CMP4Parser::GetTrackCount()
{
	if(m_File != NULL)
	{
		return MP4GetNumberOfTracks(m_File);
	}
	NL_LOGE("GetTrackCount error\n");
	return -1;
}
int CMP4Parser::GetTrackFormat(int idx)
{

}


void  CMP4Parser::ParserVideoParams()
{
    uint8_t **seqheader;
    uint8_t **pictheader;
	uint32_t *seqheadersize;
    uint32_t *pictheadersize;

    MP4GetTrackH264SeqPictHeaders(m_File, m_TrackId, &seqheader, &seqheadersize, &pictheader,&pictheadersize);

	NL_LOGE("sps %p %d",*seqheader, *seqheadersize);
	for (int ix = 0; ix < *seqheadersize; ix++) {
        NL_LOGE("%x ",(*seqheader)[ix]);
    }
	NL_LOGE("\n");

	NL_LOGE("pps %p %d",*pictheader, *pictheadersize);
	for (int ix = 0; ix < *pictheadersize; ix++) {
        NL_LOGE("%x ",(*pictheader)[ix]);
    }
	NL_LOGE("\n");

	//exit(0);
	if(m_Sps != NULL)
	{
		delete[] m_Sps;
	}
	m_Sps = new uint8_t[(*seqheadersize) + 1];
	memcpy(m_Sps, *seqheader, *seqheadersize);
	m_SpsLen = *seqheadersize;
	
	if(m_Pps != NULL)
	{
		delete[] m_Pps;
	}
	m_Pps = new uint8_t[(*pictheadersize) + 1];
	memcpy(m_Pps, *pictheader, *pictheadersize);
	m_PpsLen = *pictheadersize;

	free(seqheader);
    free(seqheadersize);
	free(pictheader);
    free(pictheadersize);

	GetFrameRate();
}

void  CMP4Parser::ParserAudioParams()
{
	uint8_t *conf;
	uint32_t confsize;
	/*
	MP4GetTrackESConfiguration(m_File, 1, &conf, &confsize);
	NL_LOGE("conf %p %d :",conf, confsize);
	for (int ix = 0; ix < confsize; ix++) {
		NL_LOGE("%x ",(conf)[ix]);
	}
	NL_LOGE("\n");
	*/
	MP4GetTrackESConfiguration(m_File, 2, &conf, &confsize);
	NL_LOGE("conf %p %d :",conf, confsize);
	for (int ix = 0; ix < confsize; ix++) {
		NL_LOGE("%x ",(conf)[ix]);
	}
	NL_LOGE("\n");

	//audioSpecificConfig[0] = ((profile+1) << 3) | (sampling_frequency_index >> 1);
	//audioSpecificConfig[1] = (sampling_frequency_index << 7) | (channel_configuration << 3);
	m_profile = conf[0]>>3 & 0x1F;
	m_channles = conf[1]>>3 & 0xF;
	m_freIdx = ((conf[0] & 0x3) << 1) | (conf[1] >> 7 & 0x1);
	NL_LOGE("profile:%d channles:%d freIdx:%d\n", m_profile, m_channles, m_freIdx);
	/*
	packet[0] = (byte)0xFF;
    packet[1] = (byte)0xF9;
    packet[2] = (byte)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
    packet[3] = (byte)(((chanCfg&3)<<6) + (packetLen>>11));
    packet[4] = (byte)((packetLen&0x7FF) >> 3);
    packet[5] = (byte)(((packetLen&7)<<5) + 0x1F);
    packet[6] = (byte)0xFC;
    */
	free(conf);

	m_FrameRate = (float)samplingFrequencyTable[m_freIdx]/1024.0;
}


int CMP4Parser::SelectTrack(int type)
{
	if(m_File == NULL)
	{
		NL_LOGE("SelectTrack error\n");
		return -1;
	}
	uint32_t numOfTracks = MP4GetNumberOfTracks(m_File);
	NL_LOGE("numOfTracks %d \n", numOfTracks);
	
    for (uint32_t id = 1; id <= numOfTracks; id++) {
        const char* trackType = MP4GetTrackType(m_File, id);
		NL_LOGE("trackType:%s\n",trackType);
		if(type == 0)//video
		{
			if (MP4_IS_VIDEO_TRACK_TYPE(trackType)) {
	            m_TrackId = id;
				m_CurSampleId = 1;
				ParserVideoParams();
	            break;
	        }
		}
		else//audio
		{
			if (MP4_IS_AUDIO_TRACK_TYPE(trackType)) {
	            m_TrackId = id;
				m_CurSampleId = 1;
				ParserAudioParams();
	            break;
	        }
		}
    }
	m_NumSamples = MP4GetTrackNumberOfSamples(m_File, m_TrackId);

	return m_TrackId;
}
bool CMP4Parser::SetDataSource(char* file)
{
	if (m_File != NULL) {
		MP4Close(m_File);
    }
	
	m_File = MP4Read(file);
	if(m_File == MP4_INVALID_FILE_HANDLE)
	{
		NL_LOGE("mp4 file read error");
		return false;
	}
	
	return true;
}
bool CMP4Parser::UnselectTrack(int idx)
{
	m_TrackId = 0;
	m_CurSampleId = 1;
}

bool CMP4Parser::ReadSampleData(unsigned char* data, int& length, long& pts)
{
	if(m_CurSampleId > m_NumSamples)
	{
		return false;
	}
	
	bool isIframe;
    if (!MP4ReadSample(m_File, m_TrackId, m_CurSampleId, (uint8_t**)&data,(uint32_t*)&length, NULL, NULL, NULL, &isIframe)) {
        NL_LOGE("read sampleId %u error\n", m_CurSampleId);

        return false;
    }

	//char nalHeader[] = {0x00, 0x00, 0x00, 0x01};
	//memcpy(data, nalHeader, 4);
	
	pts = MP4GetSampleTime(m_File, m_TrackId, m_CurSampleId);
	pts = pts/m_FrameRate;
	NL_LOGE("read sampleSize %u isIframe:%d pts:%.0f\n", length, isIframe, pts/m_FrameRate);
	m_CurSampleId++;
}
bool CMP4Parser::SeekTo()
{
	return true;
}

float CMP4Parser::GetFrameRate()
{
	
	if(m_Sps == NULL)
	{
		NL_LOGE("m_Sps == NULL\n");
		//exit(0);
		return 0.0;
	}
	
	float fps = 0.0;
	get_bit_context buffer;
	memset(&buffer, 0, sizeof(get_bit_context));
	SPS _sps;
	buffer.buf = m_Sps + 1;
	buffer.buf_size = m_SpsLen-1;
	int ret = h264dec_seq_parameter_set(&buffer, &_sps);
	int m_nWidth = h264_get_width(&_sps);
	int m_nHeight = h264_get_height(&_sps);
	ret = h264_get_framerate(&fps, &_sps);
	NL_LOGE("m_nWidth:%d m_nHeight:%d fps:%f\n",m_nWidth, m_nHeight, fps);
	//exit(0);
	if (ret == 0)
	{
		m_FrameRate = fps;
	}
	return fps;
	
}

bool CMP4Parser::GetSpsAndPps(u_int8_t* &sps, uint32_t& spsLen, u_int8_t* &pps,uint32_t& ppsLen)
{
	if(m_Sps == NULL || m_Pps == NULL)
	{
		return false;
	}
	else
	{
		sps = m_Sps;
		pps = m_Pps; 
		spsLen = m_SpsLen;
		ppsLen = m_PpsLen;
	}
	return true;
}


int main(int argc, char* argv[])
{
	char nalHeader[] = {0x00, 0x00, 0x00, 0x01};
	FILE* h264File = NULL;
	FILE* aacFile = NULL;
	h264File = fopen("S06.264", "wb+");
	aacFile = fopen("S06.aac", "wb+");
	//demuxH264FromMp4();
	CMP4Parser* parser = new CMP4Parser();

	parser->SetDataSource("S06.mp4");
	int ret = parser->SelectTrack(0);
	if(ret <= 0)
	{
		exit(0);
	}

	//*
	u_int8_t* sps;
	uint32_t spsLen;
	u_int8_t* pps;
	uint32_t ppsLen;
	bool r = parser->GetSpsAndPps(sps, spsLen, pps, ppsLen);
	if(r)
	{
		fwrite(nalHeader, 4, 1, h264File);//头四个字节是nal长度， 用0 0 0 1替换
        fwrite(sps, spsLen, 1, h264File);
		fwrite(nalHeader, 4, 1, h264File);//头四个字节是nal长度， 用0 0 0 1替换
        fwrite(pps, ppsLen, 1, h264File);
	}

	unsigned char* data = new unsigned char[512*1024];
	int length = 512*1024;
	long pts;
	while(parser->ReadSampleData(data,length,pts))
	{
		
		int offset = 0;
		int type = data[4] & 0x1f;
		
		
		if(type != 5 && type != 1)
		{
			int _len = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
			NL_LOGE("_len:%x _len:%d data[0]:%x data[1]:%x data[2]:%x data[3]:%x\n",_len, _len, data[0], data[1], data[2], data[3]);
			offset = _len + 4;
		}
		
		fwrite(nalHeader, 4, 1, h264File);//头四个字节是nal长度， 用0 0 0 1替换
        fwrite(data + offset + 4, length - offset - 4, 1, h264File);
        
		//fwrite(data, length, 1, h264File);
		/*
		if(pts > 8200.0)
		{
			NL_LOGE("pts:%d\n",pts);
			exit(0);
		}
		*/
		length = 512*1024;
	}
	free(data);
	exit(0);
	//*/
	ret = parser->SelectTrack(1);
	uint8_t profile = parser->GetAudioProfile();
	uint8_t channles = parser->GetAudioChannles();
	uint8_t freIdx = parser->GetAudioFreIdx();
	
	unsigned char* data_a = new unsigned char[2*1024];
	int alength = 2*1024;
	long apts;
	uint8_t packet[7];
	while(parser->ReadSampleData(data_a,alength,apts))
	{
		int _len = alength + 7;
		packet[0] = (uint8_t)0xFF;
	    packet[1] = (uint8_t)0xF9;
	    packet[2] = (uint8_t)(((profile-1)<<6) + (freIdx<<2) +(channles>>2));
	    packet[3] = (uint8_t)(((channles&3)<<6) + (_len>>11));
	    packet[4] = (uint8_t)((_len&0x7FF) >> 3);
	    packet[5] = (uint8_t)(((_len&7)<<5) + 0x1F);
	    packet[6] = (uint8_t)0xFC;
		
        fwrite(packet, 7, 1, aacFile);
		fwrite(data_a, alength, 1, aacFile);

		alength = 2*1024;
	}
	free(data_a);
	
	delete parser;
	fclose(h264File);
	fclose(aacFile);
    h264File = NULL;
	aacFile = NULL;
		
	return 0;
}
