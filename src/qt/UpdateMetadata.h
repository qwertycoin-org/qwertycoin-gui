// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <string>
#include <vector>

class UpdateMetadata
{
public:
    struct Result
    {
        bool available = false;
        std::string version;
        std::string hash;
        std::string downloadUrl;
    };

    static Result evaluate(
        const std::vector<std::string> &records,
        const std::string &buildTag,
        const std::string &currentVersion);
    static std::string downloadUrl(const std::string &buildTag, const std::string &version);

    Result check(const std::string &buildTag, const std::string &currentVersion) const;
};
