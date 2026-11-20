// Copyright (C) 2026 Vee3 bring-up contributors
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <telephony/ril.h>

namespace lge_ril {

// Matches TeamHackLG's RIL_VARIANT_LEGACY command table, not its full CAF
// table: the legacy mode implemented only these three post-108 requests.
// In particular, framework 115 (SIM_OPEN_CHANNEL) must never become the
// vendor's 115 (MODIFY_QOS), whose payload has an entirely different layout.
inline int request_to_legacy(int request) {
    if (request >= 1 && request <= RIL_REQUEST_VOICE_RADIO_TECH) return request;
    switch (request) {
        case RIL_REQUEST_IMS_REGISTRATION_STATE: return 109;
        case RIL_REQUEST_IMS_SEND_SMS: return 110;
        case RIL_REQUEST_SET_UICC_SUBSCRIPTION: return 118;
        default: return -1;
    }
}

// Reverse mapping from the same transport's unsolicited-response table.
inline int unsolicited_to_framework(int indication) {
    if (indication >= RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED &&
        indication <= RIL_UNSOL_VOICE_RADIO_TECH_CHANGED) return indication;
    switch (indication) {
        case 1036: return RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED;
        case 1039: return RIL_UNSOL_ON_SS;
        case 1040: return RIL_UNSOL_STK_CC_ALPHA_NOTIFY;
        case 1041: return RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED;
        // Legacy tethering/QoS events have no corresponding Oreo payload.
        // Forwarding their numbers unchanged would reinterpret unrelated data.
        default: return -1;
    }
}

}  // namespace lge_ril
