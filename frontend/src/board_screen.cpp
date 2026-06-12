#include "board_screen.h"

#include "api_error_utils.h"
#include "validation_utils.h"
#include "../local_repositories/local_board_repository.hpp"
#include "../local_repositories/local_status_repository.hpp"
#include "../local_repositories/local_task_repository.hpp"

#include <QDebug>
#include <QInputDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QUrl>

BoardScreen::BoardScreen(int board_id, QSqlDatabase db, QWidget* parent)
    : QWidget(parent)
    , board_id_(board_id)
    , db_(db)
    , avatar_network_manager_(new QNetworkAccessManager(this)) {
    setupLayout();

    connect(avatar_network_manager_, &QNetworkAccessManager::finished, this,
            &BoardScreen::onAvatarImageDownloaded);
}

void BoardScreen::setNetworkManager(NetworkManager* manager) {
    if (network_manager_) {
        disconnect(network_manager_, &NetworkManager::responseReceived, this,
                   &BoardScreen::onNetworkResponse);
        disconnect(network_manager_, &NetworkManager::syncResponseReceived, this,
                   &BoardScreen::onSyncResponse);
    }

    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &BoardScreen::onNetworkResponse);
        connect(network_manager_, &NetworkManager::syncResponseReceived, this,
                &BoardScreen::onSyncResponse);
    }
}

void BoardScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void BoardScreen::reloadBoardData() {
    if (board_id_ <= 0) {
        return;
    }

    loadFromLocalDatabase();
}

void BoardScreen::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (network_manager_) {
        network_manager_->GET(network_manager_->user_info_url_);
    }
}

void BoardScreen::loadFromLocalDatabase() {
    clearBoardData();

    LocalBoardRepository board_repo(db_);
    const auto board = board_repo.findById(board_id_);
    if (board) {
        board_name_label_->setText("| " + board->title_);
    } else {
        board_name_label_->setText("| Board " + QString::number(board_id_));
    }

    LocalStatusRepository status_repo(db_);
    const std::vector<LocalStatus> statuses = status_repo.findByBoardId(board_id_);
    for (const LocalStatus& status : statuses) {
        status_names_.insert(status.id_, status.name_);
        showStatusWindow(status.id_, status.name_);
    }

    LocalTaskRepository task_repo(db_);
    const std::vector<LocalTask> tasks = task_repo.findByBoardId(board_id_);
    for (const LocalTask& task : tasks) {
        if (!status_names_.contains(task.status_id_)) {
            qDebug() << "BoardScreen: skip task with missing status" << task.id_
                     << "status_id=" << task.status_id_;
            continue;
        }

        const QString status_name = status_names_.value(task.status_id_);
        StatusWindow* status = showStatusWindow(task.status_id_, status_name);

        auto* card = new TaskCard(task.id_, task.board_id_, task.status_id_, db_, status);
        if (sync_coordinator_) {
            card->setSyncCoordinator(sync_coordinator_);
        }
        QDateTime deadline;
        if (!task.deadline_.isEmpty()) {
            deadline = QDateTime::fromString(task.deadline_, Qt::ISODate);
        }
        card->setData(task.title_, task.description_, deadline, task.is_completed_,
                      task.priority_color_);
        connect(card, &TaskCard::openTaskEditScreen, this, &BoardScreen::openTaskEditScreen);
        status->addTaskCard(card);
    }
}

void BoardScreen::clearBoardData() {
    status_windows_.clear();
    status_names_.clear();

    while (board_layout_ && board_layout_->count() > 1) {
        QLayoutItem* item = board_layout_->takeAt(0);
        if (!item) {
            continue;
        }

        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void BoardScreen::removeStatusWindow(int status_id) {
    auto it = status_windows_.find(status_id);
    if (it == status_windows_.end()) {
        return;
    }

    StatusWindow* status = it.value();
    status_windows_.erase(it);
    status_names_.remove(status_id);

    if (!status) {
        return;
    }

    if (board_layout_) {
        board_layout_->removeWidget(status);
    }
    status->clearTasks();
    status->hide();
    status->setParent(nullptr);
    status->deleteLater();
}

StatusWindow* BoardScreen::showStatusWindow(int status_id, const QString& name) {
    if (status_windows_.contains(status_id)) {
        if (StatusWindow* existing = status_windows_.value(status_id)) {
            return existing;
        }
        status_windows_.remove(status_id);
    }

    auto* status = new StatusWindow(status_id, board_id_, name, db_, this);
    if (sync_coordinator_) {
        status->setSyncCoordinator(sync_coordinator_);
    }
    connect(status, &StatusWindow::openTaskCreateScreen, this, &BoardScreen::openTaskCreateScreen);
    connect(status, &StatusWindow::deleteRequested, this, &BoardScreen::onStatusDeleteRequested);
    board_layout_->insertWidget(board_layout_->count() - 1, status);
    status_windows_.insert(status_id, status);
    return status;
}

void BoardScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    auto* header_widget = new QWidget(this);
    header_widget->setFixedHeight(70);
    header_widget->setStyleSheet("background-color: white; border-bottom: 1px solid #dfe1e6;");

    auto* header_layout = new QHBoxLayout(header_widget);
    header_layout->setContentsMargins(20, 0, 20, 0);

    logo_label_ = new QLabel("Chronos", this);
    logo_label_->setStyleSheet("QLabel { font-size: 22px; font-weight: bold; color: #305CDE; }"
                               "QLabel:hover { color: #2549B3; }");
    logo_label_->setCursor(Qt::PointingHandCursor);
    logo_label_->installEventFilter(this);

    board_name_label_ = new QLabel("| Board Name", this);
    board_name_label_->setStyleSheet("font-size: 18px; color: #172b4d;");

    header_layout->addWidget(logo_label_);
    header_layout->addWidget(board_name_label_);
    header_layout->addStretch();

    status_create_button_ = new QPushButton("+ Создать статус", this);
    status_create_button_->setCursor(Qt::PointingHandCursor);
    status_create_button_->setStyleSheet(
        "QPushButton { background: #ebedf0; border: 2px solid transparent; padding: 8px 15px; "
        "border-radius: 5px; "
        "font-weight: bold; }"
        "QPushButton:hover { border-color: #305CDE; background: #d4dbeb; }");
    connect(status_create_button_, &QPushButton::clicked, this,
            &BoardScreen::onStatusCreateRequest);
    header_layout->addWidget(status_create_button_);

    board_settings_button_ = new QPushButton("⚙️", this);
    board_settings_button_->setFixedSize(40, 40);
    board_settings_button_->setStyleSheet(
        "QPushButton { background: #dfe1e6; border: 2px solid transparent; border-radius: 20px; "
        "font-size: 18px; }"
        "QPushButton:hover { border-color: #305CDE; background: #d4dbeb; }");
    connect(board_settings_button_, &QPushButton::clicked, this,
            &BoardScreen::onBoardSettingsRequested);
    header_layout->addWidget(board_settings_button_);

    pomodoro_button_ = new QPushButton("🍅", this);
    pomodoro_button_->setFixedSize(40, 40);
    pomodoro_button_->setStyleSheet(
        "QPushButton { background: #dfe1e6; border: 2px solid transparent; border-radius: 20px; "
        "font-size: 18px; }"
        "QPushButton:hover { border-color: #305CDE; background: #d4dbeb; }");
    connect(pomodoro_button_, &QPushButton::clicked, this, &BoardScreen::onPomodoroRequest);
    header_layout->addWidget(pomodoro_button_);

    profile_button_ = new QPushButton("👤", this);
    profile_button_->setFixedSize(40, 40);
    profile_button_->setStyleSheet(
        "QPushButton { background: #dfe1e6; border: 2px solid transparent; border-radius: 20px; "
        "font-size: 18px; }"
        "QPushButton:hover { border-color: #305CDE; background: #d4dbeb; }");
    connect(profile_button_, &QPushButton::clicked, this, &BoardScreen::onProfileRequest);
    header_layout->addWidget(profile_button_);

    main_layout->addWidget(header_widget);

    scroll_area_ = new QScrollArea(this);
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area_->setStyleSheet(
        "QScrollArea { background-color: #f4f5f7; border: none; }"
        "QScrollBar:horizontal { background: transparent; height: 10px; margin: 0 20px 8px 20px; }"
        "QScrollBar::handle:horizontal { background: #a7b4db; border-radius: 5px; min-width: 40px; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: "
        "transparent; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 4px 0; }"
        "QScrollBar::handle:vertical { background: #a7b4db; border-radius: 4px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; "
        "}");

    auto* scroll_content = new QWidget();
    board_layout_ = new QHBoxLayout(scroll_content);
    board_layout_->setContentsMargins(20, 20, 20, 20);
    board_layout_->setSpacing(20);
    board_layout_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    board_layout_->addStretch(1);

    scroll_area_->setWidget(scroll_content);
    main_layout->addWidget(scroll_area_);
}

void BoardScreen::onStatusCreateRequest() {
    if (!sync_coordinator_) {
        return;
    }

    bool flag;
    const QString name = QInputDialog::getText(this, "Новый статус",
                                               "Название статуса:", QLineEdit::Normal, "", &flag);

    const QString trimmed_name = name.trimmed();
    if (!flag) {
        return;
    }

    const QString validation_error = ValidationUtils::validateStatusName(trimmed_name);
    if (!validation_error.isEmpty()) {
        QMessageBox::warning(this, "Ошибка создания статуса", validation_error);
        return;
    }

    LocalStatusRepository repo(db_);
    const std::vector<LocalStatus> existing_statuses = repo.findByBoardId(board_id_);
    for (const LocalStatus& existing_status : existing_statuses) {
        if (existing_status.name_.trimmed() == trimmed_name) {
            QMessageBox::warning(this, "Ошибка создания статуса",
                                 "Статус с таким названием уже существует.");
            return;
        }
    }

    const int temp_id = repo.createLocalId();
    LocalStatus status(temp_id, board_id_, trimmed_name, 0);
    status.sync_status_ = SyncStatus::PENDING;
    status.server_version_ = 0;
    try {
        repo.save(status);
    } catch (const std::exception& e) {
        qDebug() << "BoardScreen: failed to save status:" << e.what();
        return;
    }

    auto* new_status = new StatusWindow(temp_id, board_id_, trimmed_name, db_, this);
    new_status->setSyncCoordinator(sync_coordinator_);
    connect(new_status, &StatusWindow::openTaskCreateScreen, this,
            &BoardScreen::openTaskCreateScreen);
    connect(new_status, &StatusWindow::deleteRequested, this,
            &BoardScreen::onStatusDeleteRequested);
    board_layout_->insertWidget(board_layout_->count() - 1, new_status);
    status_windows_.insert(temp_id, new_status);
    status_names_.insert(temp_id, trimmed_name);
    showStatusWindow(temp_id, trimmed_name);
    sync_coordinator_->syncStatuses();
}

void BoardScreen::onStatusDeleteRequested(int status_id) {
    removeStatusWindow(status_id);
}

void BoardScreen::onProfileRequest() {
    emit openProfileScreen();
}

void BoardScreen::setDefaultAvatar() {
    profile_button_->setText("👤");
    profile_button_->setIcon(QIcon());
}

void BoardScreen::onAvatarImageDownloaded(QNetworkReply* reply) {
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

    const int size = 40;
    QPixmap scaled =
        pixmap.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);

    const int x = (size - scaled.width()) / 2;
    const int y = (size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    painter.end();

    profile_button_->setText("");
    profile_button_->setIcon(QIcon(rounded));
    profile_button_->setIconSize(QSize(size, size));

    reply->deleteLater();
}

void BoardScreen::loadAvatar(const QString& avatar_s3_key) {
    if (avatar_s3_key.isEmpty() || !network_manager_ || !avatar_network_manager_) {
        setDefaultAvatar();
        return;
    }

    const QUrl avatar_url(network_manager_->avatar_public_base_url_ + avatar_s3_key);
    avatar_network_manager_->get(QNetworkRequest(avatar_url));
}

void BoardScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (!network_manager_ || endpoint != network_manager_->user_info_url_) {
        return;
    }

    if (code == 200) {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QString avatar_s3_key = doc.object()["data"].toObject()["avatar_s3_key"].toString();
        loadAvatar(avatar_s3_key);
    } else {
        setDefaultAvatar();
    }
}

void BoardScreen::onSyncResponse(const QString& endpoint, const QByteArray& data, int code,
                                 const QString& entity, int local_id, const QString& operation) {
    if (!network_manager_ || entity != "status" || operation != "create" ||
        endpoint != network_manager_->statuses_create_url_ || (code >= 200 && code < 300)) {
        return;
    }

    LocalStatusRepository repo(db_);
    const auto status = repo.findById(local_id);
    if (!status || status->board_id_ != board_id_) {
        return;
    }

    if (ApiErrorUtils::isDuplicateFieldError(data, "name") || code == 400 || code == 409) {
        rollbackFailedStatusCreation(local_id);
    }

    QMessageBox::warning(
        this, "Ошибка создания статуса",
        ApiErrorUtils::parseApiErrorMessage(data, QStringLiteral("Не удалось создать статус.")));
}

void BoardScreen::onPomodoroRequest() {
    emit openPomodoroScreen();
}

void BoardScreen::onBoardSettingsRequested() {
    emit openBoardEditScreen(board_id_);
}

bool BoardScreen::eventFilter(QObject* watched, QEvent* event) {
    if (watched == logo_label_ && event->type() == QEvent::MouseButtonPress) {
        emit openDashboardScreen();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void BoardScreen::rollbackFailedStatusCreation(int local_id) {
    try {
        LocalStatusRepository repo(db_);
        repo.deleteById(local_id);
    } catch (const std::exception& e) {
        qDebug() << "BoardScreen: failed to rollback status creation:" << e.what();
    }

    removeStatusWindow(local_id);
}
