/* SPDX-License-Identifier: Apache-2.0 */
#include "HdrEffect.h"

namespace android {
namespace {

bool separateBuffers(const LgeFrame420 &a, const LgeFrame420 &b) {
    const uintptr_t left = reinterpret_cast<uintptr_t>(a.data);
    const uintptr_t right = reinterpret_cast<uintptr_t>(b.data);
    const uintptr_t maximum = static_cast<uintptr_t>(-1);
    return a.size <= maximum - left && b.size <= maximum - right
            && (left + a.size <= right || right + b.size <= left);
}

uint8_t percentile(const uint32_t *histogram, uint32_t count, uint32_t percent) {
    const uint32_t target = (count * percent + 99) / 100;
    uint32_t accumulated = 0;
    for (unsigned int value = 0; value < 256; ++value) {
        accumulated += histogram[value];
        if (accumulated >= target)
            return static_cast<uint8_t>(value);
    }
    return 255;
}

int bounded(int value, int minimum, int maximum) {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

void buildCurve(const LgeFrame420 &frame, uint8_t *curve, LgeHdrStats *stats) {
    uint32_t histogram[256] = {};
    // At most 256 x 256 samples, also for large or padded capture buffers.
    const size_t stepX = 1 + (frame.width - 1) / 256;
    const size_t stepY = 1 + (frame.height - 1) / 256;
    LgeHdrStats result = {};
    for (size_t y = 0; y < frame.height; y += stepY) {
        const uint8_t *line = frame.data + frame.yOffset + y * frame.stride;
        for (size_t x = 0; x < frame.width; x += stepX) {
            ++histogram[line[x]];
            ++result.samples;
        }
    }
    result.low = percentile(histogram, result.samples, 10);
    result.median = percentile(histogram, result.samples, 50);
    result.high = percentile(histogram, result.samples, 90);
    const int spread = static_cast<int>(result.high) - result.low;
    // Leave flat, very dark and almost white scenes alone; avoid blindly
    // lifting noise or attempting to invent clipped detail.
    if (spread > 64 && result.median >= 24 && result.median <= 232) {
        const int strength = bounded(spread - 64, 0, 128);
        result.shadowLift = static_cast<uint8_t>(
                bounded(20 + (128 - result.median) / 16, 12, 24) * strength / 128);
        result.highlightPull = static_cast<uint8_t>(
                bounded(12 + (static_cast<int>(result.median) - 128) / 24, 8, 16)
                    * strength / 128);
    }
    result.changed = result.shadowLift != 0 || result.highlightPull != 0;
    int previous = 0;
    for (int y = 0; y < 256; ++y) {
        int mapped = y;
        if (y < 128)
            mapped += (y * (128 - y) * result.shadowLift + 2048) / 4096;
        else
            mapped -= ((y - 128) * (255 - y) * result.highlightPull + 2016) / 4032;
        // Keep luminance ordering and black/white endpoints intact.
        mapped = bounded(mapped, previous, 255);
        curve[y] = static_cast<uint8_t>(mapped);
        previous = mapped;
    }
    *stats = result;
}

void applyCurve(const LgeFrame420 &frame, const uint8_t *curve) {
    for (size_t y = 0; y < frame.height; ++y) {
        uint8_t *line = frame.data + frame.yOffset + y * frame.stride;
        for (size_t x = 0; x < frame.width; ++x)
            line[x] = curve[line[x]];
    }
}

} // namespace

bool lgeApplyHdrEffect(const LgeFrame420 &main,
        const LgeFrame420 *thumbnail, LgeHdrStats *stats) {
    const uintptr_t address = reinterpret_cast<uintptr_t>(main.data);
    if (main.size > static_cast<uintptr_t>(-1) - address
            || !lgeValidFrame420(main)
            || (thumbnail && (!lgeValidFrame420(*thumbnail)
                || !separateBuffers(main, *thumbnail))))
        return false;
    uint8_t curve[256];
    LgeHdrStats result;
    buildCurve(main, curve, &result);
    if (result.changed) {
        applyCurve(main, curve);
        if (thumbnail)
            applyCurve(*thumbnail, curve);
    }
    if (stats)
        *stats = result;
    return true;
}

} // namespace android
