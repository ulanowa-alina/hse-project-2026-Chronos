#include "profile_screen.h"

#include <QDebug>
#include <QShowEvent>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)

ProfileScreen::ProfileScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void ProfileScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &ProfileScreen::onNetworkResponse);
    }
}

void ProfileScreen::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    if (network_manager_) {
        qDebug() << "ProfileScreen: Окно открыто, запрашиваю данные аккаунта...";
        network_manager_->GET(network_manager_->user_info_url_);
    }
}

void ProfileScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->user_info_url_)
        return;

    if (code == 200) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();
        QJsonObject data_obj = root["data"].toObject();

        QString name = data_obj["name"].toString();
        QString email = data_obj["email"].toString();
        QString status = data_obj["status"].toString();

        name_label_->setText(name);
        email_label_->setText(email);
        status_label_->setText(status);

        qDebug() << "ProfileScreen: Данные получены";
    } else {
        qDebug() << "Ошибка сервера, код:" << code;
        name_label_->setText("Ошибка загрузки");
    }
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
    avatar_label_->setStyleSheet("background-color: #f0f2f5; "
                                 "border: 4px solid #305CDE; "
                                 "border-radius: 60px; "
                                 "font-size: 50px;");

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

    connect(logo_button_, &QPushButton::clicked, this, &ProfileScreen::boardRequested);
    connect(logout_button_, &QPushButton::clicked, this, &ProfileScreen::logoutRequested);
    connect(edit_button_, &QPushButton::clicked, this, &ProfileScreen::profileEditRequested);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
