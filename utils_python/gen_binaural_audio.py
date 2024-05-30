# Copyright (c) 2024, Gia Minh Nguyen (Giaminhnguyen.2004@gmail.com) (u7556893@anu.edu.au)
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

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
        soundfile.write(f'{location}/outputs/{i}_{f_name}.wav', y, fs)


if __name__ == '__main__':
    gen_audio_all_dir()
