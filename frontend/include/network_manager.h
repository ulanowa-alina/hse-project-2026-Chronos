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
    const QString user_info_url_ = "/personal/v1/info";
    const QString tasks_edit_url_ = "/task/v1/edit";
    const QString tasks_create_url_ = "/board/v1/tasks/create";
    const QString statuses_edit_url_ = "/status/v1/edit";
    const QString statuses_create_url_ = "/status/v1/create";
    const QString login_url_ = "/auth/v1/login";
    const QString tasks_delete_url_ = "/tasks/v1/delete";
    const QString statuses_delete_url_ = "/status/v1/delete";
    const QString user_edit_info_url_ = "/personal/v1/edit";

    void GET(const QString& endpoint);
    void POST(const QString& endpoint, const QJsonObject& data);
    void PATCH(const QString& endpoint, const QJsonObject& data);
    void DELETE(const QString& endpoint, const QJsonObject& data);
    void PUT(const QString& endpoint, const QJsonObject& data);

    void setToken(const QString& new_token) {
        JWT_token_ = new_token;
    }
    void clearToken() {
        JWT_token_.clear();
    }

  signals:
    void responseReceived(const QString& endpoint, const QByteArray& data, int statusCode);

  private slots:
    void onResult(QNetworkReply* reply);

  private:
    QNetworkAccessManager* manager_;
    QMap<QNetworkReply*, RequestData> request_storage_;
    QString JWT_token_;

    const QSet<int> retryable_codes_ = {500};

    const QString base_url_ = "http://127.0.0.1:8080";
    const int MAX_RETRIES_ = 3;

    void sendRequest(const RequestData& req_data);
};
#endif // NETWORK_MANAGER_H
