/* SPDX-License-Identifier: Apache-2.0 */
#ifndef LGE_CAMERA_NATIVE_HDR_EFFECT_H
#define LGE_CAMERA_NATIVE_HDR_EFFECT_H

#include "../compat/FrameTransform.h"

namespace android {

struct LgeHdrStats {
    uint32_t samples;
    uint8_t low;
    uint8_t median;
    uint8_t high;
    uint8_t shadowLift;
    uint8_t highlightPull;
    bool changed;
};

// Single-exposure tone effect, not exposure fusion or sensor/ISP HDR.
// Operates on private NV12/NV21 capture buffers before JPEG compression.
// Validate both buffers before modifying either; preserve chroma and padding.
// No heap allocations. A thumbnail uses the main image's exact tone curve.
// false leaves both input buffers and stats untouched.
bool lgeApplyHdrEffect(const LgeFrame420 &main,
        const LgeFrame420 *thumbnail, LgeHdrStats *stats);

} // namespace android
#endif
