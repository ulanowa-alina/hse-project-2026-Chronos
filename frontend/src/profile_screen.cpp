#include "profile_screen.h"

#include "../local_repositories/local_user_repository.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QShowEvent>

ProfileScreen::ProfileScreen(QWidget* parent)
    : QWidget(parent)
    , avatar_network_manager_(new QNetworkAccessManager(this)) {
    setupLayout();

    connect(avatar_network_manager_, &QNetworkAccessManager::finished, this,
            &ProfileScreen::onAvatarImageDownloaded);
}

void ProfileScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void ProfileScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void ProfileScreen::setNetworkManager(NetworkManager* manager) {
    if (network_manager_) {
        disconnect(network_manager_, &NetworkManager::responseReceived, this,
                   &ProfileScreen::onNetworkResponse);
    }

    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &ProfileScreen::onNetworkResponse);
    }
}

void ProfileScreen::reloadFromLocal() {
    if (!db_.isOpen()) {
        return;
    }

    LocalUserRepository repo(db_);
    std::optional<LocalUser> user;
    if (sync_coordinator_ && sync_coordinator_->currentUserId() > 0) {
        user = repo.findById(sync_coordinator_->currentUserId());
    } else {
        user = repo.getCurrentUser();
    }
    if (!user) {
        name_label_->setText("Нет данных");
        email_label_->setText("");
        status_label_->setText("");
        setDefaultAvatar();
        return;
    }

    name_label_->setText(user->name_);
    email_label_->setText(user->email_);
    status_label_->setText(user->status_);
}

void ProfileScreen::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (network_manager_) {
        network_manager_->GET(network_manager_->user_info_url_);
    }
}

void ProfileScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (!network_manager_ || endpoint != network_manager_->user_info_url_) {
        return;
    }

    if (code == 200) {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonObject data_obj = doc.object()["data"].toObject();
        const QString avatar_s3_key = data_obj["avatar_s3_key"].toString();
        updateAvatarPreview(avatar_s3_key);
        qDebug() << "ProfileScreen avatar_s3_key:" << avatar_s3_key;
    } else {
        qDebug() << "ProfileScreen: failed to load avatar, code =" << code;
        setDefaultAvatar();
    }
}

void ProfileScreen::updateAvatarPreview(const QString& avatar_s3_key) {
    if (avatar_s3_key.isEmpty() || !network_manager_ || !avatar_network_manager_) {
        setDefaultAvatar();
        return;
    }

    const QUrl avatar_url(network_manager_->avatar_public_base_url_ + avatar_s3_key);
    avatar_network_manager_->get(QNetworkRequest(avatar_url));
}

void ProfileScreen::setDefaultAvatar() {
    avatar_label_->clear();
    avatar_label_->setText("🐶");
    avatar_label_->setStyleSheet("background-color: #f0f2f5; "
                                 "border: 4px solid #305CDE; "
                                 "border-radius: 60px; "
                                 "font-size: 50px;");
}

void ProfileScreen::onAvatarImageDownloaded(QNetworkReply* reply) {
    if (!reply) {
        setDefaultAvatar();
        return;
    }

    const QByteArray image_data = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        setDefaultAvatar();
        reply->deleteLater();
        return;
    }

    QPixmap pixmap;
    if (!pixmap.loadFromData(image_data)) {
        setDefaultAvatar();
        reply->deleteLater();
        return;
    }

    const int size = 120;
    const int border_width = 4;
    const int image_size = size - border_width * 2;

    QPixmap scaled = pixmap.scaled(image_size, image_size, Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);

    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#305CDE"));

    painter.drawEllipse(1, 1, size - 2, size - 2);

    QPainterPath path;
    path.addEllipse(border_width, border_width, image_size, image_size);
    painter.setClipPath(path);

    const int x = border_width + (image_size - scaled.width()) / 2;
    const int y = border_width + (image_size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    painter.end();

    avatar_label_->setStyleSheet("background: transparent; border: none;");
    avatar_label_->setText("");
    avatar_label_->setPixmap(rounded);

    reply->deleteLater();
}

void ProfileScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_button_ = new QPushButton("Chronos", this);
    logo_button_->setCursor(Qt::PointingHandCursor);
    logo_button_->setStyleSheet("QPushButton { "
                                "   background: transparent; "
                                "   border: none; "
                                "   font-weight: bold; "
                                "   color: #305CDE; "
                                "   font-size: 18px; "
                                "   font-family: 'Arial'; "
                                "   padding: 0px; "
                                "}"
                                "QPushButton:hover { "
                                "   color: #2549B3; "
                                "}");
    logout_button_ = new QPushButton("Выход из аккаунта", this);
    logout_button_->setCursor(Qt::PointingHandCursor);
    logout_button_->setStyleSheet(
        "QPushButton { "
        "   background: transparent; color: #C03438; border: none; "
        "   font-size: 16px; font-weight: 400; padding: 0px; "
        "}"
        "QPushButton:hover { color: #e74c3c; text-decoration: underline; }");

    top_bar->addWidget(logo_button_);
    top_bar->addStretch();
    top_bar->addWidget(logout_button_);
    main_layout->addLayout(top_bar);

    main_layout->addSpacing(30);

    avatar_label_ = new QLabel("🐶");
    avatar_label_->setFixedSize(120, 120);
    avatar_label_->setAlignment(Qt::AlignCenter);
    avatar_label_->setStyleSheet("background: transparent; border: none; font-size: 50px;");

    name_label_ = new QLabel("Иван Иванов");
    name_label_->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");

    status_label_ = new QLabel("Developer");
    status_label_->setStyleSheet("color: #305CDE; font-weight: 500; font-size: 14px;");

    email_label_ = new QLabel("example@mail.ru");
    email_label_->setStyleSheet("color: #7f8c8d; font-size: 14px;");

    main_layout->addWidget(avatar_label_, 0, Qt::AlignCenter);
    main_layout->addWidget(name_label_, 0, Qt::AlignCenter);
    main_layout->addWidget(status_label_, 0, Qt::AlignCenter);
    main_layout->addWidget(email_label_, 0, Qt::AlignCenter);

    main_layout->addStretch();

    edit_button_ = new QPushButton("Редактировать профиль");
    edit_button_->setMinimumHeight(45);
    edit_button_->setStyleSheet("QPushButton { "
                                "   background-color: #305CDE; "
                                "   color: white; "
                                "   border-radius: 10px; "
                                "   font-weight: bold; "
                                "   font-size: 15px; "
                                "}"
                                "QPushButton:hover { "
                                "   background-color: #2549B3; "
                                "}");
    main_layout->addWidget(edit_button_);

    connect(logo_button_, &QPushButton::clicked, this, &ProfileScreen::openDashboardScreen);
    connect(logout_button_, &QPushButton::clicked, this, &ProfileScreen::logoutRequested);
    connect(edit_button_, &QPushButton::clicked, this, &ProfileScreen::profileEditRequested);
}
