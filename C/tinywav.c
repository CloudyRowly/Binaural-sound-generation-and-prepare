/**
 * Copyright (c) 2015-2023, Martin Roth (mhroth@gmail.com)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * 
 * Copyright (c) 2024 - Added binaural sound computation
 * by Gia Minh Nguyen (Giaminhnguyen.2004@gmail.com) (u7556893@anu.edu.au)
 */

#include <string.h> // for memcpy
#if _WIN32
#include <malloc.h> // for alloca
#else
#include <alloca.h>
#endif
#include "tinywav.h"
#include <math.h>
#include <stdio.h>

/** @returns true if the chunk of 4 characters matches the supplied string */
static bool chunkIDMatches(char chunk[4], const char* chunkName)
{
  for (int i=0; i<4; ++i) {
    if (chunk[i] != chunkName[i]) {
      return false;
    }
  }
  return true;
}

int tinywav_open_write(TinyWav *tw, int16_t numChannels, int32_t samplerate, TinyWavSampleFormat sampFmt,
                       TinyWavChannelFormat chanFmt, const char *path) {
  
  if (tw == NULL || path == NULL || numChannels < 1 || samplerate < 1) {
    return -1;
  }
  
#if _WIN32
  errno_t err = fopen_s(&tw->f, path, "wb");
  if (err != 0) { tw->f == NULL; }
#else
  tw->f = fopen(path, "wb");
#endif
  
  if (tw->f == NULL) {
    perror("[tinywav] Failed to open file for writing");
    return -1;
  }

  tw->numChannels = numChannels;
  tw->numFramesInHeader = -1; // not used for writer
  tw->totalFramesReadWritten = 0;
  tw->sampFmt = sampFmt;
  tw->chanFmt = chanFmt;

  // prepare WAV header
  /**@note: We do this byte-by-byte to avoid dependencies (htonl() et al.) and because struct padding depends on
   * specific compiler implementation ('slurping' directly into the header struct is therefore dangerous). */
  tw->h.ChunkID[0] = 'R';
  tw->h.ChunkID[1] = 'I';
  tw->h.ChunkID[2] = 'F';
  tw->h.ChunkID[3] = 'F';
  tw->h.ChunkSize = 0; // fill this in on file-close
  tw->h.Format[0] = 'W';
  tw->h.Format[1] = 'A';
  tw->h.Format[2] = 'V';
  tw->h.Format[3] = 'E';
  tw->h.Subchunk1ID[0] = 'f';
  tw->h.Subchunk1ID[1] = 'm';
  tw->h.Subchunk1ID[2] = 't';
  tw->h.Subchunk1ID[3] = ' ';
  tw->h.Subchunk1Size = 16; // PCM
  tw->h.AudioFormat = (tw->sampFmt-1); // 1 PCM, 3 IEEE float
  tw->h.NumChannels = numChannels;
  tw->h.SampleRate = samplerate;
  tw->h.ByteRate = samplerate * numChannels * tw->sampFmt;
  tw->h.BlockAlign = numChannels * tw->sampFmt;
  tw->h.BitsPerSample = 8 * tw->sampFmt;
  tw->h.Subchunk2ID[0] = 'd';
  tw->h.Subchunk2ID[1] = 'a';
  tw->h.Subchunk2ID[2] = 't';
  tw->h.Subchunk2ID[3] = 'a';
  tw->h.Subchunk2Size = 0; // fill this in on file-close

  // write WAV header
  size_t elementCount = fwrite(tw->h.ChunkID, sizeof(char), 4, tw->f);
  elementCount += fwrite(&tw->h.ChunkSize, sizeof(uint32_t), 1, tw->f);
  elementCount += fwrite(tw->h.Format, sizeof(char), 4, tw->f);
  elementCount += fwrite(tw->h.Subchunk1ID, sizeof(char), 4, tw->f);
  elementCount += fwrite(&tw->h.Subchunk1Size, sizeof(uint32_t), 1, tw->f);
  elementCount += fwrite(&tw->h.AudioFormat, sizeof(uint16_t), 1, tw->f);
  elementCount += fwrite(&tw->h.NumChannels, sizeof(uint16_t), 1, tw->f);
  elementCount += fwrite(&tw->h.SampleRate, sizeof(uint32_t), 1, tw->f);
  elementCount += fwrite(&tw->h.ByteRate, sizeof(uint32_t), 1, tw->f);
  elementCount += fwrite(&tw->h.BlockAlign, sizeof(uint16_t), 1, tw->f);
  elementCount += fwrite(&tw->h.BitsPerSample, sizeof(uint16_t), 1, tw->f);
  elementCount += fwrite(tw->h.Subchunk2ID, sizeof(char), 4, tw->f);
  elementCount += fwrite(&tw->h.Subchunk2Size, sizeof(uint32_t), 1, tw->f);
  if (elementCount != 25) {
    return -1;
  }

  return 0;
}

int tinywav_open_read(TinyWav *tw, const char *path, TinyWavChannelFormat chanFmt) {
  
  if (tw == NULL || path == NULL) {
    return -1;
  }
  
#if _WIN32
  errno_t err = fopen_s(&tw->f, path, "rb");
  if (err != 0) { tw->f == NULL; }
#else
  tw->f = fopen(path, "rb");
#endif
  
  if (tw->f == NULL) {
    perror("[tinywav] Failed to open file for reading");
    return -1;
  }
  
  // Parse WAV header
  /** @note: We do this byte-by-byte to avoid dependencies (htonl() et al.) and because struct padding depends on
   *  specific compiler implementation ('slurping' directly into the header struct is therefore dangerous).
   *  The RIFF format specifies little-endian order for the data stream. */

  // RIFF Chunk, WAVE Subchunk
  size_t elementCount = fread(tw->h.ChunkID, sizeof(char), 4, tw->f);
  elementCount += fread(&tw->h.ChunkSize, sizeof(uint32_t), 1, tw->f);
  elementCount += fread(tw->h.Format, sizeof(char), 4, tw->f);
  
  if (elementCount < 9 || !chunkIDMatches(tw->h.ChunkID, "RIFF") || !chunkIDMatches(tw->h.Format, "WAVE")) {
    tinywav_close_read(tw);
    return -1;
  }
  
  // Go through subchunks until we find 'fmt '  (There are sometimes JUNK or other chunks before 'fmt ')
  while (fread(tw->h.Subchunk1ID, sizeof(char), 4, tw->f) == 4) {
    fread(&tw->h.Subchunk1Size, sizeof(uint32_t), 1, tw->f);
    if (chunkIDMatches(tw->h.Subchunk1ID, "fmt ")) {
      break;
    } else {
      fseek(tw->f, tw->h.Subchunk1Size, SEEK_CUR); // skip this subchunk
    }
  }
  
  // fmt Subchunk
  elementCount  = fread(&tw->h.AudioFormat, sizeof(uint16_t), 1, tw->f);
  elementCount += fread(&tw->h.NumChannels, sizeof(uint16_t), 1, tw->f);
  elementCount += fread(&tw->h.SampleRate, sizeof(uint32_t), 1, tw->f);
  elementCount += fread(&tw->h.ByteRate, sizeof(uint32_t), 1, tw->f);
  elementCount += fread(&tw->h.BlockAlign, sizeof(uint16_t), 1, tw->f);
  elementCount += fread(&tw->h.BitsPerSample, sizeof(uint16_t), 1, tw->f);
  if (elementCount != 6) {
    tinywav_close_read(tw);
    return -1;
  }
  
  // skip over any other chunks before the "data" chunk (e.g. JUNK, INFO, bext, ...)
  while (fread(tw->h.Subchunk2ID, sizeof(char), 4, tw->f) == 4) {
    fread(&tw->h.Subchunk2Size, sizeof(uint32_t), 1, tw->f);
    if (chunkIDMatches(tw->h.Subchunk2ID, "data")) {
      break;
    } else {
      fseek(tw->f, tw->h.Subchunk2Size, SEEK_CUR); // skip this subchunk
    }
  }
    
  tw->numChannels = tw->h.NumChannels;
  tw->chanFmt = chanFmt;

  if (tw->h.BitsPerSample == 32 && tw->h.AudioFormat == 3) {
    tw->sampFmt = TW_FLOAT32; // file has 32-bit IEEE float samples
  } else if (tw->h.BitsPerSample == 16 && tw->h.AudioFormat == 1) {
    tw->sampFmt = TW_INT16; // file has 16-bit int samples
  } else {
    tw->sampFmt = TW_FLOAT32;
    printf("[tinywav] Warning: wav file has %d bits per sample (int), which is not natively supported yet. Treating them as float; you may want to convert them manually after reading.\n", tw->h.BitsPerSample);
  }

  tw->numFramesInHeader = tw->h.Subchunk2Size / (tw->numChannels * tw->sampFmt);
  tw->totalFramesReadWritten = 0;
  
  return 0;
}

int tinywav_read_f(TinyWav *tw, void *data, int len) {
  
  if (tw == NULL || data == NULL || len < 0 || !tinywav_isOpen(tw)) {
    return -1;
  }
  
  if (tw->totalFramesReadWritten * tw->h.BlockAlign >= tw->h.Subchunk2Size) {
    // We are past the 'data' subchunk (size as declared in header).
    // Sometimes there are additionl chunks *after* -- ignore these.
    return 0; // there's nothing more to read, not an error.
  }
  
  switch (tw->sampFmt) {
    case TW_INT16: {
      int16_t *interleaved_data = (int16_t *) alloca(tw->numChannels*len*sizeof(int16_t));
      size_t samples_read = fread(interleaved_data, sizeof(int16_t), tw->numChannels*len, tw->f);
      tw->totalFramesReadWritten += samples_read / tw->numChannels;
      int frames_read = (int) samples_read / tw->numChannels;
      switch (tw->chanFmt) {
        case TW_INTERLEAVED: { // channel buffer is interleaved e.g. [LRLRLRLR]
          for (int pos = 0; pos < tw->numChannels * frames_read; pos++) {
            ((float *) data)[pos] = (float) interleaved_data[pos] / INT16_MAX;
          }
          return frames_read;
        }
        case TW_INLINE: { // channel buffer is inlined e.g. [LLLLRRRR]
          for (int i = 0, pos = 0; i < tw->numChannels; i++) {
            for (int j = i; j < frames_read * tw->numChannels; j += tw->numChannels, ++pos) {
              ((float *) data)[pos] = (float) interleaved_data[j] / INT16_MAX;
            }
          }
          return frames_read;
        }
        case TW_SPLIT: { // channel buffer is split e.g. [[LLLL],[RRRR]]
          for (int i = 0, pos = 0; i < tw->numChannels; i++) {
            for (int j = 0; j < frames_read; j++, ++pos) {
              ((float **) data)[i][j] = (float) interleaved_data[j*tw->numChannels + i] / INT16_MAX;
            }
          }
          return frames_read;
        }
        default: return 0;
      }
    }
    case TW_FLOAT32: {
      float *interleaved_data = (float *) alloca(tw->numChannels*len*sizeof(float));
      size_t samples_read = fread(interleaved_data, sizeof(float), tw->numChannels*len, tw->f);
      tw->totalFramesReadWritten += samples_read / tw->numChannels;
      int frames_read = (int) samples_read / tw->numChannels;
      switch (tw->chanFmt) {
        case TW_INTERLEAVED: { // channel buffer is interleaved e.g. [LRLRLRLR]
          memcpy(data, interleaved_data, tw->numChannels*frames_read*sizeof(float));
          return frames_read;
        }
        case TW_INLINE: { // channel buffer is inlined e.g. [LLLLRRRR]
          for (int i = 0, pos = 0; i < tw->numChannels; i++) {
            for (int j = i; j < frames_read * tw->numChannels; j += tw->numChannels, ++pos) {
              ((float *) data)[pos] = interleaved_data[j];
            }
          }
          return frames_read;
        }
        case TW_SPLIT: { // channel buffer is split e.g. [[LLLL],[RRRR]]
          for (int i = 0, pos = 0; i < tw->numChannels; i++) {
            for (int j = 0; j < frames_read; j++, ++pos) {
              ((float **) data)[i][j] = interleaved_data[j*tw->numChannels + i];
            }
          }
          return frames_read;
        }
        default: return 0;
      }
    }
    default: return 0;
  }
}

void tinywav_close_read(TinyWav *tw) {
  if (tw->f == NULL) {
    return; // fclose(NULL) is undefined behaviour
  }
  
  fclose(tw->f);
  tw->f = NULL;
}

int tinywav_write_f(TinyWav *tw, void *f, int len) {
  
  if (tw == NULL || f == NULL || len < 0 || !tinywav_isOpen(tw)) {
    return -1;
  }
  
  // 1. Bring samples into interleaved format
  // 2. write to disk
  
  switch (tw->sampFmt) {
    case TW_INT16: {
      int16_t *z = (int16_t *) alloca(tw->numChannels*len*sizeof(int16_t));
      switch (tw->chanFmt) {
        case TW_INTERLEAVED: {
          const float *const x = (const float *const) f;
          for (int i = 0; i < tw->numChannels*len; ++i) {
            z[i] = (int16_t) (x[i] * (float) INT16_MAX);
          }
          break;
        }
        case TW_INLINE: {
          const float *const x = (const float *const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = (int16_t) (x[j*len+i] * (float) INT16_MAX);
            }
          }
          break;
        }
        case TW_SPLIT: {
          const float **const x = (const float **const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = (int16_t) (x[j][i] * (float) INT16_MAX);
            }
          }
          break;
        }
        default: return 0;
      }

      size_t samples_written = fwrite(z, sizeof(int16_t), tw->numChannels*len, tw->f);
      size_t frames_written = samples_written / tw->numChannels;
      tw->totalFramesReadWritten += frames_written;
      return (int) frames_written;
    }
    case TW_FLOAT32: {
      float *z = (float *) alloca(tw->numChannels*len*sizeof(float));
      switch (tw->chanFmt) {
        case TW_INTERLEAVED: {
          const float *const x = (const float *const) f;
          for (int i = 0; i < tw->numChannels*len; ++i) {
            z[i] = x[i];
          }
          break;
        }
        case TW_INLINE: {
          const float *const x = (const float *const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = x[j*len+i];
            }
          }
          break;
        }
        case TW_SPLIT: {
          const float **const x = (const float **const) f;
          for (int i = 0, k = 0; i < len; ++i) {
            for (int j = 0; j < tw->numChannels; ++j) {
              z[k++] = x[j][i];
            }
          }
          break;
        }
        default: return 0;
      }

      size_t samples_written = fwrite(z, sizeof(float), tw->numChannels*len, tw->f);
      size_t frames_written = samples_written / tw->numChannels;
      tw->totalFramesReadWritten += frames_written;
      return (int) frames_written;
    }
    default: return 0;
  }
}

void tinywav_close_write(TinyWav *tw) {
  if (tw == NULL || tw->f == NULL) {
    return; // fclose(NULL) is undefined behaviour
  }
  
  uint32_t data_len = tw->totalFramesReadWritten * tw->numChannels * tw->sampFmt;
  uint32_t chunkSize_len = 36 + data_len; // 36 is size of header minus 8 (RIFF + this field)
  
  // update header struct as well
  tw->h.ChunkSize = chunkSize_len;
  tw->h.Subchunk2Size = data_len;
  
  // set length of data
  fseek(tw->f, 4, SEEK_SET); // offset of ChunkSize
  fwrite(&chunkSize_len, sizeof(uint32_t), 1, tw->f); // write ChunkSize
  
  fseek(tw->f, 40, SEEK_SET); // offset Subchunk2Size
  fwrite(&data_len, sizeof(uint32_t), 1, tw->f); // write Subchunk2Size
  
  fclose(tw->f);
  tw->f = NULL;
}

bool tinywav_isOpen(TinyWav *tw) {
  return (tw->f != NULL);
}






#define NUM_CHANNELS 2
#define SAMPLE_RATE 48000
#define BLOCK_SIZE 512
#define FILTER_SIZE 256
#define CONVOLVE_BLOCK_SIZE 512

void copy_array_f(float* dest, float* src, int dest_offset, int src_offset, int length) {
	for(int i = 0; i < length; i++) {
		dest[dest_offset + i] = src[src_offset + i];
	}
}

void conv_32(float* filter, float* audio, float* output, uint32_t start, uint32_t len) {
  for(int i = start; i < len + start; i++) {
    output[i] = 0;
    for(int j = 0; j < FILTER_SIZE; j++) {
      output[i] += filter[j] * audio[i - j];
    }
  }
}

void binaural_compute_no_ptrs(int degrees, char* audio_file) {

	// get snapped angle that is multiple of 30 degrees
	int snap_seg = (int) round(((float) degrees) / 30);
	int snap_deg = snap_seg * 30;
  snap_deg = snap_deg % 360;

  printf("snap_deg: %d, degrees: %d\r\n", snap_deg, degrees);
	// convert degrees to char
	
  char char_snap_deg[8] = "";
	char char_degrees[8] = "";
	sprintf(char_snap_deg, "%d", snap_deg);
	sprintf(char_degrees, "%d", degrees);

  printf("snap_deg: %s, degrees: %s\r\n", char_snap_deg, char_degrees);
  // build filter file path
	char filter_path[64] = "";
  strcat(filter_path, "dataset_bin/");
	strcat(filter_path, char_snap_deg);
	strcat(filter_path, "_");
	strcat(filter_path, "degrees.bin");

	// build output file path
	char output_path[64] = "";
	strcat(output_path, "outputs/");
	strcat(output_path, char_degrees);
	strcat(output_path, "_");
	strcat(output_path, "degrees_");
	strcat(output_path, audio_file);

	FILE* f_file;
	uint32_t byteCheck;

	printf("filter path: %s \r\n", filter_path);
	printf("output path: %s \r\n", output_path);
	// load filter's LR channels
	f_file = fopen(filter_path, "rb");

	static float filter_l[256];  // given filters have 256 samples each channel
	static float filter_r[256];
	fread(filter_l, 4, 256, f_file);
	fread(filter_r, 4, 256, f_file);
	fclose(f_file);

  for(int i = 0; i < 256; i++) {
    printf("filter_l[%d]: %f, filter_r[%d]: %f\r\n", i, filter_l[i], i, filter_r[i]);
  }

	// setup for audio file comprehension and format
	static TinyWav tw; // address to store read audio file
	uint32_t sample_rate;

	// load audio file
	int f_res = tinywav_open_read(&tw, "music.wav", TW_SPLIT);

	// get # of elements of data block per channel
	uint32_t data_size = tw.h.Subchunk2Size / (sizeof(float));
																				  // 8 = 4 * 2 (4 bytes/float * 2 channels
	sample_rate = (uint32_t) tw.h.SampleRate;          // get audio's sample rate
	uint32_t data_left = data_size;
	uint32_t iteration = (uint32_t)ceil((float)data_size/CONVOLVE_BLOCK_SIZE);

	// prepare output file
	static TinyWav tw_out;
	tinywav_open_write(&tw_out,
	    2,
	    sample_rate,
	    TW_FLOAT32, // the output samples will be 32-bit floats. TW_INT16 is also supported
	    TW_SPLIT,   // the samples to be written will be provided by an array of pointer
								  // that points to different sub-arrays: [[L,L,L,L], [R,R,R,R]]
	    output_path // the output path
	);

	static float cache_l[FILTER_SIZE - 1] = {0};
	static float cache_r[FILTER_SIZE - 1] = {0};

	static float sample_l[CONVOLVE_BLOCK_SIZE + FILTER_SIZE - 1] = {0};
	static float sample_r[CONVOLVE_BLOCK_SIZE + FILTER_SIZE - 1] = {0};

	//static float sample_out_inline[CONVOLVE_BLOCK_SIZE * NUM_CHANNELS] = {0};

	static float samples[2 * CONVOLVE_BLOCK_SIZE] = {0};
	static float out_l[CONVOLVE_BLOCK_SIZE + FILTER_SIZE - 1] = {0};
	static float out_r[CONVOLVE_BLOCK_SIZE + FILTER_SIZE - 1] = {0};

	for (int i = 0; i < iteration; ++i) {
		uint32_t input_seq_length = data_left < CONVOLVE_BLOCK_SIZE ? data_left : CONVOLVE_BLOCK_SIZE;
		uint32_t sample_length;
		if(i == 0) {
			sample_length = input_seq_length;
		} else {
			sample_length = input_seq_length + FILTER_SIZE - 1;
		}

		if(i == 0) {  // 1st iteration (edge case) as there are no data to prepend
			tinywav_read_f(&tw, samples, input_seq_length);

			// split inline sample array
			copy_array_f(sample_l, samples, 0, 0, input_seq_length);
			copy_array_f(sample_r, samples, 0, input_seq_length, input_seq_length);

			// Cache the last n samples of the current block to pad to the next block
			// so that convolution is continuous (where n = FILTER_SIZE - 1)
			copy_array_f(cache_l, sample_l, 0, input_seq_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
			copy_array_f(cache_r, sample_r, 0, input_seq_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
			// Convolution: A:= filter, B:= input seq
			// Convolution for Left channel
			conv_32(filter_l, sample_l, out_l, 0, input_seq_length);
			// Convolution for Right channel
			conv_32(filter_r, sample_r, out_r, 0, input_seq_length);

			float* out_split[2] = {out_l, out_r};
			tinywav_write_f(&tw_out, out_split, input_seq_length);

		} else {  // i > 0 => prepend data first and then convolve so convolution is continuous
			//sample_length = input_seq_length + FILTER_SIZE - 1;
			// load cache, prepend tail of last block into current block
			copy_array_f(sample_l, cache_l, 0, 0, FILTER_SIZE - 1);
			copy_array_f(sample_r, cache_r, 0, 0, FILTER_SIZE - 1);

			tinywav_read_f(&tw, samples, input_seq_length);

			// split inline sample array
			copy_array_f(sample_l, samples, FILTER_SIZE - 1, 0, input_seq_length);
			copy_array_f(sample_r, samples, FILTER_SIZE - 1, input_seq_length, input_seq_length);

      //for(int j = 0; j < sample_length; j++) {
      //  printf("sample_l[%d]: %f, sample_r[%d]: %f\r\n", j, sample_l[j], j, sample_r[j]);
      //}

			// Cache
			copy_array_f(cache_l, sample_l, 0, sample_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
			copy_array_f(cache_r, sample_r, 0, sample_length - FILTER_SIZE + 1, FILTER_SIZE - 1);

			// Convolution: A:= filter, B:= input seq
			// Convolution for Left channel
      conv_32(filter_l, sample_l, out_l, FILTER_SIZE - 1, input_seq_length);
      conv_32(filter_r, sample_r, out_r, FILTER_SIZE - 1, input_seq_length);

      //for(int j = 0; j < sample_length; j++) {
      //  printf("%f \r\n", out_r[j]);
      //}

      float* out_split[2] = {&out_l[255], &out_r[255]};
			tinywav_write_f(&tw_out, out_split, input_seq_length);
		}

		data_left -= input_seq_length;
		// print to console every 10 rounds or end of loop
		if(i % 10 == 0 || i == iteration - 1) {
			printf("done convolution block: %d / %d (%i)\r\n", i, iteration - 1);
		}
	}

	tinywav_close_write(&tw_out);
	tinywav_close_read(&tw);
}

void binaural_compute(int degrees, char* audio_file) {

	// get snapped angle that is multiple of 30 degrees
	int snap_seg = (int) round(((float) degrees) / 30);
	int snap_deg = snap_seg * 30;
  snap_deg = snap_deg % 360;

  printf("snap_deg: %d, degrees: %d\r\n", snap_deg, degrees);
	// convert degrees to char
	
  char char_snap_deg[8] = "";
	char char_degrees[8] = "";
	sprintf(char_snap_deg, "%d", snap_deg);
	sprintf(char_degrees, "%d", degrees);

  printf("snap_deg: %s, degrees: %s\r\n", char_snap_deg, char_degrees);
  // build filter file path
	char filter_path[64] = "";
  strcat(filter_path, "dataset_bin/");
	strcat(filter_path, char_snap_deg);
	strcat(filter_path, "_");
	strcat(filter_path, "degrees.bin");

	// build output file path
	char output_path[64] = "";
	strcat(output_path, "outputs/");
	strcat(output_path, char_degrees);
	strcat(output_path, "_");
	strcat(output_path, "degrees_");
	strcat(output_path, audio_file);

	FILE* f_file;
	uint32_t byteCheck;

	printf("filter path: %s \r\n", filter_path);
	printf("output path: %s \r\n", output_path);
	// load filter's LR channels
	f_file = fopen(filter_path, "rb");

	static float filter_l[256];  // given filters have 256 samples each channel
	static float filter_r[256];
	fread(filter_l, 4, 256, f_file);
	fread(filter_r, 4, 256, f_file);
	fclose(f_file);

  for(int i = 0; i < 256; i++) {
    printf("filter_l[%d]: %f, filter_r[%d]: %f\r\n", i, filter_l[i], i, filter_r[i]);
  }

	// setup for audio file comprehension and format
	static TinyWav tw; // address to store read audio file
	uint32_t sample_rate;

	// load audio file
	int f_res = tinywav_open_read(&tw, audio_file, TW_SPLIT);

	// get # of elements of data block per channel
	uint32_t data_size = tw.h.Subchunk2Size / (sizeof(float));
																				  // 8 = 4 * 2 (4 bytes/float * 2 channels
	sample_rate = (uint32_t) tw.h.SampleRate;          // get audio's sample rate
	uint32_t data_left = data_size;
	uint32_t iteration = (uint32_t)ceil((float)data_size/CONVOLVE_BLOCK_SIZE);

	// prepare output file
	static TinyWav tw_out;
	tinywav_open_write(&tw_out,
	    2,
	    sample_rate,
	    TW_FLOAT32, // the output samples will be 32-bit floats. TW_INT16 is also supported
	    TW_SPLIT,   // the samples to be written will be provided by an array of pointer
								  // that points to different sub-arrays: [[L,L,L,L], [R,R,R,R]]
	    output_path // the output path
	);

	// Prepare array to cache the tail of current samples to prepend to next convolving block
	static float cache_last_samples[(NUM_CHANNELS * (FILTER_SIZE - 1))] = {0};
	float* cache_ptrs[NUM_CHANNELS];
  
	// For audio read
	// samples are cached in TW_SPLIT format: [[L,L,L,L], [R,R,R,R]]
	static float samples[(NUM_CHANNELS * CONVOLVE_BLOCK_SIZE) + ((FILTER_SIZE - 1) * NUM_CHANNELS)] = {0};
	// create pointers for left and right channel in samples array
	float* sample_ptrs[NUM_CHANNELS];
  
	// For audio write
	// array to store converted binaural sample
	static float sample_out[(NUM_CHANNELS * CONVOLVE_BLOCK_SIZE) + ((FILTER_SIZE - 1) * NUM_CHANNELS)] = {0};
	float* sample_out_ptrs[NUM_CHANNELS];
  
	// sample pointers to offset for prepended cache when i > 0
	static float* sample_ptrs_offset[NUM_CHANNELS];
	float* sample_out_ptrs_offset[NUM_CHANNELS];
  
	// generate pointers to different channel section for both read and write
	for (int j = 0; j < NUM_CHANNELS; ++j) {
		cache_ptrs[j] = cache_last_samples + j * (FILTER_SIZE - 1);
		sample_ptrs[j] = samples + j * (CONVOLVE_BLOCK_SIZE + (FILTER_SIZE - 1));
		sample_out_ptrs[j] = sample_out + j * (CONVOLVE_BLOCK_SIZE + (FILTER_SIZE - 1));
		sample_ptrs_offset[j] = sample_ptrs[j] + (FILTER_SIZE - 1);
		sample_out_ptrs_offset[j] = sample_out_ptrs[j] + (FILTER_SIZE - 1);
	}
  
	for (int i = 0; i < iteration; ++i) {
		uint32_t input_seq_length = data_left < CONVOLVE_BLOCK_SIZE ? data_left : CONVOLVE_BLOCK_SIZE;
    uint32_t sample_length;
		if(i == 0) {
			sample_length = input_seq_length;
		} else {
			sample_length = input_seq_length + FILTER_SIZE - 1;
		}
  
	  if(i == 0) {  // 1st iteration (edge case) as there are no data to prepend
			tinywav_read_f(&tw, sample_ptrs, input_seq_length);
  
			// Cache the last n samples of the current block to pad to the next block
			// so that convolution is continuous (where n = FILTER_SIZE - 1)
			copy_array_f(cache_ptrs[0], sample_ptrs[0], 0, input_seq_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
			copy_array_f(cache_ptrs[1], sample_ptrs[1], 0, input_seq_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
			// Convolution: A:= filter, B:= input seq
			// Convolution for Left channel
      conv_32(filter_l, sample_ptrs[0], sample_out_ptrs[0], 0, input_seq_length);
			// Convolution for Right channel
			conv_32(filter_r, sample_ptrs[1], sample_out_ptrs[1], 0, input_seq_length);
  
			//copy_array_f(sample_out_ptrs[1], sample_ptrs[1], 0, 0, input_seq_length);  // TEST
			tinywav_write_f(&tw_out, sample_out_ptrs, input_seq_length);
  
		} else {  // i > 0 => prepend data first and then convolve so convolution is continuous
			//sample_length = input_seq_length + FILTER_SIZE - 1;
			// load cache, prepend tail of last block into current block
			copy_array_f(sample_ptrs[0], cache_ptrs[0], 0, 0, FILTER_SIZE - 1);
			copy_array_f(sample_ptrs[1], cache_ptrs[1], 0, 0, FILTER_SIZE - 1);
  
			tinywav_read_f(&tw, sample_ptrs_offset, input_seq_length);
  
			// Cache
			copy_array_f(cache_ptrs[0], sample_ptrs_offset[0], 0, input_seq_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
			copy_array_f(cache_ptrs[1], sample_ptrs_offset[1], 0, input_seq_length - FILTER_SIZE + 1, FILTER_SIZE - 1);
  
			// Convolution: A:= filter, B:= input seq
			// Convolution for Left channel
      conv_32(filter_l, sample_ptrs[0], sample_out_ptrs[0], FILTER_SIZE - 1, input_seq_length);
			// Convolution for Right channel
			conv_32(filter_r, sample_ptrs[1], sample_out_ptrs[1], FILTER_SIZE - 1, input_seq_length);
      
			//copy_array_f(sample_out_ptrs[1], sample_ptrs_offset[1], 256, 0, input_seq_length);  // TEST
			tinywav_write_f(&tw_out, sample_out_ptrs_offset, input_seq_length);
			}
  
		data_left -= input_seq_length;
		// print to console every 10 rounds or end of loop
		if(i % 10 == 0 || i == iteration - 1) {
			printf("done convolution block: %d / %d (%i)\r\n", i, iteration - 1);
		}
	}

	tinywav_close_write(&tw_out);
	tinywav_close_read(&tw);
}


int main() {
  binaural_compute(150, "music.wav");
}