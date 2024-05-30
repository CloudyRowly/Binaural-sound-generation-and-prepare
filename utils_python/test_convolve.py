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

import numpy as np

# Full mode example
x = np.arange(5, 10)

# This is a differencing filter:
#   y[n] = x[n] - x[n-2]
h = np.array([1, 0, -1])

y = np.convolve(x, h, mode='full')

print('Input           :\t', x)
print('Filter          :\t', h)
print('Full convolution:\t', y)

a = np.array(([1, 2, 3, 4, 5],
             [6, 7, 8, 9, 10],
             [11, 12, 13, 14, 15]))

f = np.array(([1, 0, -1],
             [1, 0, -1],
             [1, 0, -1]))

y = np.convolve(a, f, mode='full')

print('Input           :\t', x)
print('Filter          :\t', h)
print('Full convolution:\t', y)