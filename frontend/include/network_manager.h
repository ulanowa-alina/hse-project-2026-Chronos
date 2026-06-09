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
    QString sync_entity_;
    int sync_local_id_ = 0;
    QString sync_operation_;
    bool is_sync_request_ = false;
};

class NetworkManager : public QObject {
    Q_OBJECT;

  public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    const QString register_url_ = "/auth/v1/register";
    const QString login_url_ = "/auth/v1/login";
    const QString user_edit_info_url_ = "/personal/v1/edit";
    const QString user_info_url_ = "/personal/v1/info";

    const QString tasks_edit_url_ = "/task/v1/edit";
    const QString tasks_create_url_ = "/task/v1/create";
    const QString tasks_get_all_url_ = "/task/v1/get_all";
    const QString tasks_delete_url_ = "/task/v1/delete";

    const QString statuses_edit_url_ = "/status/v1/edit";
    const QString statuses_create_url_ = "/status/v1/create";
    const QString statuses_get_all_url_ = "/status/v1/get_all";
    const QString statuses_delete_url_ = "/status/v1/delete";
    const QString statieses_edit_url_ = "/status/v1/edit";

    const QString board_get_url_ = "/board/v1/get";
    const QString boards_get_all_url_ = "/board/v1/get_all";
    const QString boards_create_url_ = "/board/v1/create";
    const QString boards_edit_url_ = "/board/v1/edit";
    const QString board_tasks_url_ = "/board/v1/tasks";
    const QString boards_delete_url_ = "/board/v1/delete";

    const QString pomodoro_create_url_ = "/pomodoro/v1/create";
    const QString pomodoro_get_user_sessions_url_ = "/pomodoro/v1/get_user_sessions";


    void GET(const QString& endpoint);
    void POST(const QString& endpoint, const QJsonObject& data);
    void PATCH(const QString& endpoint, const QJsonObject& data);
    void DELETE(const QString& endpoint, const QJsonObject& data);
    void PUT(const QString& endpoint, const QJsonObject& data);

    void syncPOST(const QString& endpoint, const QJsonObject& data, const QString& entity,
                  int local_id, const QString& operation);
    void syncPATCH(const QString& endpoint, const QJsonObject& data, const QString& entity,
                   int local_id, const QString& operation);
    void syncDELETE(const QString& endpoint, const QJsonObject& data, const QString& entity,
                    int local_id, const QString& operation);
    void syncPUT(const QString& endpoint, const QJsonObject& data, const QString& entity,
                 int local_id, const QString& operation);

    void setToken(const QString& new_token) {
        JWT_token_ = new_token;
    }
    void clearToken() {
        JWT_token_.clear();
    }
    bool hasToken() const {
        return !JWT_token_.isEmpty();
    }

  signals:
    void responseReceived(const QString& endpoint, const QByteArray& data, int statusCode);
    void syncResponseReceived(const QString& endpoint, const QByteArray& data, int statusCode,
                              const QString& entity, int localId, const QString& operation);

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
