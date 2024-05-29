import scipy.io
import numpy as np
import soundfile
import os
from pathlib import Path

# load music wave
location = Path(__file__).absolute().parent
f_name = "gunshot"
x, fs = soundfile.read(f'{location}/{f_name}.wav')
print(x.shape)

def gen_audio_all_dir():
    #for i in range(0, 360, 30):
        i = 210
        f = open(f'{location}/dataset_bin_underscore/{i}_degrees.bin', 'rb')
        h = np.fromfile(f, dtype=np.float32)
        print(h.shape)
        #h = np.array(ir)
        y = np.c_[(np.convolve(x[:, 0], h[:256], mode='full')),(np.convolve(x[:, 1], h[256:], mode='full'))]
        soundfile.write(f'{f_name}_{i}.wav', y, fs)


if __name__ == '__main__':
    gen_audio_all_dir()