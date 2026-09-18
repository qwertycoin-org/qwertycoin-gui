// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#include "UpdateMetadata.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <set>

#include <boost/algorithm/string.hpp>

#include "common/dns_utils.h"
#include "common/util.h"
#include "misc_log_ex.h"

namespace
{
    constexpr const char updateHostname[] = "updates.qwertycoin.org";
    constexpr const char software[] = "qwertycoin-gui";
    constexpr const char unpublishedPlaceholder[] = "qwc:update-metadata-not-yet-published";

    bool supportedTarget(const std::string &buildTag)
    {
        return buildTag == "linux-x64" || buildTag == "mac-armv8" ||
            buildTag == "install-win-x64" || buildTag == "win-x64";
    }

    bool validVersion(const std::string &version)
    {
        if (version.empty() || version.size() > 32)
            return false;

        size_t components = 1;
        size_t componentDigits = 0;
        uint64_t componentValue = 0;
        bool previousWasDot = false;
        for (const unsigned char c : version)
        {
            if (c == '.')
            {
                if (previousWasDot || componentDigits == 0 || componentValue > std::numeric_limits<int>::max())
                    return false;
                previousWasDot = true;
                componentDigits = 0;
                componentValue = 0;
                ++components;
            }
            else
            {
                if (c < '0' || c > '9')
                    return false;
                if (componentDigits != 0 && componentValue == 0)
                    return false;
                if (++componentDigits > 9)
                    return false;
                componentValue = componentValue * 10 + (c - '0');
                previousWasDot = false;
            }
        }
        return !previousWasDot && componentDigits != 0 && componentValue <= std::numeric_limits<int>::max() &&
            components == 3;
    }

    bool validSha256(const std::string &hash)
    {
        return hash.size() == 64 && std::all_of(hash.begin(), hash.end(), [](const unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    }
}

UpdateMetadata::Result UpdateMetadata::evaluate(
    const std::vector<std::string> &records,
    const std::string &buildTag,
    const std::string &currentVersion)
{
    Result result;
    if (!supportedTarget(buildTag) || !validVersion(currentVersion))
        return result;

    std::map<std::string, std::set<std::string>> matches;
    for (const auto &record : records)
    {
        if (record == unpublishedPlaceholder)
            continue;

        std::vector<std::string> fields;
        boost::split(fields, record, boost::is_any_of(":"));
        if (fields.size() != 4)
        {
            if (boost::starts_with(record, std::string(software) + ":" + buildTag + ":"))
                return {};
            continue;
        }
        if (fields[0] != software || fields[1] != buildTag)
            continue;
        if (!validVersion(fields[2]) || !validSha256(fields[3]))
            return {};
        matches[fields[2]].insert(fields[3]);
    }

    for (const auto &match : matches)
    {
        if (match.second.size() != 1)
            return {};
        if (result.version.empty() || tools::vercmp(result.version.c_str(), match.first.c_str()) < 0)
        {
            result.version = match.first;
            result.hash = *match.second.begin();
        }
    }

    if (result.version.empty() || tools::vercmp(result.version.c_str(), currentVersion.c_str()) <= 0)
        return {};

    result.downloadUrl = downloadUrl(buildTag, result.version);
    result.available = !result.downloadUrl.empty();
    if (!result.available)
        return {};
    return result;
}

std::string UpdateMetadata::downloadUrl(const std::string &buildTag, const std::string &version)
{
    if (!supportedTarget(buildTag) || !validVersion(version))
        return {};

    std::string filename;
    if (buildTag == "linux-x64")
        filename = "qwertycoin-gui-v" + version + "-linux-x86_64.tar.gz";
    else if (buildTag == "mac-armv8")
        filename = "qwertycoin-gui-v" + version + "-macos-arm64.dmg";
    else if (buildTag == "install-win-x64")
        filename = "qwertycoin-gui-v" + version + "-windows-x86_64-setup.exe";
    else if (buildTag == "win-x64")
        filename = "qwertycoin-gui-v" + version + "-windows-x86_64.zip";

    return "https://github.com/qwertycoin-org/qwertycoin-gui/releases/download/v" + version + "/" + filename;
}

UpdateMetadata::Result UpdateMetadata::check(
    const std::string &buildTag,
    const std::string &currentVersion) const
{
    if (!supportedTarget(buildTag) || !validVersion(currentVersion))
        return {};

    bool dnssecAvailable = false;
    bool dnssecValid = false;
    const auto records = tools::DNSResolver::instance().get_txt_record(
        updateHostname, dnssecAvailable, dnssecValid);
    if (!dnssecAvailable || !dnssecValid)
    {
        MWARNING("Ignoring GUI update metadata because DNSSEC validation failed for " << updateHostname);
        return {};
    }

    return evaluate(records, buildTag, currentVersion);
}
