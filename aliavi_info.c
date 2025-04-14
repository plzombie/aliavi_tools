/*
Zero-Clause BSD

Copyright (c) 2021, 2025 Mikhail Morozov
All rights reserved.

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>

typedef struct {
	uint8_t sign[6];
	uint8_t frame_no1;
	uint8_t frame_no2;
	uint8_t frame_no3;
	uint8_t frame_no4;
	uint8_t frame_total1;
	uint8_t frame_total2;
	uint8_t frame_total3;
	uint8_t frame_total4;
	uint8_t size_hi;
	uint8_t size_lo;
	uint8_t unknwn2[8]; // Constant
	uint8_t somestr[8];
	uint8_t unknwn3[6];
	uint8_t audioframe1;
	uint8_t audioframe2;
	uint8_t unknwn4[2];
	uint8_t videoframe1;
	uint8_t videoframe2;
	uint8_t unknwn5[4];
} aliavi_frame_header_t;

bool jpegRetrieveHeader(const unsigned char *buf, size_t buf_size, unsigned char *jpegheader, size_t *jpegheader_size, size_t jpegheader_maxsize);

int wmain(int argc, wchar_t **argv)
{
	FILE *f = 0, *f_wavout = 0;
	aliavi_frame_header_t hdr;
	size_t frame_size, frame_no, total_frames, total_frames_check, audioframe_size, videoframe_size;
	wchar_t *imagefilename = 0;
	size_t imagedirlen, jpegheader_size;
	unsigned char *buf, *jpegheader = 0;
	
	if(argc < 2) {
		wprintf(L"\taliavi_info file.mtv [out.wav] [imagedir]\n");
		return 0;
	}
	
	f = _wfopen(argv[1], L"rb");
	if(!f) {
		wprintf(L"Error: Can't open file\n");
		return 0;
	}
	
	if(argc > 2) {
		f_wavout = _wfopen(argv[2], L"wb");
		if(!f_wavout) {
			wprintf(L"Error: Can't open output audiofile\n");
			return 0;
		}
	}
	
	if(argc > 3) {
		imagedirlen = wcslen(argv[3]);
		imagefilename = malloc((imagedirlen + 1 + 14 + 1)*sizeof(wchar_t));
		if(!imagefilename) {
			wprintf(L"Error: Can't allocate memory\n");
			return 0;
		}
		wcscpy(imagefilename, argv[3]);
		imagefilename[imagedirlen] = '/';
		imagefilename[imagedirlen+1] = 0;
		jpegheader = malloc(65536);
		if(!jpegheader) {
			wprintf(L"Error: Can't allocate memory\n");
			return 0;
		}
	}
	
	total_frames = 0;
	total_frames_check = 0;
	
	buf = malloc(65536);
	if(!buf) {
		wprintf(L"Error: Can't allocate memory\n");
		return 0;
	}
	
	while(1) {
		if(fread(&hdr, sizeof(aliavi_frame_header_t), 1, f) != 1) {
			wprintf(L"Done! Total %lld frames\n", (long long)total_frames);
			break;
		}
		
		frame_size = (size_t)256*hdr.size_hi+hdr.size_lo;
		
		frame_no = hdr.frame_no1*(size_t)16777216+hdr.frame_no2*(size_t)65536+hdr.frame_no3*(size_t)256+hdr.frame_no4;
		
		if(frame_no == 0) total_frames_check = hdr.frame_total1*(size_t)16777216+hdr.frame_total2*(size_t)65536+hdr.frame_total3*(size_t)256+hdr.frame_total4;
		
		audioframe_size = hdr.audioframe1*256+hdr.audioframe2;
		videoframe_size = hdr.videoframe1*256+hdr.videoframe2;
		
		wprintf(L"New frame %lld, size %llx, audsz %llx, vidsz %llx\n", (long long)total_frames, (long long)frame_size, (long long)audioframe_size, (long long)videoframe_size);
		
		if(audioframe_size != 0x410) break;
		
		if(strncmp(hdr.sign, "ALIAVI", 6) != 0) {
			wprintf(L"File broken\n");
			break;
		}
		if(strncmp(hdr.somestr, "00020000", 8) != 0) {
			wprintf(L"Unknown frame\n");
			break;
		}
		if(frame_size < 48) {
			wprintf(L"Wrong frame size\n");
			break;
		}
		if(frame_no != total_frames) {
			wprintf(L"Wrong frame number\n");
			break;
		}
		
		frame_size -= 48;
		if(argc > 2) {
			if(frame_no == 0) audioframe_size += 60;
			if(audioframe_size > 65536) {
				wprintf(L"Error: audioframe is too big\n");
				break;
			}
			
			if(frame_size < audioframe_size) {
				wprintf(L"Error: audioframe size is greater than frame size\n");
				break;
			}
			
			fread(buf, 1, audioframe_size, f);
			fwrite(buf, 1, audioframe_size, f_wavout);
			
			frame_size -= audioframe_size;
		}
		
		if(argc > 3) {
			FILE *f_img;
			
			if(frame_size > 65536) {
				wprintf(L"Error: videoframe is too big\n");
				break;
			}
			
			swprintf(imagefilename+imagedirlen+1, 14+1, L"%u.jpg", (unsigned int)frame_no);
			f_img = _wfopen(imagefilename, L"wb");
			if(!f_img) {
				wprintf(L"Error: can't open output image\n");
				break;
			}
			
			fread(buf, 1, frame_size, f);
			
			if(frame_no == 0) {
				if(!jpegRetrieveHeader(buf, frame_size, jpegheader, &jpegheader_size, 65536)) {
					wprintf(L"Error: can't retrieve jpeg header\n");
					fclose(f_img);
					break;
				}
				
				fwrite(buf, 1, frame_size, f_img);
			} else {
				if(buf[0] != 0xff || buf[1] != 0xd8 || frame_size <= 2) {
					wprintf(L"Error: frame is not a jpeg frame\n");
					fclose(f_img);
					break;
				}
				
				fwrite(jpegheader, 1, jpegheader_size, f_img);
				fwrite(buf+2, 1, frame_size-2, f_img);
			}
			
			fclose(f_img);
			
			frame_size = 0;
		}
		
		fseek(f, (long)frame_size, SEEK_CUR);
		total_frames++;
	}
	
	if(total_frames != total_frames_check)
		wprintf(L"Warning: total frames count differs\n");
	
	fclose(f);
	if(argc > 2) fclose(f_wavout);
	if(argc > 3) {
		free(imagefilename);
		free(jpegheader);
	}
	free(buf);
	
	return 0;
}

bool jpegRetrieveHeader(const unsigned char *buf, size_t buf_size, unsigned char *jpegheader, size_t *jpegheader_size, size_t jpegheader_maxsize)
{
	size_t offset = 0;
	
	if(buf_size <= 2) return false;
	
	while(offset < buf_size) {
		if(buf_size-offset < 2) return false;
		
		if(buf[offset] == 0xff && buf[offset+1] == 0xd8) {
			offset += 2;
			continue;
		} else {
			size_t segment_size;
			bool is_sos = false;
			
			if(buf[offset] == 0xff && buf[offset+1] == 0xda) is_sos = true;
			
			if(buf_size-offset < 4) return false;
			
			offset += 2;
			
			segment_size = buf[offset]*256+buf[offset+1];
			
			if(segment_size < 2 || buf_size-offset < segment_size) return false;
			
			offset += segment_size;
			
			if(is_sos) break;
		}
	}
	
	if(offset >= buf_size || offset > jpegheader_maxsize) return false;
	
	memcpy(jpegheader, buf, offset);
	*jpegheader_size = offset;
	
	return true;
}
