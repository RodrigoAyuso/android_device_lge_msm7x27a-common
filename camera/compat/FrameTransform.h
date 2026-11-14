/* SPDX-License-Identifier: Apache-2.0 */
#ifndef LGE_OREO_CAMERA_FRAME_TRANSFORM_H
#define LGE_OREO_CAMERA_FRAME_TRANSFORM_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace android {

enum LgeFrameFlip {
    LGE_FRAME_FLIP_NONE = 0,
    LGE_FRAME_FLIP_HORIZONTAL = 1,
    LGE_FRAME_FLIP_VERTICAL = 2
};

struct LgeFrame420 {
    uint8_t *data;
    size_t size;
    size_t width;
    size_t height;
    size_t stride;
    size_t yOffset;
    size_t uvOffset;
};

static inline bool lgePacked420Size(size_t width, size_t height, size_t *size) {
    const size_t maximum = static_cast<size_t>(-1);
    if (!size || !width || !height || (width & 1) || (height & 1)
            || width > maximum / height)
        return false;
    const size_t pixels = width * height;
    if (pixels > maximum - pixels / 2)
        return false;
    *size = pixels + pixels / 2;
    return true;
}

static inline bool lgeValidFrame420(const LgeFrame420 &frame) {
    size_t packedSize;
    if (!frame.data || !lgePacked420Size(frame.width, frame.height, &packedSize)
            || frame.stride < frame.width || frame.yOffset > frame.size
            || frame.uvOffset > frame.size
            || frame.width > frame.size - frame.yOffset
            || frame.width > frame.size - frame.uvOffset)
        return false;
    if (frame.height - 1 > (frame.size - frame.yOffset - frame.width) / frame.stride
            || frame.height / 2 - 1 >
                (frame.size - frame.uvOffset - frame.width) / frame.stride)
        return false;
    const size_t yEnd = frame.yOffset + (frame.height - 1) * frame.stride + frame.width;
    return yEnd <= frame.uvOffset;
}

// CameraSource's yuv420sp maps to OMX YUV420SemiPlanar, whose AVC input is NV12.
// Pack active pixels into an independent buffer before the preview is queued.
static inline bool lgePackVideo420(const LgeFrame420 &source,
        uint8_t *destination, size_t destinationSize, bool sourceIsNV21, int flip) {
    size_t packedSize;
    if (!destination || !lgeValidFrame420(source)
            || !lgePacked420Size(source.width, source.height, &packedSize)
            || destinationSize < packedSize
            || flip < LGE_FRAME_FLIP_NONE || flip > LGE_FRAME_FLIP_VERTICAL)
        return false;
    const uintptr_t input = reinterpret_cast<uintptr_t>(source.data);
    const uintptr_t output = reinterpret_cast<uintptr_t>(destination);
    const uintptr_t maximum = static_cast<uintptr_t>(-1);
    if (source.size > maximum - input || destinationSize > maximum - output
            || (input < output + destinationSize && output < input + source.size))
        return false;
    const size_t pixels = source.width * source.height;
    for (size_t row = 0; row < source.height; ++row) {
        const size_t inputRow = flip == LGE_FRAME_FLIP_VERTICAL ? source.height - 1 - row : row;
        const uint8_t *from = source.data + source.yOffset + inputRow * source.stride;
        uint8_t *to = destination + row * source.width;
        if (flip == LGE_FRAME_FLIP_HORIZONTAL) {
            for (size_t col = 0; col < source.width; ++col)
                to[col] = from[source.width - 1 - col];
        } else {
            memcpy(to, from, source.width);
        }
    }
    for (size_t row = 0; row < source.height / 2; ++row) {
        const size_t inputRow = flip == LGE_FRAME_FLIP_VERTICAL ? source.height / 2 - 1 - row : row;
        const uint8_t *from = source.data + source.uvOffset + inputRow * source.stride;
        uint8_t *to = destination + pixels + row * source.width;
        for (size_t col = 0; col < source.width; col += 2) {
            const size_t pair = flip == LGE_FRAME_FLIP_HORIZONTAL ? source.width - 2 - col : col;
            to[col] = from[pair + (sourceIsNV21 ? 1 : 0)];
            to[col + 1] = from[pair + (sourceIsNV21 ? 0 : 1)];
        }
    }
    return true;
}

// To mirror the displayed JPEG horizontally, flip the input's other axis
// when the JPEG encoder (or EXIF orientation) applies a quarter turn.
static inline int lgePhotoMirrorAxis(int rotation) {
    switch (rotation) {
    case 0:
    case 180:
        return LGE_FRAME_FLIP_HORIZONTAL;
    case 90:
    case 270:
        return LGE_FRAME_FLIP_VERTICAL;
    default:
        return -1;
    }
}

// Capture buffers are private to the stopped-preview JPEG path. Preserve
// NV12/NV21 pair ordering, plane offsets and padding without another ION pool.
static inline bool lgeMirrorPhoto420(const LgeFrame420 &frame, int flip) {
    if (!lgeValidFrame420(frame)
            || (flip != LGE_FRAME_FLIP_HORIZONTAL && flip != LGE_FRAME_FLIP_VERTICAL))
        return false;
    if (flip == LGE_FRAME_FLIP_HORIZONTAL) {
        for (size_t row = 0; row < frame.height; ++row) {
            uint8_t *line = frame.data + frame.yOffset + row * frame.stride;
            for (size_t col = 0; col < frame.width / 2; ++col) {
                const uint8_t value = line[col];
                line[col] = line[frame.width - 1 - col];
                line[frame.width - 1 - col] = value;
            }
        }
        for (size_t row = 0; row < frame.height / 2; ++row) {
            uint8_t *line = frame.data + frame.uvOffset + row * frame.stride;
            for (size_t left = 0, right = frame.width - 2; left < right; left += 2, right -= 2) {
                for (size_t component = 0; component < 2; ++component) {
                    const uint8_t value = line[left + component];
                    line[left + component] = line[right + component];
                    line[right + component] = value;
                }
            }
        }
    } else {
        for (size_t plane = 0; plane < 2; ++plane) {
            const size_t rows = plane == 0 ? frame.height : frame.height / 2;
            const size_t offset = plane == 0 ? frame.yOffset : frame.uvOffset;
            for (size_t row = 0; row < rows / 2; ++row) {
                uint8_t *top = frame.data + offset + row * frame.stride;
                uint8_t *bottom = frame.data + offset + (rows - 1 - row) * frame.stride;
                for (size_t col = 0; col < frame.width; ++col) {
                    const uint8_t value = top[col];
                    top[col] = bottom[col];
                    bottom[col] = value;
                }
            }
        }
    }
    return true;
}

} // namespace android
#endif
