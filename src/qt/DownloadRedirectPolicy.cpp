// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#include "DownloadRedirectPolicy.h"

#include <algorithm>

namespace
{
    constexpr int maximumLocationLength = 8192;

    bool containsControlCharacter(const QString &value)
    {
        return std::any_of(value.cbegin(), value.cend(), [](const QChar character) {
            const ushort code = character.unicode();
            return code <= 0x1f || code == 0x7f;
        });
    }
}

bool DownloadRedirectPolicy::isRedirectStatus(const int statusCode)
{
    return statusCode == 301 || statusCode == 302 || statusCode == 303 ||
        statusCode == 307 || statusCode == 308;
}

bool DownloadRedirectPolicy::isAllowedUrl(const QUrl &url, const QStringList &allowedHosts)
{
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0 ||
        !url.userInfo().isEmpty() || (url.port(-1) != -1 && url.port(-1) != 443))
    {
        return false;
    }

    const QString host = url.host(QUrl::FullyDecoded);
    if (host.isEmpty())
        return false;

    return std::any_of(allowedHosts.cbegin(), allowedHosts.cend(), [&host](const QString &allowedHost) {
        return host.compare(allowedHost, Qt::CaseInsensitive) == 0;
    });
}

QUrl DownloadRedirectPolicy::resolve(
    const QUrl &currentUrl,
    const QString &location,
    const QStringList &allowedHosts)
{
    if (location.isEmpty() || location.size() > maximumLocationLength || containsControlCharacter(location))
        return {};

    const QUrl locationUrl = QUrl::fromEncoded(location.toUtf8(), QUrl::StrictMode);
    if (!locationUrl.isValid())
        return {};

    QUrl target = currentUrl.resolved(locationUrl);
    target.setFragment({});
    if (!isAllowedUrl(target, allowedHosts))
        return {};
    return target;
}
