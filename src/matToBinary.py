import scipy.io
import numpy as np
from pathlib import Path

def matToBin():
    location = Path(__file__).absolute().parent
    for i in range(0, 360, 30):
        mat = scipy.io.loadmat(f'{location}/dataset/{i}_degrees.mat')
        ir = mat['ir']
        f = open(f"{location}/dataset_bin_underscore/{i}_degrees.bin", "wb")
        for j in range(2):
            for x in ir[:, j]:
                f.write(x.tobytes())
        f.close()
            

if __name__ == '__main__':
    matToBin()