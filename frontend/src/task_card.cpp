#include "task_card.h"

#include "validation_utils.h"
#include "../local_repositories/local_task_repository.hpp"

#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QDrag>
#include <QLineF>
#include <QMessageBox>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QVBoxLayout>

namespace {
const int SECONDS_IN_MIN = 60;
const int SECONDS_IN_HOUR = SECONDS_IN_MIN * 60;
const int SECONDS_IN_DAY = 24 * SECONDS_IN_HOUR;

const int SMALL_TIMER_INTERVAL = 1'000;
const int BIG_TIMER_INTERVAL = 60'000;

const int COMPLETE_BTN_SIZE = 22;
const int COMPLETE_BTN_RADIUS = COMPLETE_BTN_SIZE / 2;
const int BLUE_LINE_WIDTH = 3;
const int DRAG_HANDLE_SIZE = 24;

const int HTTP_OK = 200;
const int HTTP_CREATED = 201;
const int HTTP_NO_CONTENT = 204;

bool hasNeutralPriority(const QString& priority_color) {
    return priority_color.isEmpty() || priority_color == "none" || priority_color == "gray";
}
} // namespace

TaskCard::TaskCard(int task_id, int board_id, int status_id, QSqlDatabase db, QWidget* parent)
    : QFrame(parent)
    , task_id_(task_id)
    , board_id_(board_id)
    , status_id_(status_id)
    , db_(db) {
    setupLayout();
    if (task_id_ < 0) {
        title_->setPlaceholderText("Введите название...");
        title_->setFocus();
    }

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &TaskCard::onUpdateTimer);
}

void TaskCard::setNetworkManager(NetworkManager* manager) {
    if (network_manager_) {
        disconnect(network_manager_, &NetworkManager::responseReceived, this,
                   &TaskCard::onNetworkResponse);
    }
    network_manager_ = manager;
    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &TaskCard::onNetworkResponse);
    }
}

void TaskCard::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void TaskCard::setData(const QString& title, const QString& description, const QDateTime& deadline,
                       bool is_completed, const QString& priority_color) {
    title_->setText(title);
    title_->setCursorPosition(0);
    is_completed_ = is_completed;
    deadline_ = deadline;

    if (description.trimmed().isEmpty()) {
        description_text_->setVisible(false);
    } else {
        description_label_->setText(description);
        description_text_->setVisible(true);
    }

    if (hasNeutralPriority(priority_color)) {
        this->setStyleSheet("TaskCard { "
                            "   background-color: white; "
                            "   border: 4px solid transparent; "
                            "   border-radius: 12px; "
                            "}"
                            "TaskCard:hover { "
                            "   border: 4px solid #305CDE; "
                            "}");
    } else {
        QString border_color;
        if (priority_color == "green") {
            border_color = "#A8E4A0";
        } else if (priority_color == "yellow") {
            border_color = "#FCE883";
        } else if (priority_color == "red") {
            border_color = "#E4717A";
        }

        this->setStyleSheet(QString("TaskCard { "
                                    "   background-color: white; "
                                    "   border: 4px solid %1; "
                                    "   border-radius: 12px; "
                                    "}"
                                    "TaskCard:hover { "
                                    "   border: 4px solid #305CDE; "
                                    "}")
                                .arg(border_color));
    }
    doneVisualState();

    if (deadline_.isValid() && !deadline_.isNull() && !is_completed_) {
        deadline_label_->setVisible(true);
        onUpdateTimer();
        timer_->start();
    } else {
        deadline_label_->setVisible(false);
        timer_->stop();
    }
}

void TaskCard::onUpdateTimer() {
    if (!deadline_.isValid() || is_completed_) {
        timer_->stop();
        return;
    }

    QDateTime current = QDateTime::currentDateTime();
    QDateTime display_deadline = deadline_.addSecs(3 * 3600);
    QString date_part = display_deadline.toString("dd.MM в hh:mm");

    if (current >= deadline_) {
        deadline_label_->setText(QString("%1 (Просрочено!)").arg(date_part));
        deadline_label_->setStyleSheet("color: #e74c3c; font-size: 12px;");
        timer_->stop();
        return;
    }

    qint64 secs_to = current.secsTo(deadline_);

    if (secs_to < SECONDS_IN_HOUR) {
        int minutes = secs_to / SECONDS_IN_MIN;
        int seconds = secs_to % SECONDS_IN_MIN;

        deadline_label_->setText(
            QString("%1 (Осталось: %2м %3с)").arg(date_part).arg(minutes).arg(seconds));
        deadline_label_->setStyleSheet("color: #e74c3c; font-size: 12px;");

        if (timer_->interval() != SMALL_TIMER_INTERVAL) {
            timer_->setInterval(SMALL_TIMER_INTERVAL);
        }
        return;
    }

    int days = secs_to / SECONDS_IN_DAY;
    int hours = (secs_to % SECONDS_IN_DAY) / SECONDS_IN_HOUR;

    if (days > 0) {
        deadline_label_->setText(
            QString("%1 (Осталось: %2д %3ч)").arg(date_part).arg(days).arg(hours));
        deadline_label_->setStyleSheet("color: #7f8c8d; font-size: 12px;");
    } else {
        deadline_label_->setText(QString("%1 (Осталось: %2ч)").arg(date_part).arg(hours));
        deadline_label_->setStyleSheet("color: #e67e22; font-size: 12px;");
    }

    if (timer_->interval() != BIG_TIMER_INTERVAL) {
        timer_->setInterval(BIG_TIMER_INTERVAL);
    }
}

void TaskCard::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->tasks_edit_url_ &&
        endpoint != network_manager_->tasks_create_url_ &&
        endpoint != network_manager_->tasks_delete_url_)
        return;

    if (endpoint == network_manager_->tasks_delete_url_) {
        if (should_be_delete_ && (code == HTTP_OK || code == HTTP_NO_CONTENT)) {
            qDebug() << "TaskCard: Задача успешно удалена";
            scheduleDeletion();
        } else if (should_be_delete_) {
            qDebug() << "TaskCard: Ошибка удаления. Код:" << code;
            should_be_delete_ = false;
        }
        return;
    }

    if (code == HTTP_OK || code == HTTP_CREATED) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject taskData = doc.object()["data"].toObject();

        if (task_id_ == -1) {
            task_id_ = taskData["id"].toInt();
            qDebug() << "TaskCard: Задача создана с ID:" << task_id_;
        } else {
            qDebug() << "TaskCard: Данные задачи обновлены";
        }
    } else {
        qDebug() << "TaskCard: Ошибка. Код:" << code;
    }
}

bool TaskCard::eventFilter(QObject* watched, QEvent* event) {
    if (watched == drag_handle_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse_event = static_cast<QMouseEvent*>(event);
            if (mouse_event->button() == Qt::LeftButton) {
                drag_start_position_ = mouse_event->pos();
                return true;
            }
        }

        if (event->type() == QEvent::MouseMove) {
            auto* mouse_event = static_cast<QMouseEvent*>(event);
            if (!(mouse_event->buttons() & Qt::LeftButton)) {
                return false;
            }

            if (QLineF(mouse_event->pos(), drag_start_position_).length() >
                QApplication::startDragDistance()) {
                startDrag();
                return true;
            }
        }
    }

    return QFrame::eventFilter(watched, event);
}

void TaskCard::mousePressEvent(QMouseEvent* event) {
    QFrame::mousePressEvent(event);
}

void TaskCard::mouseMoveEvent(QMouseEvent* event) {
    QFrame::mouseMoveEvent(event);
}

void TaskCard::updateTaskStatus() {
    if (task_id_ < 0 || !sync_coordinator_) {
        return;
    }

    LocalTaskRepository repo(db_);
    const auto existing = repo.findById(task_id_);
    if (!existing) {
        return;
    }

    LocalTask task = *existing;
    task.status_id_ = status_id_;
    task.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(task);
    } catch (const std::exception& e) {
        qDebug() << "TaskCard: failed to update task status:" << e.what();
        return;
    }
    sync_coordinator_->syncTasks();
}

void TaskCard::onTaskSaveRequest() {
    const QString title = title_->text().trimmed();

    if (task_id_ < 0 && title.isEmpty()) {
        LocalTaskRepository repo(db_);
        repo.deleteById(task_id_);
        scheduleDeletion();
        return;
    }

    if (title.isEmpty()) {
        return;
    }

    const QString validation_error = ValidationUtils::validateTaskFields(title, QString());
    if (!validation_error.isEmpty()) {
        QMessageBox::warning(this, "Ошибка изменения задачи", validation_error);
        return;
    }

    if (!sync_coordinator_) {
        return;
    }

    LocalTaskRepository repo(db_);
    if (task_id_ < 0) {
        LocalTask task(task_id_, board_id_, title, status_id_, QStringLiteral("gray"), QString());
        task.sync_status_ = SyncStatus::PENDING;
        task.server_version_ = 0;
        try {
            repo.save(task);
        } catch (const std::exception& e) {
            qDebug() << "TaskCard: failed to save task:" << e.what();
            return;
        }
    } else {
        const auto existing = repo.findById(task_id_);
        if (!existing) {
            return;
        }
        LocalTask task = *existing;
        task.title_ = title;
        task.description_ = QString();
        task.status_id_ = status_id_;
        task.sync_status_ = SyncStatus::PENDING;
        try {
            repo.save(task);
        } catch (const std::exception& e) {
            qDebug() << "TaskCard: failed to save task:" << e.what();
            return;
        }
    }

    sync_coordinator_->syncTasks();
}
void TaskCard::onOpenSettings() {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: white; border: 1px solid #d0d2d6; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 8px 20px; color: #172b4d; }"
        "QMenu::item:selected { background: #f4f5f7; }");

    QAction* edit_action = menu.addAction("📝 Изменить задачу");
    QAction* rename_action = menu.addAction("🏷️ Переименовать");
    QAction* delete_action = menu.addAction("🗑️ Удалить");

    QAction* selected =
        menu.exec(settings_button_->mapToGlobal(QPoint(0, settings_button_->height())));

    if (selected == edit_action) {
        qDebug() << "Вызов глобального окна редактирования для task_id:" << task_id_;
        emit openTaskEditScreen(task_id_, board_id_, status_id_);
    } else if (selected == rename_action) {
        title_->setFocus();
        title_->selectAll();
    } else if (selected == delete_action) {
        onDeleteTaskRequest();
    }
}

void TaskCard::onTitleEditRequest() {
    QString title = title_->text().trimmed();
    if (task_id_ < 0 || !sync_coordinator_) {
        return;
    }

    LocalTaskRepository repo(db_);
    const auto existing = repo.findById(task_id_);
    if (!existing) {
        return;
    }

    auto restoreTitle = [this, &existing]() {
        title_->setText(existing->title_);
        title_->setCursorPosition(0);
    };

    if (title.isEmpty()) {
        restoreTitle();
        return;
    }

    const QString validation_error = ValidationUtils::validateTaskFields(title, QString());
    if (!validation_error.isEmpty()) {
        restoreTitle();
        QMessageBox::warning(this, "Ошибка изменения задачи", validation_error);
        return;
    }

    LocalTask task = *existing;
    task.title_ = title;
    task.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(task);
    } catch (const std::exception& e) {
        qDebug() << "TaskCard: failed to save task:" << e.what();
        return;
    }
    title_->setCursorPosition(0);
    sync_coordinator_->syncTasks();
}

void TaskCard::onMarkDoneRequest() {
    is_completed_ = !is_completed_;
    doneVisualState();

    if (task_id_ < 0 || !sync_coordinator_) {
        return;
    }

    LocalTaskRepository repo(db_);
    const auto existing = repo.findById(task_id_);
    if (!existing) {
        return;
    }

    LocalTask task = *existing;
    task.is_completed_ = is_completed_;
    task.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(task);
    } catch (const std::exception& e) {
        qDebug() << "TaskCard: failed to save task:" << e.what();
        return;
    }
    sync_coordinator_->syncTasks();
}

void TaskCard::doneVisualState() {
    if (is_completed_) {
        title_->setReadOnly(true);
        title_->setStyleSheet("font-weight: bold; font-size: 18px; color: #a0a0a0; border: none; "
                              "background: transparent;");
        description_label_->setStyleSheet(
            "color: #d0d0d0; font-size: 12px; background: transparent;");
        blue_line_->setStyleSheet("background-color: #d0d0d0; border-radius: 1.5px;");
        deadline_label_->setStyleSheet("color: #d0d0d0; font-size: 12px; background: transparent;");

        complete_button_->setText("✓");
        complete_button_->setStyleSheet(
            QString("QPushButton { border: 2px solid #2ecc71; border-radius: %1px; background: "
                    "#2ecc71; color: white; font-weight: bold; font-size: 12px; }")
                .arg(COMPLETE_BTN_RADIUS));

        timer_->stop();
    } else {
        title_->setReadOnly(false);
        title_->setStyleSheet("font-weight: bold; font-size: 18px; border: none; background: "
                              "transparent; color: #305CDE; padding: 0px;");
        description_label_->setStyleSheet(
            "color: #7f8c8d; font-size: 12px; background: transparent;");
        blue_line_->setStyleSheet("background-color: #305CDE; border-radius: 1.5px;");

        complete_button_->setText("");
        complete_button_->setStyleSheet(QString("QPushButton { border: 2px solid #b2b2b2; "
                                                "border-radius: %1px; background: transparent; }"
                                                "QPushButton:hover { border-color: #2ecc71; }")
                                            .arg(COMPLETE_BTN_RADIUS));

        if (deadline_.isValid() && !deadline_.isNull()) {
            onUpdateTimer();
            timer_->start();
        }
    }
    title_->setCursorPosition(0);
}

void TaskCard::onDeleteTaskRequest() {
    if (!sync_coordinator_) {
        scheduleDeletion();
        return;
    }

    if (task_id_ < 0) {
        LocalTaskRepository repo(db_);
        repo.deleteById(task_id_);
        scheduleDeletion();
        return;
    }

    LocalTaskRepository repo(db_);
    repo.markDeletedById(task_id_);
    sync_coordinator_->syncTasks();
    scheduleDeletion();
}

void TaskCard::scheduleDeletion() {
    if (deletion_scheduled_) {
        return;
    }
    deletion_scheduled_ = true;

    if (timer_) {
        timer_->stop();
    }
    if (network_manager_) {
        disconnect(network_manager_, nullptr, this, nullptr);
    }
    if (drag_handle_) {
        drag_handle_->removeEventFilter(this);
    }

    for (QWidget* ancestor = parentWidget(); ancestor; ancestor = ancestor->parentWidget()) {
        removeEventFilter(ancestor);
        const auto child_widgets = findChildren<QWidget*>();
        for (QWidget* child : child_widgets) {
            child->removeEventFilter(ancestor);
        }
    }

    if (QWidget* parent_widget = parentWidget()) {
        if (QLayout* parent_layout = parent_widget->layout()) {
            parent_layout->removeWidget(this);
        }
    }

    hide();
    setParent(nullptr);
    deleteLater();
}

void TaskCard::startDrag() {
    QMimeData* mime = new QMimeData;

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << task_id_ << board_id_ << status_id_;
    mime->setData("application/task", data);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mime);

    QPixmap task_image = grab();
    drag->setPixmap(task_image);
    drag->setHotSpot(rect().center());
    drag->exec(Qt::MoveAction);
}

void TaskCard::setupLayout() {
    this->setObjectName("taskCard");
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setMinimumHeight(100);
    this->setMaximumHeight(180);

    this->setMaximumWidth(260);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    this->setStyleSheet(
        "#taskCard { background: white; border: 1px solid #e0e0e0; border-radius: 12px; }"
        "#taskCard:hover { border: 1px solid #3498db; }");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(4);

    layout->setAlignment(Qt::AlignTop);

    auto* header_layout = new QHBoxLayout();
    header_layout->setContentsMargins(0, 0, 0, 0);
    header_layout->setSpacing(6);

    drag_handle_ = new QLabel(this);
    drag_handle_->setObjectName("dragHandle");
    drag_handle_->setFixedSize(DRAG_HANDLE_SIZE, DRAG_HANDLE_SIZE);
    drag_handle_->setAlignment(Qt::AlignCenter);
    drag_handle_->setCursor(Qt::OpenHandCursor);

    QIcon drag_icon(":/icons/move_icon.svg");

    if (!drag_icon.isNull()) {
        int target_size = DRAG_HANDLE_SIZE - 6;
        QPixmap drag_pixmap = drag_icon.pixmap(QSize(target_size, target_size));
        drag_handle_->setPixmap(drag_pixmap);
    } else {
        drag_handle_->setText("⋮⋮");
        drag_handle_->setStyleSheet(
            "QLabel { color: #8E8E8E; font-size: 16px; font-weight: bold; }");
    }
    drag_handle_->installEventFilter(this);

    title_ = new QLineEdit(this);
    title_->setPlaceholderText("Введите название...");
    title_->setStyleSheet("font-weight: bold; font-size: 18px; border: none; "
                          "background: transparent; color: #305CDE; padding: 0px;");
    title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(title_, &QLineEdit::editingFinished, this, &TaskCard::onTitleEditRequest);

    settings_button_ = new QPushButton("⋮", this);
    settings_button_->setFixedSize(26, 26);
    settings_button_->setStyleSheet(
        "border: none; color: #5e6c84; font-size: 20px; font-weight: bold;");
    header_layout->addWidget(drag_handle_);
    header_layout->addWidget(title_);
    header_layout->addWidget(settings_button_);
    layout->addLayout(header_layout);

    deadline_label_ = new QLabel(this);
    deadline_label_->setStyleSheet(
        "color: #7f8c8d; font-size: 12px; font-weight: 500; background: transparent;");
    layout->addWidget(deadline_label_);
    description_text_ = new QWidget(this);
    auto* desc_layout = new QHBoxLayout(description_text_);

    desc_layout->setContentsMargins(4, 2, 0, 2);
    desc_layout->setSpacing(10);

    blue_line_ = new QWidget(description_text_);
    blue_line_->setFixedWidth(BLUE_LINE_WIDTH);
    blue_line_->setStyleSheet("background-color: #305CDE; border-radius: 1.5px;");
    blue_line_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

    auto* desc_content_widget = new QWidget(description_text_);
    auto* desc_content_layout = new QVBoxLayout(desc_content_widget);
    desc_content_layout->setContentsMargins(0, 0, 0, 0);
    desc_content_layout->setSpacing(2);

    auto* description_title_label_ = new QLabel("Описание:", desc_content_widget);
    description_title_label_->setStyleSheet(
        "color: #7f8c8d; font-size: 12px; font-weight: 500; background: transparent;");

    description_label_ = new QLabel(desc_content_widget);
    description_label_->setWordWrap(true);
    description_label_->setStyleSheet("color: #7f8c8d; font-size: 12px; background: transparent;");
    description_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    desc_content_layout->addWidget(description_title_label_);
    desc_content_layout->addWidget(description_label_);

    desc_layout->addWidget(blue_line_);
    desc_layout->addWidget(desc_content_widget);
    layout->addWidget(description_text_);

    auto* footer_layout = new QHBoxLayout();
    complete_button_ = new QPushButton(this);
    complete_button_->setFixedSize(COMPLETE_BTN_SIZE, COMPLETE_BTN_SIZE);
    complete_button_->setStyleSheet(
        QString("QPushButton { border: 2px solid #b2b2b2; border-radius: %1px; background: "
                "transparent; font-size: 12px; }"
                "QPushButton:hover { border-color: #2ecc71; }")
            .arg(COMPLETE_BTN_RADIUS));

    footer_layout->addStretch();
    footer_layout->addWidget(complete_button_);
    layout->addLayout(footer_layout);

    layout->addStretch();

    connect(settings_button_, &QPushButton::clicked, this, &TaskCard::onOpenSettings);
    connect(complete_button_, &QPushButton::clicked, this, &TaskCard::onMarkDoneRequest);
}
