// SPDX-License-Identifier: Apache-2.0
// Included inside ril_compat.cpp's private namespace, after backend_functions.
// Preserve the framework token and the vendor response; never invent identity.
struct PendingBootIdentity {
    int request;
    RIL_Token token;
    struct timespec deadline;
};

bool boot_identity_expired(const struct timespec& now,
                           const struct timespec& deadline) {
    return now.tv_sec > deadline.tv_sec ||
        (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec);
}

void resume_boot_identity(void* opaque) {
    PendingBootIdentity* pending = static_cast<PendingBootIdentity*>(opaque);
    struct timespec now = {};
    const bool ready = backend_functions->onStateRequest() == RADIO_STATE_ON;
    const bool expired = clock_gettime(CLOCK_MONOTONIC, &now) != 0 ||
        boot_identity_expired(now, pending->deadline);
    if (!ready && !expired) {
        const struct timeval delay = {0, 250000};
        framework_env.RequestTimedCallback(resume_boot_identity, pending, &delay);
        return;
    }
    const int request = pending->request;
    RIL_Token token = pending->token;
    delete pending;
    ALOGI("identity request %d: forwarding real query (radio_ready=%d)",
          request, ready);
    backend_functions->onRequest(request, NULL, 0, token);
}

bool defer_boot_identity(int request, void* data, size_t length, RIL_Token token) {
    if ((request != RIL_REQUEST_BASEBAND_VERSION &&
         request != RIL_REQUEST_DEVICE_IDENTITY) || data != NULL || length != 0 ||
        backend_functions->onStateRequest() == RADIO_STATE_ON)
        return false;
    PendingBootIdentity* pending = new (std::nothrow) PendingBootIdentity;
    if (!pending) return false;
    if (clock_gettime(CLOCK_MONOTONIC, &pending->deadline) != 0) {
        delete pending;
        return false;
    }
    pending->deadline.tv_sec += 10;
    pending->request = request;
    pending->token = token;
    const struct timeval delay = {0, 250000};
    ALOGI("identity request %d: deferring until radio ON, deadline 10s", request);
    framework_env.RequestTimedCallback(resume_boot_identity, pending, &delay);
    return true;
}
