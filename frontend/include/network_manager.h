#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

class NetworkManager : public QObject {
    Q_OBJECT;

  public:
    explicit NetworkManager(QObject* parent = nullptr);

    const QString register_url_ = "/auth/v1/register";
    const QString info_url_ = "/personal/v1/info";

    void GET(const QString& endpoint);
    void POST(const QString& endpoint, const QJsonObject& data);

  signals:
    void responseReceived(const QString& endpoint, const QByteArray& data, int statusCode);

  private slots:
    void on_result(QNetworkReply* reply);

  private:
    QNetworkAccessManager* manager_;
    const QString base_url_ = "http://51.250.114.15:8080";
};
#endif // NETWORK_MANAGER_H
