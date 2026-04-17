#include "network_manager.h"

#include <QDebug>
#include <QTimer>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , manager_(new QNetworkAccessManager(this)) {
    qDebug() << "NetworkManager: Служба запущена, сервер:" << base_url_;
}

NetworkManager::~NetworkManager() {
    for (auto* reply : request_storage_.keys()) {
        if (reply && reply->isRunning()) {
            reply->abort();
            reply->deleteLater();
        }
    }
    request_storage_.clear();
}

void NetworkManager::GET(const QString& endpoint) {
    sendRequest({endpoint, "GET", QJsonObject(), 0});
}

void NetworkManager::POST(const QString& endpoint, const QJsonObject& data) {
    sendRequest({endpoint, "POST", data, 0});
}

void NetworkManager::PATCH(const QString& endpoint, const QJsonObject& data) {
    sendRequest({endpoint, "PATCH", data, 0});
}

void NetworkManager::DELETE(const QString& endpoint, const QJsonObject& data) {
    sendRequest({endpoint, "DELETE", data, 0});
}

void NetworkManager::PUT(const QString& endpoint, const QJsonObject& data) {
    sendRequest({endpoint, "PUT", data, 0});
}

void NetworkManager::sendRequest(const RequestData& req_data) {
    QUrl url(base_url_ + req_data.endpoint_);
    QNetworkRequest request(url);

    if (!JWT_token_.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + JWT_token_).toUtf8());
    }
    QNetworkReply* reply = nullptr;

    if (req_data.method_ == "POST") {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QByteArray body_data = QJsonDocument(req_data.body_).toJson();
        reply = manager_->post(request, body_data);
    } else if (req_data.method_ == "GET") {
        reply = manager_->get(request);
    } else if (req_data.method_ == "PATCH") {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QByteArray body_data = QJsonDocument(req_data.body_).toJson();
        reply = manager_->sendCustomRequest(request, "PATCH", body_data);
    } else if (req_data.method_ == "DELETE") {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QByteArray body_data = QJsonDocument(req_data.body_).toJson();
        reply = manager_->sendCustomRequest(request, "DELETE", body_data);
    } else if (req_data.method_ == "PUT") {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QByteArray body_data = QJsonDocument(req_data.body_).toJson();
        reply = manager_->put(request, body_data);
    }

    if (reply) {
        request_storage_.insert(reply, req_data);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() { onResult(reply); });
        const QUrl endpoint_url("http://localhost" + req_data.endpoint_);
        qDebug() << "NetworkManager: Отправлен" << req_data.method_ << "на" << endpoint_url.path();
        if (!endpoint_url.query().isEmpty()) {
            qDebug() << "Query:" << endpoint_url.query();
        }
    }
}

void NetworkManager::onResult(QNetworkReply* reply) {
    if (!reply)
        return;

    RequestData req = request_storage_.take(reply);

    QString endpoint = req.endpoint_;

    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool is_retryable =
        retryable_codes_.contains(status_code) || (reply->error() == QNetworkReply::TimeoutError);

    if (is_retryable && req.retry_count < MAX_RETRIES_) {
        req.retry_count++;
        qDebug() << "NetworkManager: Попытка №" << req.retry_count << " для " << req.endpoint_;

        QTimer::singleShot(1000, this, [this, req]() { sendRequest(req); });

        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Ошибка сервера на " << endpoint << ":" << reply->errorString();
    }

    qDebug() << "--- Получен ответ ---";
    const QUrl endpoint_url("http://localhost" + endpoint);
    qDebug() << "Endpoint:" << endpoint_url.path();
    if (!endpoint_url.query().isEmpty()) {
        qDebug() << "Query:" << endpoint_url.query();
    }
    qDebug() << "Status:" << status_code;
    qDebug() << "Data:" << data;

    emit responseReceived(endpoint, data, status_code);
    reply->deleteLater();
}
