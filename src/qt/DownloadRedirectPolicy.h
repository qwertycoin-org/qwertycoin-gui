// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>

namespace DownloadRedirectPolicy
{
    bool isRedirectStatus(int statusCode);
    bool isAllowedUrl(const QUrl &url, const QStringList &allowedHosts);
    QUrl resolve(const QUrl &currentUrl, const QString &location, const QStringList &allowedHosts);
}
