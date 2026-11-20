/*
   Copyright (c) 2013, The Linux Foundation. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <android-base/logging.h>
#include <android-base/properties.h>

#include "vendor_init.h"
#include "property_service.h"

using android::base::GetProperty;
using android::init::property_set;

#ifndef LGE_GEN2_VARIANT_H
#define LGE_GEN2_VARIANT_H
namespace lge_gen2 {
struct Profile {
    const char* device;
    const char* family;
    const char* models[4];
    const char* dual_sim_models[2];
    bool configure_dual_sim;
};
static const Profile kV1 = {"v1", "LG Optimus L1 II",
    {"E410", "E411", "E415", "E420"}, {"E415", "E420"}, false};
static const Profile kVee3 = {"vee3", "LG Optimus L3 II",
    {"E425", "E430", "E431", "E435"}, {"E435", NULL}, true};
class Detector {
    const Profile& profile_;
public:
    explicit Detector(const Profile& profile) : profile_(profile) {}
    bool allowed_model(const std::string& model) const {
        for (size_t i = 0; i < 4; ++i) if (model == profile_.models[i]) return true;
        return false;
    }
    bool dual_sim_model(const std::string& model) const {
        for (size_t i = 0; i < 2; ++i) {
            if (profile_.dual_sim_models[i] &&
                model.compare(0, 7, std::string("LG-") + profile_.dual_sim_models[i]) == 0)
                return true;
        }
        return false;
    }
    bool unavailable_baseband(const std::string& value) const {
        std::string clean;
        for (size_t i = 0; i < value.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(value[i]);
            if (!isspace(c)) clean += static_cast<char>(tolower(c));
        }
        return clean.empty() || clean == "unknown";
    }


struct Evidence {
    std::set<std::string> models;
    std::set<std::string> versions;
};

bool token_char(char c) {
    return isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '+';
}

bool pair_at(const std::string& text, size_t offset, char first, char second) {
    return offset + 2 <= text.size() &&
        toupper(static_cast<unsigned char>(text[offset])) == first &&
        toupper(static_cast<unsigned char>(text[offset + 1])) == second;
}

void inspect(const std::string& text, bool firmware, Evidence* evidence, bool terminated = true) {
    for (size_t i = 0; i + 4 <= text.size(); ++i) {
        if (i && isalnum(static_cast<unsigned char>(text[i - 1]))) continue;
        size_t offset = i;
        if (pair_at(text, offset, 'L', 'G')) {
            offset += 2;
            if (offset < text.size() && text[offset] == '-') ++offset;
        }
        if (offset + 4 > text.size() || toupper(static_cast<unsigned char>(text[offset])) != 'E') continue;
        std::string base = text.substr(offset, 4);
        base[0] = 'E';
        // Only identifiers belonging to the selected hardware family are accepted.
        if (!allowed_model(base)) continue;
        size_t end = offset + 4;
        std::string model = "LG-" + base;
        // LG stock firmware identifiers include LGE410fAT-00-V10a and LGE425fAT-00-V10i.
        // AT is a firmware tag, while the preceding optional letter is the SKU suffix.
        if (firmware && pair_at(text, end, 'A', 'T')) {
            end += 2;
        } else if (firmware && end < text.size() &&
                   isalpha(static_cast<unsigned char>(text[end])) && pair_at(text, end + 1, 'A', 'T')) {
            model += static_cast<char>(toupper(static_cast<unsigned char>(text[end])));
            end += 3;
        } else if (end < text.size() && isalpha(static_cast<unsigned char>(text[end]))) {
            if (end + 1 < text.size() && isalnum(static_cast<unsigned char>(text[end + 1]))) continue;
            model += static_cast<char>(toupper(static_cast<unsigned char>(text[end])));
            ++end;
        }
        if (end < text.size() && isalnum(static_cast<unsigned char>(text[end]))) continue;
        if (firmware) {
            if (end >= text.size() || (text[end] != '-' && text[end] != '_')) continue;
            size_t stop = end;
            while (stop < text.size() && token_char(text[stop]) && stop - i < PROP_VALUE_MAX - 1U) ++stop;
            if ((!terminated && stop == text.size()) ||
                (stop < text.size() && token_char(text[stop]))) continue;
            std::string version = text.substr(i, stop - i);
            std::string upper = version;
            for (size_t j = 0; j < upper.size(); ++j)
                upper[j] = static_cast<char>(toupper(static_cast<unsigned char>(upper[j])));
            size_t marker = upper.find("-V");
            if (marker == std::string::npos || marker + 3 >= version.size() ||
                !isdigit(static_cast<unsigned char>(version[marker + 2])) ||
                !isdigit(static_cast<unsigned char>(version[marker + 3]))) continue;
            if (evidence->versions.size() < 16) evidence->versions.insert(version);
        }
        evidence->models.insert(model);
        // Do not parse the short model again inside an accepted LG-prefixed token.
        i = end - 1;
    }
}

std::string resolve(const Evidence& evidence) {
    std::string base, qualified;
    for (std::set<std::string>::const_iterator i = evidence.models.begin(); i != evidence.models.end(); ++i) {
        if (base.empty()) base = i->substr(0, 7);
        if (i->substr(0, 7) != base) return "";
        if (i->size() > 7) {
            if (!qualified.empty() && qualified != *i) return "";
            qualified = *i;
        }
    }
    return qualified.empty() ? base : qualified;
}

std::string read_text(const std::string& path, size_t limit) {
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return "";
    std::string text;
    char buffer[4096];
    while (text.size() < limit) {
        size_t wanted = limit - text.size();
        if (wanted > sizeof(buffer)) wanted = sizeof(buffer);
        ssize_t count = read(fd, buffer, wanted);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        text.append(buffer, static_cast<size_t>(count));
    }
    close(fd);
    return text;
}

void inspect_factory(const std::string& path, Evidence* evidence) {
    std::istringstream input(read_text(path, 65536));
    std::string line;
    while (std::getline(input, line)) {
        size_t split = line.find('=');
        if (split == std::string::npos) continue;
        std::string key = line.substr(0, split);
        if (key == "ro.product.model" || key == "ro.lge.model" || key == "ro.lge.model_name")
            inspect(line.substr(split + 1), false, evidence);
        if (key == "ro.lge.swversion") inspect(line.substr(split + 1), true, evidence);
    }
}

std::vector<std::string> directory_entries(const std::string& path) {
    std::vector<std::string> entries;
    DIR* directory = opendir(path.c_str());
    if (!directory) return entries;
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_name[0] != '.') entries.push_back(entry->d_name);
    }
    closedir(directory);
    return entries;
}

void append_named(const std::string& directory, std::vector<std::string>* paths) {
    const char* names[] = {"modem", "radio", "amss"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        std::string path = directory + "/" + names[i];
        struct stat st;
        if (stat(path.c_str(), &st) == 0 && S_ISBLK(st.st_mode)) paths->push_back(path);
    }
}

bool modem_name(const std::string& name) {
    return name == "modem" || name == "radio" || name == "amss";
}

std::vector<std::string> modem_paths(bool* legacy) {
    std::vector<std::string> paths;
    append_named("/dev/block/by-name", &paths);
    append_named("/dev/block/bootdevice/by-name", &paths);
    std::vector<std::string> controllers = directory_entries("/dev/block/platform");
    for (size_t i = 0; i < controllers.size(); ++i)
        append_named("/dev/block/platform/" + controllers[i] + "/by-name", &paths);
    std::vector<std::string> blocks = directory_entries("/sys/class/block");
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].compare(0, 6, "mmcblk") != 0) continue;
        std::istringstream input(read_text("/sys/class/block/" + blocks[i] + "/uevent", 4096));
        std::string line, name, device;
        while (std::getline(input, line)) {
            if (line.compare(0, 9, "PARTNAME=") == 0) name = line.substr(9);
            if (line.compare(0, 8, "DEVNAME=") == 0) device = line.substr(8);
        }
        if (modem_name(name) && device.compare(0, 6, "mmcblk") == 0 &&
            device.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") == std::string::npos)
            paths.push_back("/dev/block/" + device);
    }
    // Old gen2 bootloaders use an unlabeled MBR. Keep the existing DT map as
    // a fallback only; model identity is still read from the phone's firmware.
    *legacy = paths.empty();
    if (*legacy) paths.push_back("/dev/block/mmcblk0p12");
    return paths;
}

void inspect_modem(const std::string& path, std::set<dev_t>* seen, Evidence* evidence) {
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISBLK(st.st_mode) || !seen->insert(st.st_rdev).second) {
        close(fd);
        return;
    }
    const size_t limit = 128U * 1024U * 1024U;
    size_t total = 0;
    char buffer[16384];
    std::string printable;
    Evidence local;
    bool complete = false;
    while (total < limit) {
        size_t wanted = limit - total;
        if (wanted > sizeof(buffer)) wanted = sizeof(buffer);
        ssize_t count = read(fd, buffer, wanted);
        if (count < 0 && errno == EINTR) continue;
        if (count == 0) { complete = true; break; }
        if (count < 0) break;
        total += static_cast<size_t>(count);
        for (ssize_t i = 0; i < count; ++i) {
            unsigned char c = static_cast<unsigned char>(buffer[i]);
            if (c >= 32 && c <= 126) {
                printable += static_cast<char>(c);
                if (printable.size() == 512) {
                    inspect(printable, true, &local, false);
                    printable.erase(0, 256);
                }
            } else {
                if (printable.size() >= 7) inspect(printable, true, &local);
                printable.clear();
            }
        }
    }
    if (total == limit) {
        char extra;
        complete = read(fd, &extra, 1) == 0;
    }
    close(fd);
    if (!complete) {
        LOG(WARNING) << "LgeGen2Variant: modem identity scan incomplete; keeping this source untrusted";
        return;
    }
    inspect(printable, true, &local);
    evidence->models.insert(local.models.begin(), local.models.end());
    evidence->versions.insert(local.versions.begin(), local.versions.end());
}

void override_property(const char* name, const std::string& value) {
    if (value.size() >= PROP_VALUE_MAX) return;
    prop_info* property = const_cast<prop_info*>(__system_property_find(name));
    int result = property ? __system_property_update(property, value.c_str(), value.size()) :
        __system_property_add(name, strlen(name), value.c_str(), value.size());
    if (result != 0) LOG(ERROR) << "LgeGen2Variant: unable to publish property " << name;
}

void Load() {
    Evidence evidence;
    std::string source = "bootloader";
    const char* boot_keys[] = {"ro.boot.model", "ro.boot.product.model", "ro.boot.lge.model",
        "ro.boot.lge_model_name", "ro.boot.hardware.sku"};
    for (size_t i = 0; i < sizeof(boot_keys) / sizeof(boot_keys[0]); ++i)
        inspect(GetProperty(boot_keys[i], ""), false, &evidence);
    std::istringstream command_line(read_text("/proc/cmdline", 8192));
    std::string token;
    while (command_line >> token) {
        size_t equal = token.find('=');
        if (equal == std::string::npos) continue;
        std::string key = token.substr(0, equal);
        if (key == "lge.model" || key == "lge.model_name" || key == "androidboot.model" ||
            key == "androidboot.lge_model_name") inspect(token.substr(equal + 1), false, &evidence);
    }
    if (evidence.models.empty()) {
        source = "factory";
        inspect_factory("/factory/factory.prop", &evidence);
        inspect_factory("/mpt/factory.prop", &evidence);
    }
    if (evidence.models.empty() ||
        (evidence.versions.empty() && unavailable_baseband(GetProperty("gsm.version.baseband", "")))) {
        bool legacy = false;
        std::vector<std::string> paths = modem_paths(&legacy);
        std::set<dev_t> seen;
        Evidence firmware;
        std::string firmware_source = legacy ? "modem_legacy_map" : "modem_named";
        for (size_t i = 0; i < paths.size(); ++i) inspect_modem(paths[i], &seen, &firmware);
        if (evidence.models.empty()) {
            evidence = firmware;
            source = firmware_source;
        } else {
            std::string first = resolve(evidence), second = resolve(firmware);
            if (!first.empty() && !second.empty() && first.substr(0, 7) == second.substr(0, 7) &&
                (first.size() == 7 || second.size() == 7 || first == second)) {
                evidence.models.insert(firmware.models.begin(), firmware.models.end());
                evidence.versions.insert(firmware.versions.begin(), firmware.versions.end());
                source += "+" + firmware_source;
            }
        }
    }
    if (evidence.models.empty()) {
        Evidence reported;
        inspect(GetProperty("gsm.version.baseband", ""), true, &reported);
        if (!reported.models.empty()) {
            evidence = reported;
            source = "baseband_property";
        }
    }
    std::string model = resolve(evidence);
    std::string status = !model.empty() ? "detected" :
        evidence.models.empty() ? "not_found" : "ambiguous";
    // Product properties already exist here on Oreo. Update them in init;
    // a later shell setprop cannot replace an existing read-only property.
    override_property("ro.product.device", profile_.device);
    override_property("ro.product.model", model.empty() ? profile_.family : model);
    override_property("ro.lge.variant.source", source);
    override_property("ro.lge.variant.status", status);
    override_property("ro.lge.baseband.source", unavailable_baseband(GetProperty("gsm.version.baseband", ""))
        ? "unavailable" : "existing_property");
    if (!model.empty()) {
        // Preserve the original DT's dual-SIM rule. Do not erase existing
        // persistent radio settings on single-SIM or unknown variants.
        if (profile_.configure_dual_sim && dual_sim_model(model)) {
            property_set("persist.radio.multisim.config", "dsds");
            property_set("persist.multisim.config", "dsds");
            override_property("ro.multi.rild", "true");
        }
        if (evidence.versions.size() == 1 && unavailable_baseband(GetProperty("gsm.version.baseband", ""))) {
            const uint32_t result = property_set("gsm.version.baseband", *evidence.versions.begin());
            override_property("ro.lge.baseband.source", result == 0 ? "firmware:" + source : "publish_failed");
        }
    }
    LOG(INFO) << "LgeGen2Variant: device=" << profile_.device << " model=" << (model.empty() ? "unknown" : model)
              << " source=" << source << " status=" << status
              << " firmware_candidates=" << evidence.versions.size();
}


};  // class Detector
}  // namespace lge_gen2
#endif  // LGE_GEN2_VARIANT_H
