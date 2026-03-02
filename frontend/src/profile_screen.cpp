#include "profile_screen.h"

#include <QDebug>
#include <QShowEvent>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)

ProfileScreen::ProfileScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void ProfileScreen::set_network_manager(NetworkManager* manager) {
    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &ProfileScreen::on_network_response);
    }
}

void ProfileScreen::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    if (network_manager_) {
        qDebug() << "ProfileScreen: Окно открыто, запрашиваю данные аккаунта...";
        network_manager_->GET(network_manager_->info_url_);
    }
}

void ProfileScreen::on_network_response(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->info_url_)
        return;

    if (code == 200) {
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (!doc.isNull()) {
            qDebug() << "data - не JSON";
            QJsonObject root = doc.object();

            QString fullName = root["full_name"].toString();
            QString email = root["email"].toString();
            QString username = root["username"].toString();

            name_label_->setText(fullName);
            email_label_->setText(email);
            qDebug() << "Данные профиля получены";
        }
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

    logo_label_ = new QLabel("Chronos");
    logo_label_->setStyleSheet(
        "font-weight: bold; color: #305CDE; font-size: 18px; font-family: 'Arial';");

    logout_button_ = new QPushButton("Выход из аккаунта");
    logout_button_->setStyleSheet("QPushButton { "
                                  "   background: transparent; "
                                  "   color: #C03438; "
                                  "   border: none; "
                                  "   font-size: 16px; "
                                  "   font-weight: 400; "
                                  "   padding: 0px; "
                                  "   text-align: right; "
                                  "}"
                                  "QPushButton:hover { "
                                  "   color: #e74c3c; "
                                  "   text-decoration: underline; "
                                  "}");

    top_bar->addWidget(logo_label_);
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

    connect(logout_button_, &QPushButton::clicked, this, &ProfileScreen::logoutRequested);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
