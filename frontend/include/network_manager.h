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

class RequestData {
  public:
    QString endpoint_;
    QString method_;
    QJsonObject body_;
    int retry_count = 0;
};

class NetworkManager : public QObject {
    Q_OBJECT;

  public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    const QString register_url_ = "/auth/v1/register";
    const QString info_url_ = "/personal/v1/info";

    void GET(const QString& endpoint);
    void POST(const QString& endpoint, const QJsonObject& data);

  signals:
    void responseReceived(const QString& endpoint, const QByteArray& data, int statusCode);

  private slots:
    void onResult(QNetworkReply* reply);

  private:
    QNetworkAccessManager* manager_;
    QMap<QNetworkReply*, RequestData> request_storage_;

    const QSet<int> retryable_codes_ = {500};

    const QString base_url_ = "http://51.250.114.15:8080";
    const int MAX_RETRIES_ = 3;

    void sendRequest(const RequestData& req_data);
};
#endif // NETWORK_MANAGER_H
