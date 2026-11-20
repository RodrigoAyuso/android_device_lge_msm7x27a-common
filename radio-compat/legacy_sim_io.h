// Copyright (C) 2026 Vee3 bring-up contributors
// SPDX-License-Identifier: Apache-2.0
#ifndef LGE_LEGACY_SIM_IO_H
#define LGE_LEGACY_SIM_IO_H

namespace lge_ril {
// The shared Nougat BoardConfig enabled TARGET_RIL_SUPPORT_SEEK. Its vendor
// SIM_IO ABI therefore has a leading cla, including for ordinary CRSM I/O.
// Keep this private layout separate from Oreo's public RIL_SIM_IO_v6.
struct LegacySeekSimIo {
    int cla;
    int command;
    int fileid;
    char* path;
    int p1;
    int p2;
    int p3;
    char* data;
    char* pin2;
    char* aidPtr;
};

static_assert(sizeof(int) == 4, "Legacy SIM_IO needs 32-bit integers");
static_assert(__builtin_offsetof(LegacySeekSimIo, command) == 4,
              "Legacy SIM_IO must begin with cla");
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(LegacySeekSimIo) == 40, "Unexpected ARM32 SEEK SIM_IO size");
static_assert(__builtin_offsetof(LegacySeekSimIo, path) == 12,
              "Unexpected ARM32 SEEK path offset");
static_assert(__builtin_offsetof(LegacySeekSimIo, aidPtr) == 36,
              "Unexpected ARM32 SEEK AID offset");
#endif
}  // namespace lge_ril
#endif
