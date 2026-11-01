/* SPDX-License-Identifier: Apache-2.0 */
#ifndef LGE_QCAMERA_VIDEO_MIRROR_H
#define LGE_QCAMERA_VIDEO_MIRROR_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace android {

enum LgeVideoMirrorMode {
    LGE_VIDEO_MIRROR_OFF = 0,
    LGE_VIDEO_MIRROR_HORIZONTAL = 1,
    LGE_VIDEO_MIRROR_VERTICAL = 2
};

// The preview buffer remains owned by the camera/display pipeline. Transform
// only into a separate encoder buffer, keeping each NV12/NV21 chroma pair intact.
static inline bool lgeMirrorVideo420(const uint8_t *source, size_t sourceSize,
        uint8_t *destination, size_t destinationSize, size_t width, size_t height,
        size_t stride, size_t chromaOffset, int mode) {
    if (!source || !destination || source == destination || !width || !height
            || (width & 1) || (height & 1) || stride < width
            || stride > sourceSize / height || stride > destinationSize / height
            || chromaOffset < stride * height || chromaOffset > sourceSize
            || chromaOffset > destinationSize
            || stride > (sourceSize - chromaOffset) / (height / 2)
            || stride > (destinationSize - chromaOffset) / (height / 2)
            || (mode != LGE_VIDEO_MIRROR_HORIZONTAL && mode != LGE_VIDEO_MIRROR_VERTICAL)) {
        return false;
    }
    for (size_t row = 0; row < height; ++row) {
        const size_t inputRow = mode == LGE_VIDEO_MIRROR_VERTICAL ? height - 1 - row : row;
        const uint8_t *input = source + inputRow * stride;
        uint8_t *output = destination + row * stride;
        if (mode == LGE_VIDEO_MIRROR_VERTICAL) {
            memcpy(output, input, width);
        } else {
            for (size_t column = 0; column < width; ++column) {
                output[column] = input[width - 1 - column];
            }
        }
    }
    for (size_t row = 0; row < height / 2; ++row) {
        const size_t inputRow = mode == LGE_VIDEO_MIRROR_VERTICAL ? height / 2 - 1 - row : row;
        const uint8_t *input = source + chromaOffset + inputRow * stride;
        uint8_t *output = destination + chromaOffset + row * stride;
        if (mode == LGE_VIDEO_MIRROR_VERTICAL) {
            memcpy(output, input, width);
        } else {
            for (size_t column = 0; column < width; column += 2) {
                output[column] = input[width - 2 - column];
                output[column + 1] = input[width - 1 - column];
            }
        }
    }
    return true;
}

} // namespace android
#endif
