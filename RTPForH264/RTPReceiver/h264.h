// MPEG2RTP.h
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>

#include <winsock2.h>

//#include "mem.h"

#define PACKET_BUFFER_END            (unsigned int)0x00000000


#define MAX_RTP_PKT_LENGTH     1400
#define MAXRTPPACKETSIZE  (65536 - 28)    //!< Maximum size of an RTP packet incl. header
#define MAXH264NALUSIZE		(8000)

#define DEST_IP                "172.20.174.237"
#define DEST_PORT              1234

#define H264                   96

typedef unsigned short uint16;   //!< type definition for unsigned short (16 bits)
typedef __int64   int64;
/*
#if (defined(WIN32) || defined(WIN64)) && !defined(__GNUC__)
typedef __int64   int64;
typedef unsigned __int64   uint64;
# define FORMAT_OFF_T "I64d"
# ifndef INT64_MIN
#  define INT64_MIN        (-9223372036854775807i64 - 1i64)
# endif
#else
typedef long long int64;
typedef unsigned long long  uint64;
# define FORMAT_OFF_T "lld"
# ifndef INT64_MIN
#  define INT64_MIN        (-9223372036854775807LL - 1LL)
# endif
#endif
*/

# define  open     _open
# define  close    _close
# define  read     _read
# define  write    _write
# define  lseek    _lseeki64
# define  fsync    _commit
# define  tell     _telli64

typedef struct 
{
    /* byte 0 */
    unsigned char csrc_len:4;        /* expect 0 */
    unsigned char extension:1;       /* expect 1, see RTP_OP below */
    unsigned char padding:1;         /* expect 0 */
    unsigned char version:2;         /* expect 2 */
    /* byte 1 */
    unsigned char payload:7;         /* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;          /* expect 1 */
    /* bytes 2,3 */
    unsigned short seq_no;            
    /* bytes 4-7 */
    unsigned  long timestamp;        
    /* bytes 8-11 */
    unsigned long ssrc;              /* stream number is used here. */
} RTP_FIXED_HEADER;

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;        
} NALU_HEADER; /* 1 BYTE */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;              
} FU_INDICATOR; /* 1 BYTE */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER;   /* 1 BYTES */

typedef struct
{
  unsigned int v;          //!< Version, 2 bits, MUST be 0x2
  unsigned int p;          //!< Padding bit, Padding MUST NOT be used
  unsigned int x;          //!< Extension, MUST be zero
  unsigned int cc;         /*!< CSRC count, normally 0 in the absence
                                of RTP mixers */
  unsigned int m;          //!< Marker bit
  unsigned int pt;         //!< 7 bits, Payload Type, dynamically established
  uint16       seq;        /*!< RTP sequence number, incremented by one for
                                each sent packet */
  unsigned int timestamp;  //!< timestamp, 27 MHz for H.264
  unsigned int ssrc;       //!< Synchronization Source, chosen randomly
  byte *       payload;    //!< the payload including payload headers
  unsigned int paylen;     //!< length of payload in bytes
  byte *       packet;     //!< complete packet including header and payload
  unsigned int packlen;    //!< length of packet, typically paylen+12
} RTPpacket_t;

BOOL InitWinsock();
void DumpRTPHeader (RTPpacket_t *p);
int WriteRTPPacket (RTPpacket_t *p, FILE *f);
int DecomposeRTPpacket (RTPpacket_t *p);
void DumpRTPHeader (RTPpacket_t *p);