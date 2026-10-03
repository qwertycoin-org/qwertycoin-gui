// Copyright (c) 2020-2024, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "network.h"

#include <QDebug>
#include <QtCore>

#include "DownloadRedirectPolicy.h"
#include "utils.h"

using epee::net_utils::http::fields_list;
using epee::net_utils::http::http_response_info;
using epee::net_utils::http::abstract_http_client;

HttpClient::HttpClient(QObject *parent /* = nullptr */)
    : QObject(parent)
    , m_cancel(false)
    , m_contentLength(0)
    , m_received(0)
{
}

void HttpClient::cancel()
{
    m_cancel = true;
}

quint64 HttpClient::contentLength() const
{
    return m_contentLength;
}

quint64 HttpClient::received() const
{
    return m_received;
}

bool HttpClient::on_header(const http_response_info &headers)
{
    if (m_cancel.exchange(false))
    {
        return false;
    }

    size_t contentLength = 0;
    if (!epee::string_tools::get_xtype_from_string(contentLength, headers.m_header_info.m_content_length))
    {
        qWarning() << "Failed to get Content-Length";
    }
    m_contentLength = contentLength;
    emit contentLengthChanged();

    m_received = 0;
    emit receivedChanged();

    return net::http::client::on_header(headers);
}

bool HttpClient::handle_target_data(std::string &piece_of_transfer)
{
    if (m_cancel.exchange(false))
    {
        return false;
    }

    m_received += piece_of_transfer.size();
    emit receivedChanged();

    return net::http::client::handle_target_data(piece_of_transfer);
}

Network::Network(QObject *parent)
    : QObject(parent)
    , m_scheduler(this)
{
}

void Network::get(const QString &url, const QJSValue &callback, const QString &contentType /* = {} */) const
{
    m_scheduler.run(
        [this, url, contentType] {
            std::shared_ptr<abstract_http_client> httpClient = newClient();
            if (httpClient.get() == nullptr)
            {
                return QJSValueList({url, "", "failed to initialize a client"});
            }
            std::string response;
            QString error = get(httpClient, url, response, contentType);
            return QJSValueList({url, QString::fromStdString(response), error});
        },
        callback);
}

void Network::getJSON(const QString &url, const QJSValue &callback) const
{
    get(url, callback, "application/json; charset=utf-8");
}

std::string Network::get(const QString &url, const QString &contentType /* = {} */) const
{
    std::string response;
    std::shared_ptr<abstract_http_client> httpClient = newClient();
    if (httpClient.get() == nullptr)
    {
        throw std::runtime_error("failed to initialize a client");
    }
    QString error = get(httpClient, url, response, contentType);
    if (!error.isEmpty())
    {
        throw std::runtime_error(QString("failed to fetch %1: %2").arg(url).arg(error).toStdString());
    }
    return response;
}

QString Network::get(
    std::shared_ptr<abstract_http_client> httpClient,
    const QString &url,
    std::string &response,
    const QString &contentType /* = {} */,
    const QStringList &allowedRedirectHosts /* = {} */,
    const int maximumRedirects /* = 0 */) const
{
    if (maximumRedirects < 0 || maximumRedirects > 10 ||
        (maximumRedirects > 0 && allowedRedirectHosts.isEmpty()))
        return "invalid redirect policy";

    response.clear();

    QUrl currentUrl = QUrl::fromEncoded(url.toUtf8(), QUrl::StrictMode);
    if (!currentUrl.isValid())
        return "invalid URL";

    const bool redirectsEnabled = maximumRedirects > 0;
    if (redirectsEnabled && !DownloadRedirectPolicy::isAllowedUrl(currentUrl, allowedRedirectHosts))
        return "untrusted download URL";

    QSet<QByteArray> visitedUrls;
    visitedUrls.insert(currentUrl.toEncoded(QUrl::FullyEncoded | QUrl::RemoveFragment));

    for (int redirectCount = 0;;)
    {
        const bool isHttps = currentUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
        const auto sslSupport = isHttps
            ? epee::net_utils::ssl_support_t::e_ssl_support_enabled
            : epee::net_utils::ssl_support_t::e_ssl_support_disabled;

        httpClient->set_server(
            currentUrl.host().toStdString(),
            std::to_string(currentUrl.port(isHttps ? 443 : 80)),
            {},
            epee::net_utils::ssl_options_t{sslSupport});

        QString path = currentUrl.path(QUrl::FullyEncoded);
        if (path.isEmpty())
            path = QStringLiteral("/");
        const QString uri = currentUrl.hasQuery()
            ? path + QStringLiteral("?") + currentUrl.query(QUrl::FullyEncoded)
            : path;
        const http_response_info *pri = NULL;
        constexpr std::chrono::milliseconds timeout = std::chrono::seconds(15);

        fields_list headers({{"User-Agent", randomUserAgent().toStdString()}});
        if (!contentType.isEmpty())
            headers.push_back({"Content-Type", contentType.toStdString()});

        const bool result = httpClient->invoke(
            uri.toStdString(), "GET", {}, timeout, std::addressof(pri), headers);
        if (!result)
            return "unknown error";
        if (!pri)
            return "internal error";
        if (pri->m_response_code == 200)
        {
            response = std::move(pri->m_body);
            return {};
        }
        if (!redirectsEnabled || !DownloadRedirectPolicy::isRedirectStatus(pri->m_response_code))
            return QString("response code %1").arg(pri->m_response_code);
        if (redirectCount >= maximumRedirects)
            return "too many redirects";

        QString redirectLocation;
        for (const auto &field : pri->m_header_info.m_etc_fields)
        {
            if (!epee::string_tools::compare_no_case(field.first, "Location"))
            {
                if (!redirectLocation.isEmpty())
                    return "multiple redirect locations";
                redirectLocation = QString::fromStdString(field.second);
            }
        }
        const QUrl redirectUrl = DownloadRedirectPolicy::resolve(
            currentUrl, redirectLocation, allowedRedirectHosts);
        if (!redirectUrl.isValid())
            return "invalid or untrusted redirect";

        const QByteArray encodedRedirect = redirectUrl.toEncoded(QUrl::FullyEncoded | QUrl::RemoveFragment);
        if (visitedUrls.contains(encodedRedirect))
            return "redirect cycle";
        visitedUrls.insert(encodedRedirect);
        currentUrl = redirectUrl;
        ++redirectCount;
    }
}

std::shared_ptr<abstract_http_client> Network::newClient() const
{
    std::shared_ptr<abstract_http_client> client(new net::http::client());
    if (!client->set_proxy(m_proxyAddress.toStdString()))
    {
        throw std::runtime_error("failed to set proxy address");
    }
    return client;
}
