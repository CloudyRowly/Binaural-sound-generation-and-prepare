import scipy.io
import numpy as np
import soundfile
from pathlib import Path

# load music wave
f_name = "gunshot"
location = Path(__file__).absolute().parent
x, fs = soundfile.read(f'{location}/{f_name}.wav')
print(x.shape)

def gen_audio_all_dir():
    for i in range(0, 360, 30):
        mat = scipy.io.loadmat(f'{location}/dataset/{i}_degrees.mat')
        ir = mat['ir']
        h = np.array(ir)
        y = np.c_[(np.convolve(x[:, 0], h[:,0], mode='full')),(np.convolve(x[:, 1], h[:,1], mode='full'))]
        soundfile.write(f'{location}/{f_name}_{i}.wav', y, fs)


if __name__ == '__main__':
    gen_audio_all_dir()
