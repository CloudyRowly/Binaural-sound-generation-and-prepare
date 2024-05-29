import scipy.io
import numpy as np
import soundfile
from pathlib import Path

location = Path(__file__).absolute().parent
mat = scipy.io.loadmat(f'{location}/dataset/180_degrees.mat')
print(mat)