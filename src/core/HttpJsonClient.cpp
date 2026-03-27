#include "HttpJsonClient.h"

#include <QEventLoop>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace llm {

HttpJsonClient::HttpJsonClient(int timeoutMs)
    : m_timeoutMs(timeoutMs) {}

HttpResponse HttpJsonClient::get(const QUrl& url,
                                 const QMap<QString, QString>& headers,
                                 const QList<QPair<QString, QString>>& queryItems) const {
    return send("GET", url, {}, headers, queryItems);
}

HttpResponse HttpJsonClient::post(const QUrl& url,
                                  const QJsonDocument& body,
                                  const QMap<QString, QString>& headers,
                                  const QList<QPair<QString, QString>>& queryItems) const {
    return send("POST", url, body.toJson(QJsonDocument::Compact), headers, queryItems);
}

HttpResponse HttpJsonClient::send(const QByteArray& verb,
                                  QUrl url,
                                  const QByteArray& body,
                                  const QMap<QString, QString>& headers,
                                  const QList<QPair<QString, QString>>& queryItems) const {
    QUrlQuery query(url);
    for (const auto& item : queryItems) {
        query.addQueryItem(item.first, item.second);
    }
    url.setQuery(query);

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("User-Agent", QByteArrayLiteral("LlmUsageMonitor/0.1"));
    request.setTransferTimeout(m_timeoutMs);

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply = nullptr;
    if (verb == "GET") {
        reply = manager.get(request);
    } else {
        reply = manager.sendCustomRequest(request, verb, body);
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    HttpResponse response;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.rawBody = reply->readAll();
    response.ok = reply->error() == QNetworkReply::NoError &&
                  response.statusCode >= 200 &&
                  response.statusCode < 300;

    if (reply->error() != QNetworkReply::NoError) {
        response.errorMessage = reply->errorString();
    }

    QJsonParseError parseError;
    response.json = QJsonDocument::fromJson(response.rawBody, &parseError);
    if (parseError.error != QJsonParseError::NoError && response.ok) {
        response.ok = false;
        response.errorMessage = parseError.errorString();
    }

    reply->deleteLater();
    return response;
}

}  // namespace llm
