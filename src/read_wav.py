import scipy.io
import numpy as np
import soundfile
from pathlib import Path
from pathlib import Path
import matplotlib.pyplot as plt

f_name = "180_degrees_music"
location = Path(__file__).absolute().parent
x, fs = soundfile.read(f'{location}/{f_name}.wav')
print(x.shape)

# Plotting the sound wave
plt.plot(x[:, 1])
plt.xlabel('Sample')
plt.ylabel('Amplitude')
plt.title('Sound Wave')
plt.show()