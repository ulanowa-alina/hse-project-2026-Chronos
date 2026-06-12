#include "profile_edit_screen.h"

#include "api_error_utils.h"
#include "validation_utils.h"
#include "../local_repositories/local_user_repository.hpp"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {
const QString kInlineErrorStyle =
    QStringLiteral("color: #C03438; font-size: 13px; font-weight: 500; background: transparent;");
const QString kLocalSaveErrorMessage =
    QStringLiteral("Не удалось сохранить изменения. Попробуйте еще раз.");
} // namespace

ProfileEditScreen::ProfileEditScreen(QWidget* parent)
    : QWidget(parent)
    , avatar_network_manager_(new QNetworkAccessManager(this)) {
    setupLayout();

    connect(avatar_network_manager_, &QNetworkAccessManager::finished, this,
            &ProfileEditScreen::onAvatarImageDownloaded);
}

void ProfileEditScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void ProfileEditScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void ProfileEditScreen::setNetworkManager(NetworkManager* manager) {
    if (network_manager_) {
        disconnect(network_manager_, &NetworkManager::responseReceived, this,
                   &ProfileEditScreen::onNetworkResponse);
    }

    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &ProfileEditScreen::onNetworkResponse);
    }
}

void ProfileEditScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data,
                                          int code) {
    if (!network_manager_)
        return;

    if (endpoint == network_manager_->user_info_url_) {
        if (code == 200) {
            clearErrorMessage();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject data_obj = doc.object()["data"].toObject();
            QString avatar_s3_key = data_obj["avatar_s3_key"].toString();
            current_avatar_s3_key_ = avatar_s3_key;
            original_name_ = data_obj["name"].toString();
            original_email_ = data_obj["email"].toString();
            original_status_ = data_obj["status"].toString();
            avatar_delete_requested_ = false;
            resetAvatarSelection();
            qDebug() << "ProfileEditScreen avatar_s3_key:" << avatar_s3_key;

            name_input_->setText(original_name_);
            email_input_->setText(original_email_);
            status_input_->setText(original_status_);
            loadRemoteAvatar(avatar_s3_key);
        }
        return;
    }

    if (endpoint == network_manager_->user_avatar_delete_url_) {
        if (code == 200) {
            current_avatar_s3_key_.clear();
            original_name_ = name_input_->text();
            original_email_ = email_input_->text();
            original_status_ = status_input_->text();
            avatar_delete_requested_ = false;
            resetAvatarSelection();
            qDebug() << "ProfileEditScreen: Фото профиля удалено";

            password_input_->clear();
            emit profileRequested();
        } else {
            qDebug() << "ProfileEditScreen: Ошибка удаления аватара. Код:" << code
                     << "Данные:" << data;
            showErrorMessage(ApiErrorUtils::parseApiErrorMessage(
                data, QStringLiteral("Не удалось удалить фото профиля.")));
        }
        return;
    }

    if (endpoint == network_manager_->user_avatar_upload_url_) {
        if (code == 200) {
            clearErrorMessage();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject data_obj = doc.object()["data"].toObject();

            current_avatar_s3_key_ = data_obj["avatar_s3_key"].toString();
            original_name_ = name_input_->text();
            original_email_ = email_input_->text();
            original_status_ = status_input_->text();
            avatar_delete_requested_ = false;
            resetAvatarSelection();
            qDebug() << "ProfileEditScreen uploaded avatar_s3_key:" << current_avatar_s3_key_;

            password_input_->clear();
            emit profileRequested();
        } else {
            qDebug() << "ProfileEditScreen: Ошибка загрузки аватара. Код:" << code
                     << "Данные:" << data;
            showErrorMessage(ApiErrorUtils::parseApiErrorMessage(
                data, QStringLiteral("Не удалось загрузить фото профиля.")));
        }
        return;
    }

    if (endpoint == network_manager_->user_edit_info_url_) {
        if (code == 200) {
            clearErrorMessage();
            qDebug() << "ProfileEditScreen: Изменения успешно сохранены";
            password_input_->clear();
            original_name_ = name_input_->text();
            original_email_ = email_input_->text();
            original_status_ = status_input_->text();
            resetAvatarSelection();

            emit profileRequested();
        } else {
            qDebug() << "ProfileEditScreen: Ошибка сохранения! Код:" << code << "Данные:" << data;
            showErrorMessage(ApiErrorUtils::parseApiErrorMessage(
                data, QStringLiteral("Не удалось сохранить изменения профиля.")));
        }
    }
}
void ProfileEditScreen::reloadFromLocal() {
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
        return;
    }

    name_input_->setText(user->name_);
    email_input_->setText(user->email_);
    status_input_->setText(user->status_);
}

void ProfileEditScreen::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (network_manager_) {
        network_manager_->GET(network_manager_->user_info_url_);
    }
}

void ProfileEditScreen::onProfileEditRequest() {
    if (!sync_coordinator_ || !db_.isOpen()) {
        return;
    }

    clearErrorMessage();

    const QString validation_error = ValidationUtils::validateUserFields(
        name_input_->text(), email_input_->text(), status_input_->text(), password_input_->text(),
        false);
    if (!validation_error.isEmpty()) {
        showErrorMessage(validation_error);
        return;
    }

    if (!selected_avatar_file_path_.isEmpty()) {
        sendAvatarUpload();
        return;
    }

    if (avatar_delete_requested_) {
        sendAvatarDelete();
        return;
    }

    if (!hasPendingTextChanges()) {
        qDebug() << "ProfileEditScreen: Нет изменений для сохранения";
        emit profileRequested();
        return;
    }

    sendProfileUpdate();
}

void ProfileEditScreen::sendProfileUpdate() {
    LocalUserRepository repo(db_);
    std::optional<LocalUser> user;
    if (sync_coordinator_ && sync_coordinator_->currentUserId() > 0) {
        user = repo.findById(sync_coordinator_->currentUserId());
    } else {
        user = repo.getCurrentUser();
    }
    if (!user) {
        return;
    }

    LocalUser updated = *user;
    updated.name_ = name_input_->text().trimmed();
    updated.email_ = email_input_->text().trimmed();
    updated.status_ = status_input_->text().trimmed();
    updated.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(updated);
    } catch (const std::exception& e) {
        qDebug() << "ProfileEditScreen: failed to save user:" << e.what();
        showErrorMessage(kLocalSaveErrorMessage);
        return;
    }

    pending_password_ = password_input_->text();
    password_input_->clear();

    if (!pending_password_.isEmpty()) {
        sync_coordinator_->setPassword(pending_password_);
        pending_password_.clear();
    }
    sync_coordinator_->syncUsers();
    emit profileRequested();
}

void ProfileEditScreen::sendAvatarDelete() {
    QJsonObject json;
    json["name"] = name_input_->text();
    json["email"] = email_input_->text();
    json["status"] = status_input_->text();

    if (!password_input_->text().isEmpty()) {
        json["password"] = password_input_->text();
    }

    qDebug() << "ProfileEditScreen: Отправляю запрос на удаление фото...";
    network_manager_->DELETE(network_manager_->user_avatar_delete_url_, json);
}

void ProfileEditScreen::sendAvatarUpload() {
    if (selected_avatar_file_path_.isEmpty()) {
        return;
    }

    QFile file(selected_avatar_file_path_);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "ProfileEditScreen: Не удалось открыть файл аватара:"
                 << selected_avatar_file_path_;
        QMessageBox::warning(this, "Ошибка чтения файла",
                             "Не удалось открыть выбранный файл изображения.");
        return;
    }

    const QByteArray raw_bytes = file.readAll();
    file.close();

    QFileInfo file_info(selected_avatar_file_path_);
    QMimeDatabase mime_db;
    const QString content_type = mime_db.mimeTypeForFile(selected_avatar_file_path_).name();

    QJsonObject json;
    json["file_name"] = file_info.fileName();
    json["content_type"] = content_type.isEmpty() ? "application/octet-stream" : content_type;
    json["file_base64"] = QString::fromLatin1(raw_bytes.toBase64());
    json["name"] = name_input_->text();
    json["email"] = email_input_->text();
    json["status"] = status_input_->text();

    if (!password_input_->text().isEmpty()) {
        json["password"] = password_input_->text();
    }

    qDebug() << "ProfileEditScreen: Отправляю аватар на сервер...";
    network_manager_->POST(network_manager_->user_avatar_upload_url_, json);
}

void ProfileEditScreen::onAvatarPickRequested() {
    clearErrorMessage();
    qDebug() << "ProfileEditScreen: onAvatarPickRequested called";

    QFileDialog::Options options = QFileDialog::Options();

#if defined(Q_OS_MAC)
    options |= QFileDialog::DontUseNativeDialog;
#endif

    const QString file_path =
        QFileDialog::getOpenFileName(this, "Выбрать фото профиля", QString(),
                                     "Images (*.png *.jpg *.jpeg *.webp)", nullptr, options);

    if (file_path.isEmpty()) {
        return;
    }

    avatar_delete_requested_ = false;
    selected_avatar_file_path_ = file_path;
    updateAvatarButtonPreview(selected_avatar_file_path_);
    qDebug() << "ProfileEditScreen selected avatar file:" << selected_avatar_file_path_;
}
void ProfileEditScreen::onAvatarDeleteRequested() {
    clearErrorMessage();
    if (avatar_delete_requested_) {
        avatar_delete_requested_ = false;
        updateAvatarDeleteButtonState();
        loadRemoteAvatar(current_avatar_s3_key_);
        return;
    }

    if (!selected_avatar_file_path_.isEmpty()) {
        resetAvatarSelection();
        return;
    }

    if (current_avatar_s3_key_.isEmpty()) {
        return;
    }

    const auto answer = QMessageBox::question(
        this, "Удалить фото", "Фото профиля будет удалено после сохранения изменений. Продолжить?");
    if (answer != QMessageBox::Yes) {
        return;
    }

    avatar_delete_requested_ = true;
    avatar_button_->setText("Выбрать фото");
    avatar_button_->setIcon(QIcon());
    updateAvatarDeleteButtonState();
}

void ProfileEditScreen::loadRemoteAvatar(const QString& avatar_s3_key) {
    if (avatar_s3_key.isEmpty() || !network_manager_ || !avatar_network_manager_) {
        return;
    }

    const QUrl avatar_url(network_manager_->avatar_public_base_url_ + avatar_s3_key);
    qDebug() << "ProfileEditScreen: loading avatar from" << avatar_url.toString();
    avatar_network_manager_->get(QNetworkRequest(avatar_url));
}

void ProfileEditScreen::updateAvatarButtonPreview(const QString& file_path) {
    QPixmap pixmap(file_path);

    if (pixmap.isNull()) {
        avatar_button_->setText("Ошибка фото");
        avatar_button_->setIcon(QIcon());
        updateAvatarDeleteButtonState();
        return;
    }

    const int size = 140;
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

    updateAvatarDeleteButtonState();
}

void ProfileEditScreen::onAvatarImageDownloaded(QNetworkReply* reply) {
    if (!reply) {
        return;
    }

    const QByteArray image_data = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "ProfileEditScreen: avatar download error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QPixmap pixmap;
    if (!pixmap.loadFromData(image_data)) {
        qDebug() << "ProfileEditScreen: failed to load avatar from data, bytes ="
                 << image_data.size();
        reply->deleteLater();
        return;
    }

    const int size = 140;
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

    reply->deleteLater();
}

auto ProfileEditScreen::hasPendingTextChanges() const -> bool {
    return name_input_->text() != original_name_ || email_input_->text() != original_email_ ||
           status_input_->text() != original_status_ || !password_input_->text().isEmpty();
}

void ProfileEditScreen::resetAvatarSelection() {
    selected_avatar_file_path_.clear();
    avatar_button_->setText("Выбрать фото");
    avatar_button_->setIcon(QIcon());
    updateAvatarDeleteButtonState();
}

void ProfileEditScreen::updateAvatarDeleteButtonState() {
    if (!delete_avatar_button_) {
        return;
    }

    if (avatar_delete_requested_) {
        delete_avatar_button_->setEnabled(true);
        delete_avatar_button_->setText("Отменить удаление");
        return;
    }

    if (!selected_avatar_file_path_.isEmpty()) {
        delete_avatar_button_->setEnabled(true);
        delete_avatar_button_->setText("Убрать выбор");
        return;
    }

    delete_avatar_button_->setEnabled(!current_avatar_s3_key_.isEmpty());
    delete_avatar_button_->setText("Удалить фото");
}

void ProfileEditScreen::showErrorMessage(const QString& message) {
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

void ProfileEditScreen::clearErrorMessage() {
    if (!error_label_) {
        return;
    }

    error_label_->clear();
    error_label_->hide();
}

void ProfileEditScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_label_ = new QLabel("Chronos");
    logo_label_->setStyleSheet(
        "font-weight: bold; color: #305CDE; font-size: 18px; font-family: 'Arial';");

    cancel_button_ = new QPushButton("Отмена", this);
    cancel_button_->setCursor(Qt::PointingHandCursor);
    cancel_button_->setStyleSheet(
        "QPushButton { "
        "   background: transparent; color: #C03438; border: none; "
        "   font-size: 16px; font-weight: 400; padding: 0px; "
        "}"
        "QPushButton:hover { color: #e74c3c; text-decoration: underline; }");

    top_bar->addWidget(logo_label_);
    top_bar->addStretch();
    top_bar->addWidget(cancel_button_);
    main_layout->addLayout(top_bar);

    auto* avatar_layout = new QHBoxLayout();
    avatar_layout->setAlignment(Qt::AlignCenter);

    avatar_button_ = new QPushButton("Выбрать фото", this);
    avatar_button_->setIconSize(QSize(136, 136));
    avatar_button_->setFixedSize(140, 140);
    avatar_button_->setCursor(Qt::PointingHandCursor);
    avatar_button_->setStyleSheet("QPushButton {"
                                  "   background-color: #F0F2F5;"
                                  "   border: 2px dashed #305CDE;"
                                  "   border-radius: 70px;"
                                  "   color: #305CDE;"
                                  "   font-size: 14px;"
                                  "   font-weight: bold;"
                                  "}"
                                  "QPushButton:hover {"
                                  "   background-color: #E1E4E8;"
                                  "}");

    avatar_layout->addWidget(avatar_button_);
    main_layout->addLayout(avatar_layout);

    auto* avatar_actions_layout = new QHBoxLayout();
    avatar_actions_layout->setAlignment(Qt::AlignCenter);

    delete_avatar_button_ = new QPushButton("Удалить фото", this);
    delete_avatar_button_->setCursor(Qt::PointingHandCursor);
    delete_avatar_button_->setEnabled(false);
    delete_avatar_button_->setStyleSheet(
        "QPushButton { "
        "   background: transparent; color: #C03438; border: none; "
        "   font-size: 14px; font-weight: 500; padding: 0px; "
        "}"
        "QPushButton:hover { color: #e74c3c; text-decoration: underline; }"
        "QPushButton:disabled { color: #B9BCC4; text-decoration: none; }");

    avatar_actions_layout->addWidget(delete_avatar_button_);
    main_layout->addLayout(avatar_actions_layout);
    main_layout->addSpacing(10);

    main_layout->addStretch();

    auto* name_container = new QWidget(this);
    name_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* name_lay = new QVBoxLayout(name_container);
    name_lay->setContentsMargins(15, 8, 15, 8);
    name_lay->setSpacing(2);

    auto* name_hint = new QLabel("Name", name_container);
    name_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    name_input_ = new QLineEdit(name_container);

    name_input_->setPlaceholderText("Enter new name");
    name_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    name_lay->addWidget(name_hint);
    name_lay->addWidget(name_input_);
    main_layout->addWidget(name_container);

    main_layout->addSpacing(10);

    auto* email_container = new QWidget(this);
    email_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* email_lay = new QVBoxLayout(email_container);
    email_lay->setContentsMargins(15, 8, 15, 8);
    email_lay->setSpacing(2);

    auto* email_hint = new QLabel("Email", email_container);
    email_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    email_input_ = new QLineEdit(email_container);

    email_input_->setPlaceholderText("Enter email");
    email_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    email_lay->addWidget(email_hint);
    email_lay->addWidget(email_input_);
    main_layout->addWidget(email_container);

    main_layout->addSpacing(10);

    auto* status_container = new QWidget(this);
    status_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* status_lay = new QVBoxLayout(status_container);
    status_lay->setContentsMargins(15, 8, 15, 8);
    status_lay->setSpacing(2);

    auto* status_hint = new QLabel("Status", status_container);
    status_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    status_input_ = new QLineEdit(status_container);

    status_input_->setPlaceholderText("Enter status");
    status_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    status_lay->addWidget(status_hint);
    status_lay->addWidget(status_input_);
    main_layout->addWidget(status_container);

    main_layout->addSpacing(10);

    auto* pass_container = new QWidget(this);
    pass_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* pass_lay = new QVBoxLayout(pass_container);
    pass_lay->setContentsMargins(15, 8, 15, 8);
    pass_lay->setSpacing(2);

    auto* pass_hint = new QLabel("Password", pass_container);
    pass_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    password_input_ = new QLineEdit(pass_container);
    password_input_->setEchoMode(QLineEdit::Password);
    password_input_->setPlaceholderText("Enter password");
    password_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    pass_lay->addWidget(pass_hint);
    pass_lay->addWidget(password_input_);
    main_layout->addWidget(pass_container);

    main_layout->addSpacing(15);

    main_layout->addStretch();
    main_layout->addStretch();

    error_label_ = new QLabel(this);
    error_label_->setWordWrap(true);
    error_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    error_label_->setStyleSheet(kInlineErrorStyle);
    error_label_->hide();
    main_layout->addWidget(error_label_);

    save_button_ = new QPushButton("Сохранить изменения");
    save_button_->setMinimumHeight(45);
    save_button_->setStyleSheet("QPushButton { "
                                "   background-color: #305CDE; "
                                "   color: white; "
                                "   border-radius: 10px; "
                                "   font-weight: bold; "
                                "   font-size: 15px; "
                                "}"
                                "QPushButton:hover { "
                                "   background-color: #2549B3; "
                                "}");
    main_layout->addWidget(save_button_);

    connect(cancel_button_, &QPushButton::clicked, this, &ProfileEditScreen::profileRequested);
    connect(save_button_, &QPushButton::clicked, this, &ProfileEditScreen::onProfileEditRequest);
    connect(avatar_button_, &QPushButton::clicked, this, &ProfileEditScreen::onAvatarPickRequested);
    connect(delete_avatar_button_, &QPushButton::clicked, this,
            &ProfileEditScreen::onAvatarDeleteRequested);
    connect(name_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(email_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(status_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(password_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });

    updateAvatarDeleteButtonState();
}
