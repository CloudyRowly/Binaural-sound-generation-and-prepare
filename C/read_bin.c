/**
 * Copyright (c) 2024 - Gia Minh Nguyen (Giaminhnguyen.2004@gmail.com) (u7556893@anu.edu.au)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "stdio.h"

int main() {
    FILE *file;
    file = fopen("0_degrees.bin", "rb");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    // Read the samples
    float left[256];
    float right[256];
    fread(left, 4, 256, file);
    fread(right, sizeof(float), 256, file);
    fclose(file);
    for(int i = 0; i < 256; i++) {
        printf("Left: %f, Right: %f\n", left[i], right[i]);
    }
    return 0;
}