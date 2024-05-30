#include "tinywav.h"

#define NUM_CHANNELS 2
#define SAMPLE_RATE 48000
#define BLOCK_SIZE 480

int main() {
    TinyWav tw;
    tinywav_open_read(&tw, 
    	"music.wav",
    	TW_SPLIT // the samples will be delivered by the read function in split format
    );

    for (int i = 0; i < 100; i++) {
      // samples are always provided in float32 format, 
      // regardless of file sample format
      float samples[NUM_CHANNELS * BLOCK_SIZE];
    
      // Split buffer requires pointers to channel buffers
      float* samplePtrs[NUM_CHANNELS];
      for (int j = 0; j < NUM_CHANNELS; ++j) {
        samplePtrs[j] = samples + j*BLOCK_SIZE;
      }

      tinywav_read_f(&tw, samplePtrs, BLOCK_SIZE);
    
    }

    tinywav_close_read(&tw);
}