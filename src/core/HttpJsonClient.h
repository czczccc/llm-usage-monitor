#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QUrl>

namespace llm {

struct HttpResponse {
    bool ok = false;
    int statusCode = 0;
    QByteArray rawBody;
    QJsonDocument json;
    QString errorMessage;
};

class HttpJsonClient {
public:
    explicit HttpJsonClient(int timeoutMs = 30000);

    HttpResponse get(const QUrl& url,
                     const QMap<QString, QString>& headers = {},
                     const QList<QPair<QString, QString>>& queryItems = {}) const;

    HttpResponse post(const QUrl& url,
                      const QJsonDocument& body,
                      const QMap<QString, QString>& headers = {},
                      const QList<QPair<QString, QString>>& queryItems = {}) const;

private:
    HttpResponse send(const QByteArray& verb,
                      QUrl url,
                      const QByteArray& body,
                      const QMap<QString, QString>& headers,
                      const QList<QPair<QString, QString>>& queryItems) const;

    int m_timeoutMs = 30000;
};

}  // namespace llm
