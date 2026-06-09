#include "registration_screen.h"

#include "api_error_utils.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {
const QString kInlineErrorStyle =
    QStringLiteral("color: #C03438; font-size: 13px; font-weight: 500; background: transparent;");
}

RegistrationScreen::RegistrationScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void RegistrationScreen::setNetworkManager(NetworkManager* manager) {
    if (network_manager_) {
        disconnect(network_manager_, &NetworkManager::responseReceived, this,
                   &RegistrationScreen::onNetworkResponse);
    }

    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &RegistrationScreen::onNetworkResponse);
    }
}

void RegistrationScreen::clearInputs() {
    if (email_input_) {
        email_input_->clear();
    }
    if (password_input_) {
        password_input_->clear();
    }
    if (name_input_) {
        name_input_->clear();
    }
    if (status_input_) {
        status_input_->clear();
    }
    avatar_file_path_.clear();
    if (avatar_button_) {
        avatar_button_->setText("+");
        avatar_button_->setIcon(QIcon());
    }
    clearErrorMessage();
}

void RegistrationScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void RegistrationScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data,
                                           int code) {
    if (!network_manager_) {
        return;
    }

    if (!isVisible()) {
        return;
    }

    if (endpoint != network_manager_->register_url_ && endpoint != network_manager_->login_url_ &&
        endpoint != network_manager_->user_avatar_upload_url_) {
        return;
    }
    if (endpoint == network_manager_->register_url_) {
        if (code == 200) {
            clearErrorMessage();
            qDebug() << "RegistrationScreen: Успешная регистрация. Входим в аккаунт...";
            QJsonObject login_json;
            login_json["email"] = email_input_->text();
            login_json["password"] = password_input_->text();

            network_manager_->POST(network_manager_->login_url_, login_json);
        } else {
            qDebug() << "RegistrationScreen: Ошибка регистрации. Ответ сервера: " << code;
            showErrorMessage(ApiErrorUtils::parseApiErrorMessage(data));
        }
    } else if (endpoint == network_manager_->login_url_) {
        if (code == 200) {
            clearErrorMessage();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject data_obj = doc.object()["data"].toObject();

            const QString token = data_obj["token"].toString();
            network_manager_->setToken(token);

            if (sync_coordinator_) {
                sync_coordinator_->beginUserSession(data_obj["user"].toObject());
            }

            emit authenticated(token);

            qDebug() << "RegistrationScreen: Зашел в аккаунт и получил токен";

            if (!avatar_file_path_.isEmpty()) {
                QFile file(avatar_file_path_);
                if (!file.open(QIODevice::ReadOnly)) {
                    qDebug() << "RegistrationScreen: Не удалось открыть файл аватара:"
                             << avatar_file_path_;
                    QMessageBox::warning(this, "Ошибка чтения файла",
                                         "Аккаунт создан, но выбранное фото открыть не удалось.");
                    return;
                }

                const QByteArray raw_bytes = file.readAll();
                file.close();

                QFileInfo file_info(avatar_file_path_);
                QMimeDatabase mime_db;
                const QString content_type = mime_db.mimeTypeForFile(avatar_file_path_).name();

                QJsonObject json;
                json["file_name"] = file_info.fileName();
                json["content_type"] =
                    content_type.isEmpty() ? "application/octet-stream" : content_type;
                json["file_base64"] = QString::fromLatin1(raw_bytes.toBase64());
                json["name"] = name_input_->text();
                json["email"] = email_input_->text();
                json["status"] = status_input_->text();

                if (!password_input_->text().isEmpty()) {
                    json["password"] = password_input_->text();
                }

                qDebug() << "RegistrationScreen: Аккаунт создан, отправляю фото профиля...";
                network_manager_->POST(network_manager_->user_avatar_upload_url_, json);
            }
        } else {
            qDebug() << "RegistrationScreen: Ошибка входа после регистрации:" << code;
            showErrorMessage(ApiErrorUtils::parseApiErrorMessage(data));
        }
    } else if (endpoint == network_manager_->user_avatar_upload_url_) {
        if (code == 200) {
            clearErrorMessage();
            qDebug() << "RegistrationScreen: Фото профиля успешно загружено";
        } else {
            qDebug() << "RegistrationScreen: Ошибка загрузки фото после регистрации:" << code;
            showErrorMessage(ApiErrorUtils::parseApiErrorMessage(
                data, QStringLiteral("Аккаунт создан, но фото профиля загрузить не удалось.")));
        }
    }
}

void RegistrationScreen::onRegisterRequest() {
    if (!network_manager_)
        return;

    clearErrorMessage();

    QJsonObject json;
    json["name"] = name_input_->text();
    json["email"] = email_input_->text();
    json["status"] = status_input_->text();
    json["password"] = password_input_->text();

    qDebug() << "RegistrationScreen: Отправляю данные на регистрацию...";
    network_manager_->POST(network_manager_->register_url_, json);
}

void RegistrationScreen::onAvatarPickRequested() {
    clearErrorMessage();
    qDebug() << "RegistrationScreen: onAvatarPickRequested called";
    const QString file_path = QFileDialog::getOpenFileName(this, "Выбрать фото профиля", QString(),
                                                           "Images (*.png *.jpg *.jpeg *.webp)");

    if (file_path.isEmpty()) {
        return;
    }

    avatar_file_path_ = file_path;
    qDebug() << "RegistrationScreen avatar file:" << avatar_file_path_;
    updateAvatarButton(avatar_file_path_);
}

void RegistrationScreen::showErrorMessage(const QString& message) {
    if (!error_label_) {
        return;
    }

    const QString trimmed_message = message.trimmed();
    if (trimmed_message.isEmpty()) {
        clearErrorMessage();
        return;
    }

    error_label_->setText(trimmed_message);
    error_label_->show();
}

void RegistrationScreen::clearErrorMessage() {
    if (!error_label_) {
        return;
    }

    error_label_->clear();
    error_label_->hide();
}

void RegistrationScreen::updateAvatarButton(const QString& file_path) {
    QPixmap pixmap(file_path);

    if (pixmap.isNull()) {
        avatar_button_->setText("!");
        avatar_button_->setIcon(QIcon());
        return;
    }

    const int size = 100;
    const int border_width = 2;
    const int image_size = size - border_width * 2;

    QPixmap scaled = pixmap.scaled(image_size, image_size, Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);

    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addEllipse(border_width, border_width, image_size, image_size);
    painter.setClipPath(path);

    const int x = border_width + (image_size - scaled.width()) / 2;
    const int y = border_width + (image_size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    painter.end();

    avatar_button_->setText("");
    avatar_button_->setIcon(QIcon(rounded));
    avatar_button_->setIconSize(QSize(size, size));
}

void RegistrationScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_label_ = new QLabel("Chronos");
    logo_label_->setStyleSheet(
        "font-weight: bold; color: #305CDE; font-size: 18px; font-family: 'Arial';");

    top_bar->addWidget(logo_label_);
    top_bar->addStretch();
    main_layout->addLayout(top_bar);

    main_layout->addStretch();

    login_title_label_ = new QLabel("Регистрация", this);
    login_title_label_->setStyleSheet(
        "font-size: 32px; font-weight: bold; color: #305CDE; font-family: 'Arial';");
    login_title_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(login_title_label_);

    main_layout->addSpacing(10);
    auto* avatar_layout = new QHBoxLayout();
    avatar_layout->setAlignment(Qt::AlignCenter);

    avatar_button_ = new QPushButton(this);
    int avatar_size = 100;
    avatar_button_->setFixedSize(avatar_size, avatar_size);
    avatar_button_->setCursor(Qt::PointingHandCursor);
    avatar_button_->setIconSize(QSize(100, 100));

    avatar_button_->setStyleSheet("QPushButton {"
                                  "   background-color: #F0F2F5;"
                                  "   border: 2px dashed #305CDE;"
                                  "   border-radius: 50px;"
                                  "   color: #305CDE;"
                                  "   font-size: 30px;"
                                  "   font-weight: bold;"
                                  "}"
                                  "QPushButton:hover {"
                                  "   background-color: #E1E4E8;"
                                  "}");
    avatar_button_->setText("+");

    avatar_layout->addWidget(avatar_button_);
    main_layout->addLayout(avatar_layout);

    main_layout->addSpacing(10);
    auto create_field = [this](const QString& hint, QLineEdit*& input, const QString& placeholder,
                               bool is_password = false) {
        auto* container = new QWidget(this);
        container->setStyleSheet(
            "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
        auto* lay = new QVBoxLayout(container);
        lay->setContentsMargins(15, 8, 15, 8);
        lay->setSpacing(2);

        auto* label = new QLabel(hint, container);
        label->setStyleSheet("color: #8E8E8E; font-size: 11px; border: none; font-weight: bold;");

        input = new QLineEdit(container);
        input->setPlaceholderText(placeholder);
        input->setStyleSheet(
            "border: none; font-size: 16px; background: transparent; color: #333333;");

        if (is_password) {
            input->setEchoMode(QLineEdit::Password);
        }

        lay->addWidget(label);
        lay->addWidget(input);
        return container;
    };

    main_layout->addWidget(create_field("Имя", name_input_, "Введите имя"));
    main_layout->addWidget(create_field("Email", email_input_, "Введите email"));
    main_layout->addWidget(create_field("Статус", status_input_, "Введите свою роль"));
    main_layout->addWidget(create_field("Пароль", password_input_, "Придумайте пароль", true));

    main_layout->addSpacing(15);

    auto* footer_layout = new QHBoxLayout();
    footer_layout->setAlignment(Qt::AlignCenter);
    auto* has_acc_text = new QLabel("Уже есть аккаунт?", this);
    has_acc_text->setStyleSheet("color: black; font-size: 14px;");

    login_button_ = new QPushButton("Войти", this);
    login_button_->setCursor(Qt::PointingHandCursor);
    login_button_->setStyleSheet(
        "QPushButton { color: #E53935; text-decoration: underline; border: none; "
        "background: none; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { color: #C62828; }");
    footer_layout->addWidget(has_acc_text);
    footer_layout->addWidget(login_button_);
    main_layout->addLayout(footer_layout);

    main_layout->addStretch();

    error_label_ = new QLabel(this);
    error_label_->setWordWrap(true);
    error_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    error_label_->setStyleSheet(kInlineErrorStyle);
    error_label_->hide();
    main_layout->addWidget(error_label_);

    registration_button_ = new QPushButton("Создать аккаунт", this);
    registration_button_->setMinimumHeight(50);
    registration_button_->setCursor(Qt::PointingHandCursor);
    registration_button_->setStyleSheet("QPushButton { background-color: #305CDE; color: white; "
                                        "border-radius: 12px; font-weight: bold; font-size: 16px; }"
                                        "QPushButton:hover { background-color: #2549B3; }");
    main_layout->addWidget(registration_button_);

    connect(registration_button_, &QPushButton::clicked, this,
            &RegistrationScreen::onRegisterRequest);
    connect(login_button_, &QPushButton::clicked, this, &RegistrationScreen::loginRequested);
    connect(avatar_button_, &QPushButton::clicked, this,
            &RegistrationScreen::onAvatarPickRequested);
    connect(name_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(email_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(status_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(password_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
}
