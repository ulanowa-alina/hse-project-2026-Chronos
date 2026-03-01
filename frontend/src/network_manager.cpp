#include "network_manager.h"

#include <QDebug>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , manager_(new QNetworkAccessManager(this)) {
    connect(manager_, &QNetworkAccessManager::finished, this, &NetworkManager::on_result);
    qDebug() << "NetworkManager: Служба запущена, сервер:" << base_url_;
}

void NetworkManager::GET(const QString& endpoint) {
    QUrl url(base_url_ + endpoint);
    QNetworkRequest request(url);

    manager_->get(request);
    qDebug() << "NetworkManager: Отправлен GET на" << endpoint;
}

void NetworkManager::POST(const QString& endpoint, const QJsonObject& data) {
    QUrl url(base_url_ + endpoint);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(data);

    manager_->post(request, doc.toJson());
    qDebug() << "NetworkManager: Отправлен POST на" << endpoint;
}

void NetworkManager::on_result(QNetworkReply* reply) {
    QString endpoint = reply->url().path();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QByteArray data = reply->readAll();
    qDebug() << "--- Получен ответ ---";
    qDebug() << "Endpoint:" << endpoint;
    qDebug() << "Status:" << statusCode;
    qDebug() << "Data:" << data;

    emit responseReceived(endpoint, data, statusCode);
    reply->deleteLater();
}
