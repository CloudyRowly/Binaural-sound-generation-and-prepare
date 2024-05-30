# WAV Audio Binaural sound computation and preparation

Convert a 2-channel .wav audio file into binaural (3d surround) sound.

# Folder: utils_python

Includes:

- ```gen_binaural_audio.py```: Generate sounds for different angles using the given audio file and filters in .mat files
- ```gen_binaural_from_bin.py```: Generate sounds for different angles using the given audio file and filters in .bin files
- ```matToBinary.py```: Convert impulse response filters from .mat format into binary format
- ```read_db.py```: filter files reading and visualising
- ```read_wav.py```: audio files reading and visualising
- ```test_convolve```: Example showing binaural sound convolving concept

# Folder: C

Include:

- ```tinywav.c```: Binaural sound computation in C
- ```c_wav_test```: Sample code for writing/reading functions of tinyWav library
- ```dataset_bin```: 32-bit float filter for different sound directions in 30 degrees increment (binary format)
