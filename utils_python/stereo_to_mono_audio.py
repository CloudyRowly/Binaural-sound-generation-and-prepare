import soundfile
from pathlib import Path
import numpy as np

# load music wave
location = Path(__file__).absolute().parent
f_name = "music"
x, fs = soundfile.read(f'{location}/{f_name}.wav')

def stereo_to_mono_audio(x):
    # stereo to mono
    if x.ndim == 2:
        x = np.mean(x, axis=1)
    return x

# stereo to mono
x = stereo_to_mono_audio(x)
x = x.astype(np.float32)
soundfile.write(f'{location}/outputs/{f_name}_mono.wav', x, fs)