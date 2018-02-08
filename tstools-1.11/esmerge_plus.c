/*
 * Merge a video ES and an audio ES to produce TS.
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the MPEG TS, PS and ES tools.
 *
 * The Initial Developer of the Original Code is Amino Communications Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Amino Communications Ltd, Swavesey, Cambridge UK
 *
 * ***** END LICENSE BLOCK *****
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "compat.h"
#include "es_fns.h"
#include "accessunit_fns.h"
#include "avs_fns.h"
#include "audio_fns.h"
#include "ts_fns.h"
#include "tswrite_fns.h"
#include "misc_fns.h"
#include "version.h"

// Default audio rates, in Hertz
#define CD_RATE  44100
#define DAT_RATE 48000

// Video frame rate (frames per second)
#define DEFAULT_VIDEO_FRAME_RATE  30

// Number of audio samples per frame
// For ADTS this will either be 1024 or 960. It's believed that it will
// actually, in practice, be 1024, and in fact the difference may not be
// significant enough to worry about for the moment.
#define ADTS_SAMPLES_PER_FRAME  1024
// For MPEG-1 audio layer 2, this is 1152
#define L2_SAMPLES_PER_FRAME    1152
// For AC-3 this is 256 * 6
#define AC3_SAMPLES_PER_FRAME   (256 * 6)

// ------------------------------------------------------------
#define TEST_PTS_DTS 0

#if TEST_PTS_DTS
#include "pes_fns.h"

static int check(uint64_t value)
{
	int      err;
	byte     data[5];
	uint64_t result;

	encode_pts_dts(data, 2, value);
	err = decode_pts_dts(data, 2, &result);
	if (err) return 1;

	if (value == result)
		printf("Value " LLU_FORMAT " OK\n", value);
	else
	{
		printf("Input " LLU_FORMAT ", output " LLU_FORMAT "\n", value, result);
		return 1;
	}

	return 0;

}

static int test_pts()
{
	if (check(0)) return 1;
	if (check(1)) return 1;
	if (check(2)) return 1;
	if (check(3)) return 1;
	if (check(4)) return 1;
	if (check(5)) return 1;
	if (check(6)) return 1;
	if (check(7)) return 1;
	if (check(8)) return 1;
	if (check(100)) return 1;
	if (check(10000)) return 1;
	if (check(1000000)) return 1;
	if (check(100000000)) return 1;
	if (check(10000000000LL)) return 1;
	if (check(1000000000000LL)) return 1;

	return 0;
}
#endif  // TEST_PTS_DTS

static int is_avs_I_frame(avs_frame_p  frame)
{
	return (frame->is_frame && frame->start_code == 0xB3);
}

static int is_I_or_IDR_frame(access_unit_p  frame)
{
	return (frame->primary_start != NULL &&
		frame->primary_start->nal_ref_idc != 0 &&
		(frame->primary_start->nal_unit_type == NAL_IDR ||
		all_slices_I(frame)));
}

/*
 * Merge the given elementary streams to the given output.
 *
 * Returns 0 if all goes well, 1 if something goes wrong.
 */
static int merge_with_h264(access_unit_context_p  video_context,
	int(*callbackfun)(char*, int*),
	TS_writer_p            output,
	int                    audio_type,
	int                    audio_samples_per_frame,
	int                    audio_sample_rate,
	int                    video_frame_rate,
	int                    pat_pmt_freq,
	int                    quiet,
	int                    verbose,
	int                    debugging)
{
	int  ii;
	int  err;
	uint32_t prog_pids[2];
	byte     prog_type[2];

	int video_frame_count = 0;
	int audio_frame_count = 0;

	uint32_t video_pts_increment = 90000 / video_frame_rate;
	uint32_t audio_pts_increment = (90000 * audio_samples_per_frame) / audio_sample_rate;
	uint64_t video_pts = 0;
	uint64_t audio_pts = 0;

	// The "actual" times are just for information, so we aren't too worried
	// about accuracy - thus floating point should be OK.
	double audio_time = 0.0;
	double video_time = 0.0;

	int got_video = TRUE;
	int got_audio = TRUE;

	if (verbose)
		printf("Video PTS increment %u\n"
		"Audio PTS increment %u\n", video_pts_increment, audio_pts_increment);

	// Start off our output with some null packets - this is in case the
	// reader needs some time to work out its byte alignment before it starts
	// looking for 0x47 bytes
	for (ii = 0; ii < 8; ii++)
	{
		err = write_TS_null_packet(output);
		if (err) return 1;
	}

	// Then write some program data
	// @@@ later on we might want to repeat this every so often
	prog_pids[0] = DEFAULT_VIDEO_PID;
	prog_pids[1] = DEFAULT_AUDIO_PID;
	prog_type[0] = AVC_VIDEO_STREAM_TYPE;

	switch (audio_type)
	{
	case AUDIO_ADTS:
	case AUDIO_ADTS_MPEG2:
	case AUDIO_ADTS_MPEG4:
		prog_type[1] = ADTS_AUDIO_STREAM_TYPE;
		break;
	case AUDIO_L2:
		prog_type[1] = MPEG2_AUDIO_STREAM_TYPE;
		break;
	case AUDIO_AC3:
		prog_type[1] = ATSC_DOLBY_AUDIO_STREAM_TYPE;
		break;
	default:              // what else can we do?
		prog_type[1] = ADTS_AUDIO_STREAM_TYPE;
		break;
	}
	err = write_TS_program_data2(output,
		1, // transport stream id
		1, // program number
		DEFAULT_PMT_PID,
		DEFAULT_VIDEO_PID,  // PCR pid
		2, prog_pids, prog_type);
	if (err)
	{
		fprintf(stderr, "### Error writing out TS program data\n");
		return 1;
	}

	while (got_video
		|| got_audio
		)
	{
		access_unit_p  access_unit;
		audio_frame_p  aframe;

		// Start with a video frame
		if (got_video)
		{
			err = get_next_h264_frame(video_context, quiet, debugging, &access_unit);
			if (err == EOF)
			{
				if (verbose)
					fprintf(stderr, "EOF: no more video data\n");
				got_video = FALSE;
			}
			else if (err)
			{
				fprintf(stderr, "EOF: no more video data return 1;\n");
				return 1;
			}

		}

		if (got_video)
		{
			video_time = video_frame_count / (double)video_frame_rate;
			video_pts += video_pts_increment;
			video_frame_count++;
			if (verbose)
				printf("\n%s video frame %5d (@ %.2fs, " LLU_FORMAT ")\n",
				(is_I_or_IDR_frame(access_unit) ? "**" : "++"),
				video_frame_count, video_time, video_pts);

			if (pat_pmt_freq && !(video_frame_count % pat_pmt_freq))
			{
				if (verbose)
				{
					printf("\nwriting PAT and PMT (frame = %d, freq = %d).. ",
						video_frame_count, pat_pmt_freq);
				}
				err = write_TS_program_data2(output,
					1, // tsid
					1, // Program number
					DEFAULT_PMT_PID,
					DEFAULT_VIDEO_PID, // PCR pid
					2, prog_pids, prog_type);
			}


			// PCR counts frames as seen in the stream, so is easy
			// The presentation and decoding time for B frames (if we ever get any)
			// could reasonably be the same as the PCR.
			// The presentation and decoding time for I and IDR frames is unlikely to
			// be the same as the PCR (since frames come out later...), but it may
			// work to pretend the PTS is the PCR plus a delay time (for decoding)...

			// We could output the timing information every video frame,
			// but might as well only do it on index frames.
			if (is_I_or_IDR_frame(access_unit))
				err = write_access_unit_as_TS_with_pts_dts(access_unit, video_context,
				output, DEFAULT_VIDEO_PID,
				TRUE, video_pts + 45000,
				TRUE, video_pts);
			else
				err = write_access_unit_as_TS_with_PCR(access_unit, video_context,
				output, DEFAULT_VIDEO_PID,
				video_pts, 0);
			if (err)
			{
				free_access_unit(&access_unit);
				fprintf(stderr, "### Error writing access unit (frame)\n");
				return 1;
			}
			free_access_unit(&access_unit);

			// Did the logical video stream end after the last access unit?
			if (video_context->end_of_stream)
			{
				if (verbose)
					printf("Found End-of-stream NAL unit\n");
				got_video = FALSE;
			}
		}
		//continue;

		if (!got_audio || callbackfun == NULL)
			continue;

		// Then output enough audio frames to make up to a similar time
		while (audio_pts < video_pts || !got_video)
		{
			//err = read_next_audio_frame(audio_file,audio_type,&aframe);
			char aframe_buf[1024];
			int ret = callbackfun(aframe_buf, 1024);
			fprintf(stderr, "callbackfun audio data ret:%d\n", ret);
			if (ret <= 0)
			{
				got_audio = FALSE;
				break;
			}

			audio_time = audio_frame_count *
				audio_samples_per_frame / (double)audio_sample_rate;
			audio_pts += audio_pts_increment;
			audio_frame_count++;
			if (verbose)
				printf("** audio frame %5d (@ %.2fs, " LLU_FORMAT ")\n",
				audio_frame_count, audio_time, audio_pts);

			err = write_ES_as_TS_PES_packet_with_pts_dts(output, aframe_buf,
				ret,
				DEFAULT_AUDIO_PID,
				DEFAULT_AUDIO_STREAM_ID,
				TRUE, audio_pts,
				TRUE, audio_pts);
			if (err)
			{
				return 1;
			}
		}
	}

	if (!quiet)
	{
		uint32_t video_elapsed = 100 * video_frame_count / video_frame_rate;
		uint32_t audio_elapsed = 100 * audio_frame_count*
			audio_samples_per_frame / audio_sample_rate;
		printf("Read %d video frame%s, %.2fs elapsed (%dm %.2fs)\n",
			video_frame_count, (video_frame_count == 1 ? "" : "s"),
			video_elapsed / 100.0, video_elapsed / 6000, (video_elapsed % 6000) / 100.0);
		printf("Read %d audio frame%s, %.2fs elapsed (%dm %.2fs)\n",
			audio_frame_count, (audio_frame_count == 1 ? "" : "s"),
			audio_elapsed / 100.0, audio_elapsed / 6000, (audio_elapsed % 6000) / 100.0);
	}

	return 0;

}

static void print_usage()
{
	printf(
		"Usage:\n"
		"    esmerge <video-file> <audio-file> <output-file>\n"
		"\n"
		);
	REPORT_VERSION("esmerge");
	printf(
		"\n"
		"  Merge the contents of two Elementary Stream (ES) files, one containing\n"
		"  video data, and the other audio, to produce an output file containing\n"
		"  Transport Stream (TS).\n"
		"\n"
		"Files:\n"
		"  <video-file>  is the ES file containing video.\n"
		"  <audio-file>  is the ES file containing audio.\n"
		"  <output-file> is the resultant TS file.\n"
		"\n"
		"Switches:\n"
		"  -quiet, -q        Only output error messages.\n"
		"  -verbose, -v      Output information about each audio/video frame.\n"
		"  -x                Output diagnostic information.\n"
		"\n"
		"  -h264             The video stream is H.264 (the default)\n"
		"  -avs              The video stream is AVS\n"
		"\n"
		"  -vidrate <hz>     Video frame rate in Hz - defaults to 25Hz.\n"
		"\n"
		"  -rate <hz>        Audio sample rate in Hertz - defaults to 44100, i.e., 44.1KHz.\n"
		"  -cd               Equivalent to -rate 44100 (CD rate), the default.\n"
		"  -dat              Equivalent to -rate 48000 (DAT rate).\n"
		"\n"
		"  -adts             The audio stream is ADTS (the default)\n"
		"  -l2               The audio stream is MPEG layer 2 audio\n"
		"  -mp2adts          The audio stream is MPEG-2 style ADTS regardless of ID bit\n"
		"  -mp4adts          The audio stream is MPEG-4 style ADTS regardless of ID bit\n"
		"  -ac3              The audio stream is Dolby AC-3 in ATSC\n"
		"\n"
		"  -patpmtfreq <f>    PAT and PMT will be inserted every <f> video frames. \n"
		"                     by default, f = 0 and PAT/PMT are inserted only at  \n"
		"                     the start of the output stream.\n"
		"\n"
		"Limitations\n"
		"===========\n"
		"For the moment, the video input must be H.264 or AVS, and the audio input\n"
		"ADTS, AC-3 ATSC or MPEG layer 2. Also, the audio is assumed to have a\n"
		"constant number of samples per frame.\n"
		);
}

#include <fcntl.h>
int video_fd_r = -1;
int video_fd_w = -1;
int audio_fd_r = -1;

char* read_file = NULL;
char* write_file = NULL;
char* audio_read_file = NULL;

int video_write_func(char* data, int len)
{
	if (video_fd_w == -1)
	{
		int flags = 0;
		flags = flags | O_WRONLY | O_CREAT | O_TRUNC;
		video_fd_w = open(write_file, flags, 00777);

		if (video_fd_w == -1)
		{
			fprintf(stderr, "### Error opening file %s %s\n", write_file, strerror(errno));
		}
		else
		{
			fprintf(stderr, "### opening file %s %d\n", write_file, video_fd_w);
		}
	}

	int _len = write(video_fd_w, data, len);
	if (_len == len)
	{
		//fprintf(stderr,"====>>> video_write_func _len:%d len:%d\n",_len,len);
	}
	else
	{
		//fprintf(stderr,"### error write file %s %s\n","test.ts",strerror(errno));	
	}

}

int audio_read_func(char* data, int len)
{
	if (audio_fd_r == -1)
	{
		int flags = 0;
		flags = flags | O_RDONLY;
		audio_fd_r = open(audio_read_file, flags);

		if (audio_fd_r == -1)
		{
			fprintf(stderr, "###audio_read_func Error opening file %s %s\n", audio_read_file, strerror(errno));
		}
		else
		{
			fprintf(stderr, "###audio_read_func opening file %s %d\n", audio_read_file, audio_fd_r);
		}
	}

	unsigned char aac_header[7];
	int true_size = 0;

	true_size = read(audio_fd_r, aac_header, 7);
	if (true_size <= 0)
	{
		return 0;
	}
	else
	{
		int frame_length = ((aac_header[3] & 0x03) << 11) | (aac_header[4] << 3) |
      						((unsigned)(aac_header[5] & 0xE0) >> 5);

		int ii;
		for (ii=0; ii<6; ii++)
    		data[ii] = aac_header[ii];

		true_size = read(audio_fd_r, &(data[7]), frame_length - 7);
		return frame_length;
	}

}

int video_read_func(char* data, int* len)
{
	if (video_fd_r == -1)
	{
		int flags = 0;
		flags = flags | O_RDONLY;
		video_fd_r = open(read_file, flags);

		if (video_fd_r == -1)
		{
			fprintf(stderr, "### Error opening file %s %s\n", read_file, strerror(errno));
		}
		else
		{
			fprintf(stderr, "### opening file %s %d\n", read_file, video_fd_r);
		}
	}
	*len = read(video_fd_r, data, ES_READ_AHEAD_SIZE);
	if (*len > 0)
	{
		//fprintf(stderr,"====>>> callreadfun len:%d\n",*len);
	}
	else if (*len < 0)
	{
		//fprintf(stderr,"### error read file %s %s\n","test.264",strerror(errno));	
	}
	return *len;
}

int main(int argc, char **argv)
{
	int    had_video_name = FALSE;
	int    had_audio_name = FALSE;
	int    had_output_name = FALSE;
	char  *video_name = NULL;
	char  *audio_name = NULL;
	char  *output_name = NULL;
	int    err = 0;
	ES_p   video_es = NULL;
	access_unit_context_p h264_video_context = NULL;
	avs_context_p avs_video_context = NULL;
	int    audio_file = -1;
	TS_writer_p output = NULL;
	int    quiet = FALSE;
	int    verbose = FALSE;
	int    debugging = FALSE;
	int    audio_samples_per_frame = ADTS_SAMPLES_PER_FRAME;
	int    audio_sample_rate = DAT_RATE;
	int    video_frame_rate = DEFAULT_VIDEO_FRAME_RATE;
	int    audio_type = AUDIO_ADTS;
	int    video_type = VIDEO_H264;
	int    pat_pmt_freq = 0;
	int    ii = 1;

#if TEST_PTS_DTS
	test_pts();
	return 0;
#endif

	if (argc < 2)
	{
		print_usage();
		return 0;
	}

	video_type = VIDEO_H264;
	read_file = video_name = argv[1];
	audio_read_file = audio_name = argv[2];
	write_file = output_name = argv[3];
	fprintf(stderr, "### esmerge: video_name:%s \n", video_name);
	fprintf(stderr, "### esmerge: audio_name:%s \n", audio_name);
	fprintf(stderr, "### esmerge: output_name:%s\n", output_name);

	//err = open_elementary_stream(video_name,&video_es);
	err = open_elementary_stream_ex(video_read_func, &video_es);
	if (err)
	{
		fprintf(stderr, "### esmerge: "
			"Problem starting to read video as ES - abandoning reading\n");
		return 1;
	}

	if (video_type == VIDEO_H264)
	{
		err = build_access_unit_context(video_es, &h264_video_context);
		if (err)
		{
			fprintf(stderr, "### esmerge: "
				"Problem starting to read video as H.264 - abandoning reading\n");
			close_elementary_stream(&video_es);
			return 1;
		}
	}
	else
	{
		fprintf(stderr, "### esmerge: Unknown video type\n");
		return 1;
	}

	//======================================================================
	//  audio_file = open_binary_file(audio_name,FALSE);
	//  if (audio_file == -1)
	//  {
	//    fprintf(stderr,"### esmerge: "
	//            "Problem opening audio file - abandoning reading\n");
	//    close_elementary_stream(&video_es);
	//    free_access_unit_context(&h264_video_context);
	//    free_avs_context(&avs_video_context);
	//    return 1;
	//  }
	//======================================================================

	//err = tswrite_open(TS_W_FILE,output_name,NULL,0,quiet,&output);
	err = tswrite_open_ex(TS_W_CALL, video_write_func, NULL, 0, quiet, &output);
	if (err)
	{
		fprintf(stderr, "### esmerge: "
			"Problem opening output file %s - abandoning reading\n",
			output_name);
		close_elementary_stream(&video_es);
		close_file(audio_file);
		free_access_unit_context(&h264_video_context);
		free_avs_context(&avs_video_context);
		return 1;
	}

	switch (audio_type)
	{
	case AUDIO_ADTS:
		audio_samples_per_frame = ADTS_SAMPLES_PER_FRAME;
		break;
	default:              // hmm - or we could give up...
		audio_samples_per_frame = ADTS_SAMPLES_PER_FRAME;
		break;
	}

	if (!quiet)
	{
		printf("Reading video from %s\n", video_name);
		printf("Writing output to  %s\n", output_name);
		printf("Video frame rate: %dHz\n", video_frame_rate);
	}

	if (video_type == VIDEO_H264)
		
		err = merge_with_h264(h264_video_context, audio_read_func, output,
		//err = merge_with_h264(h264_video_context, read_next_adts_frame, output,
		AUDIO_ADTS_MPEG2,
		audio_samples_per_frame, audio_sample_rate,
		video_frame_rate,
		pat_pmt_freq,
		quiet, verbose, debugging);
	else
	{
		printf("### esmerge: Unknown video type\n");
		return 1;
	}
	if (err)
	{
		printf("### esmerge: Error merging video and audio streams\n");
		close_elementary_stream(&video_es);
		free_access_unit_context(&h264_video_context);
		(void)tswrite_close(output, quiet);
		return 1;
	}

	close_elementary_stream(&video_es);
	free_access_unit_context(&h264_video_context);
	err = tswrite_close(output, quiet);
	if (err)
	{
		printf("### esmerge: Error closing output %s\n", output_name);
		return 1;
	}
	return 0;
}

