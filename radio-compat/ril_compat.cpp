// Shared LG gen2 adapter: legacy v7 and SEEK ABI.
// Copyright (C) 2026 Vee3 bring-up contributors
// SPDX-License-Identifier: Apache-2.0
#define LOG_TAG "LgeRILCompat"
#ifndef RIL_SHLIB
#define RIL_SHLIB
#endif
#include <telephony/ril.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <new>
#include <time.h>
#include <cstddef>
#include <cstring>
#include "legacy_ids.h"
#include "legacy_sim_io.h"

#if defined(ANDROID_MULTI_SIM)
#error This shim requires the single-SIM-per-rild legacy ABI used by this LG port
#endif

static_assert(offsetof(RIL_SIM_IO_v6, command) == 0,
              "Expected standard Oreo SIM_IO input, without SEEK cla");
#if __SIZEOF_POINTER__ == 4
static_assert(sizeof(RIL_SIM_IO_v6) == 36, "Unexpected Oreo ARM32 SIM_IO size");
static_assert(offsetof(RIL_SIM_IO_v6, aidPtr) == 32,
              "Unexpected Oreo ARM32 AID offset");
#endif

namespace {
const char kBackend[] = "/system/lib/libril-qc-qmi-1.so";
const char kStatusProperty[] = "vendor.ril.lge.compat";
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
RIL_Env framework_env = {};
RIL_Env vendor_env = {};
RIL_RadioFunctions exported = {};
const RIL_RadioFunctions* backend_functions = NULL;
void* backend_library = NULL;

void set_status(const char* status) {
    property_set(kStatusProperty, status);
    char hardware[PROP_VALUE_MAX] = {};
    property_get("ro.hardware", hardware, "");
    if (std::strcmp(hardware, "vee3") == 0)
        property_set("vendor.ril.vee3.compat", status);
}

void reject_request(RIL_Token token, RIL_Errno error) {
    if (framework_env.OnRequestComplete)
        framework_env.OnRequestComplete(token, error, NULL, 0);
}

#include "ril_boot_identity.h"

void on_request(int request, void* data, size_t length, RIL_Token token) {
    const int translated = lge_ril::request_to_legacy(request);
    if (translated < 0) {
        ALOGI("unsupported framework request %d; not sent to legacy blob", request);
        reject_request(token, RIL_E_REQUEST_NOT_SUPPORTED);
        return;
    }
    if (backend_functions == NULL || backend_functions->onRequest == NULL) {
        reject_request(token, RIL_E_RADIO_NOT_AVAILABLE);
        return;
    }
    if (defer_boot_identity(request, data, length, token)) return;
    if (translated != request)
        ALOGI("request %d -> legacy %d", request, translated);
    if (request == RIL_REQUEST_SIM_IO) {
        if (data == NULL || length != sizeof(RIL_SIM_IO_v6)) {
            ALOGE("invalid Oreo SIM_IO payload: size=%zu", length);
            reject_request(token, RIL_E_GENERIC_FAILURE);
            return;
        }
        const RIL_SIM_IO_v6& input = *static_cast<const RIL_SIM_IO_v6*>(data);
        lge_ril::LegacySeekSimIo legacy = {
            0, input.command, input.fileid, input.path,
            input.p1, input.p2, input.p3, input.data, input.pin2, input.aidPtr
        };
        // Match Nougat dispatchSIM_IO: cla=0, call the vendor synchronously,
        // then let the original caller release its strings. Do not log their
        // contents (PINs, card records or subscriber identifiers).
        ALOGI("SIM_IO SEEK ABI: command=0x%x file=0x%x size=%zu -> %zu",
              input.command, input.fileid, length, sizeof(legacy));
        backend_functions->onRequest(translated, &legacy, sizeof(legacy), token);
        return;
    }
    // The framework retains its own request number in token. The vendor sees
    // the translated number; its completion goes to the original callback.
    backend_functions->onRequest(translated, data, length, token);
}

int supports(int request) {
    const int translated = lge_ril::request_to_legacy(request);
    if (translated < 0 || backend_functions == NULL) return 0;
    return backend_functions->supports ? backend_functions->supports(translated) : 0;
}

void on_unsolicited(int indication, const void* data, size_t length) {
    const int translated = lge_ril::unsolicited_to_framework(indication);
    if (translated < 0) {
        ALOGI("unmapped legacy indication %d; not reinterpreted as Oreo data", indication);
        return;
    }
    if (translated != indication)
        ALOGI("legacy indication %d -> framework %d", indication, translated);
    if (framework_env.OnUnsolicitedResponse)
        framework_env.OnUnsolicitedResponse(translated, data, length);
}

// QCRIL completes requests asynchronously and the completion has no request
// number. Track only DEVICE_IDENTITY and keep the framework's opaque token.
struct IdentityToken {
    RIL_Token token;
    IdentityToken* next;
};
pthread_mutex_t identity_mutex = PTHREAD_MUTEX_INITIALIZER;
IdentityToken* identity_tokens = NULL;

void on_v1_request(int request, void* data, size_t length, RIL_Token token) {
    if (request == RIL_REQUEST_DEVICE_IDENTITY && token && !data && length == 0) {
        IdentityToken* entry = new (std::nothrow) IdentityToken;
        if (!entry) {
            reject_request(token, RIL_E_NO_MEMORY);
            return;
        }
        entry->token = token;
        pthread_mutex_lock(&identity_mutex);
        entry->next = identity_tokens;
        identity_tokens = entry;
        pthread_mutex_unlock(&identity_mutex);
    }
    on_request(request, data, length, token);
}

void on_v1_complete(RIL_Token token, RIL_Errno error, void* response, size_t length) {
    pthread_mutex_lock(&identity_mutex);
    IdentityToken** link = &identity_tokens;
    while (*link && (*link)->token != token) {
        link = &(*link)->next;
    }
    IdentityToken* entry = *link;
    if (entry) *link = entry->next;
    pthread_mutex_unlock(&identity_mutex);

    if (entry) {
        delete entry;
        // V1's DMS handler appends a private fifth pointer to the standard
        // IMEI, IMEISV, ESN, MEID array. The vendor owns the whole buffer.
        if (error == RIL_E_SUCCESS && response && length == 5 * sizeof(char*)) {
            length = 4 * sizeof(char*);
            ALOGI("V1 DEVICE_IDENTITY: QCRIL 5 -> 4");
        } else if (error == RIL_E_SUCCESS && (!response || length != 4 * sizeof(char*))) {
            ALOGI("V1 DEVICE_IDENTITY: unexpected reply bytes=%zu payload=%d",
                  length, response != NULL);
        }
    }
    framework_env.OnRequestComplete(token, error, response, length);
}
}  // namespace

extern "C" __attribute__((visibility("default")))
const RIL_RadioFunctions* RIL_Init(const RIL_Env* env, int argc, char** argv) {
    char hardware[PROP_VALUE_MAX] = {};
    property_get("ro.hardware", hardware, "");
    const bool v1 = std::strcmp(hardware, "v1") == 0;
    if (!env || !env->OnRequestComplete || !env->OnUnsolicitedResponse ||
        !env->RequestTimedCallback || (!v1 && std::strcmp(hardware, "vee3") != 0)) {
        ALOGE("invalid environment or hardware; wrapper not initialized");
        return NULL;
    }
    pthread_mutex_lock(&init_mutex);
    if (backend_functions) {
        pthread_mutex_unlock(&init_mutex);
        return &exported;
    }
    set_status("initializing");
    backend_library = dlopen(kBackend, RTLD_NOW | RTLD_LOCAL);
    if (!backend_library) {
        ALOGE("cannot open real RIL backend: %s", dlerror());
        set_status("load_failed");
        pthread_mutex_unlock(&init_mutex);
        return NULL;
    }
    typedef const RIL_RadioFunctions* (*Init)(const RIL_Env*, int, char**);
    Init initialize = reinterpret_cast<Init>(dlsym(backend_library, "RIL_Init"));
    if (!initialize || initialize == &RIL_Init) {
        ALOGE("real RIL_Init missing or recursive");
        set_status("symbol_failed");
        // Keep the handle alive: a vendor initializer may retain callbacks.
        pthread_mutex_unlock(&init_mutex);
        return NULL;
    }
    framework_env = *env;
    vendor_env = *env;
    vendor_env.OnUnsolicitedResponse = on_unsolicited;
    if (v1) vendor_env.OnRequestComplete = on_v1_complete;
    const RIL_RadioFunctions* functions = initialize(&vendor_env, argc, argv);
    if (!functions || functions->version != 7 || !functions->onRequest ||
        !functions->onStateRequest) {
        ALOGE("unsupported RIL backend (expected version 7)");
        set_status("abi_failed");
        pthread_mutex_unlock(&init_mutex);
        return NULL;
    }
    backend_functions = functions;
    exported = *functions;
    exported.onRequest = v1 ? on_v1_request : on_request;
    exported.supports = supports;
    set_status("active-v7-seek");
    ALOGI("active: real RIL v7, legacy namespace and SEEK SIM_IO ABI restored");
    pthread_mutex_unlock(&init_mutex);
    return &exported;
}
