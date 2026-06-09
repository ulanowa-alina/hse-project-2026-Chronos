#include "status_window.h"

#include "../local_repositories/local_status_repository.hpp"
#include "../local_repositories/local_task_repository.hpp"

#include <QDataStream>
#include <QDebug>
#include <QEvent>
#include <QMenu>
#include <QMimeData>

StatusWindow::StatusWindow(int status_id, int board_id, const QString& name, QSqlDatabase db,
                           QWidget* parent)
    : QFrame(parent)
    , status_id_(status_id)
    , board_id_(board_id)
    , db_(db) {
    setupLayout(name);
    setAcceptDrops(true);
}

void StatusWindow::setNetworkManager(NetworkManager* manager) {
    if (network_manager_) {
        disconnect(network_manager_, &NetworkManager::responseReceived, this,
                   &StatusWindow::onNetworkResponse);
    }
    network_manager_ = manager;
    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &StatusWindow::onNetworkResponse);
    }
}

void StatusWindow::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void StatusWindow::onCreateTaskRequest() {
    emit openTaskCreateScreen(board_id_, status_id_);
}

void StatusWindow::onOpenSettings() {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: white; border: 1px solid #d0d2d6; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 8px 20px; color: #172b4d; }"
        "QMenu::item:selected { background: #f4f5f7; }");

    QAction* rename_action = menu.addAction("✏️ Переименовать");
    QAction* delete_action = menu.addAction("🗑️ Удалить");

    QAction* selected =
        menu.exec(settings_button_->mapToGlobal(QPoint(0, settings_button_->height())));

    if (selected == rename_action) {
        onStatusEditRequest();
    } else if (selected == delete_action) {
        onStatusDeleteRequest();
    }
}

void StatusWindow::onStatusEditRequest() {
    status_name_->setReadOnly(false);
    status_name_->setStyleSheet(
        "QLineEdit { "
        "  font-weight: bold; font-size: 16px; color: #172b4d; "
        "  border: 1px solid #3498db; background: white; border-radius: 4px; padding: 2px; "
        "}");
    status_name_->setFocus();
    status_name_->selectAll();
}

void StatusWindow::onStatusNameSaved() {
    if (!sync_coordinator_) {
        return;
    }

    LocalStatusRepository repo(db_);
    const auto existing = repo.findById(status_id_);
    if (!existing) {
        return;
    }

    const QString name = status_name_->text().trimmed();
    if (name.isEmpty()) {
        status_name_->setText(existing->name_);
        status_name_->setReadOnly(true);
        status_name_->setStyleSheet(
            "QLineEdit { font-weight: bold; font-size: 16px; color: #172b4d; "
            "border: none; background: transparent; }");
        return;
    }

    LocalStatus status = *existing;
    status.name_ = name;
    status.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(status);
    } catch (const std::exception& e) {
        qDebug() << "StatusWindow: failed to save status:" << e.what();
        status_name_->setText(existing->name_);
        status_name_->setReadOnly(true);
        status_name_->setStyleSheet(
            "QLineEdit { font-weight: bold; font-size: 16px; color: #172b4d; "
            "border: none; background: transparent; }");
        return;
    }

    status_name_->setReadOnly(true);
    status_name_->setStyleSheet("QLineEdit { font-weight: bold; font-size: 16px; color: #172b4d; "
                                "border: none; background: transparent; }");

    if (sync_coordinator_) {
        sync_coordinator_->syncStatuses();
    }
}

void StatusWindow::onStatusDeleteRequest() {
    if (!sync_coordinator_) {
        deleteLater();
        return;
    }

    LocalStatusRepository repo(db_);
    repo.markDeletedById(status_id_);
    sync_coordinator_->syncStatuses();
    deleteLater();
}

void StatusWindow::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    Q_UNUSED(endpoint);
    Q_UNUSED(data);
    Q_UNUSED(code);
}

void StatusWindow::dragEnterEvent(QDragEnterEvent* event) {
    auto mime = event->mimeData();
    if (mime->hasFormat("application/task")) {
        should_be_highlighted_ = true;
        processHighlight();
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void StatusWindow::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/task")) {
        if (!should_be_highlighted_) {
            should_be_highlighted_ = true;
            processHighlight();
        }
        event->acceptProposedAction();
    } else {
        if (should_be_highlighted_) {
            should_be_highlighted_ = false;
            processHighlight();
        }
        event->ignore();
    }
}

void StatusWindow::dropEvent(QDropEvent* event) {
    auto mime = event->mimeData();
    if (!mime->hasFormat("application/task")) {
        should_be_highlighted_ = false;
        processHighlight();
        event->ignore();
        return;
    }

    QByteArray data = mime->data("application/task");
    QDataStream stream(&data, QIODevice::ReadOnly);

    int task_id = -1;
    int board_id = -1;
    int old_status_id = -1;
    stream >> task_id >> board_id >> old_status_id;

    QList<StatusWindow*> statuses = window()->findChildren<StatusWindow*>();
    StatusWindow* old_status_window = nullptr;

    for (StatusWindow* status : statuses) {
        if (status->getId() == old_status_id) {
            old_status_window = status;
            break;
        }
    }

    if (!old_status_window) {
        should_be_highlighted_ = false;
        processHighlight();
        event->ignore();
        return;
    }

    if (old_status_window == this) {
        should_be_highlighted_ = false;
        processHighlight();
        event->acceptProposedAction();
        return;
    }

    QList<TaskCard*> cards = old_status_window->findChildren<TaskCard*>();
    TaskCard* dragged_card = nullptr;

    for (TaskCard* card : cards) {
        if (card->getTaskId() == task_id) {
            dragged_card = card;
            break;
        }
    }

    if (!dragged_card) {
        should_be_highlighted_ = false;
        processHighlight();
        event->ignore();
        return;
    }

    old_status_window->removeTaskCard(dragged_card);
    insertTaskCard(dragged_card);

    dragged_card->setStatusId(status_id_);
    dragged_card->updateTaskStatus();

    should_be_highlighted_ = false;
    processHighlight();
    event->acceptProposedAction();
}

void StatusWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    should_be_highlighted_ = false;
    processHighlight();
    event->accept();
}

bool StatusWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::DragEnter) {
        dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        return event->isAccepted();
    }

    if (event->type() == QEvent::DragMove) {
        dragMoveEvent(static_cast<QDragMoveEvent*>(event));
        return event->isAccepted();
    }

    if (event->type() == QEvent::Drop) {
        dropEvent(static_cast<QDropEvent*>(event));
        return event->isAccepted();
    }

    if (event->type() == QEvent::DragLeave) {
        dragLeaveEvent(static_cast<QDragLeaveEvent*>(event));
        return event->isAccepted();
    }

    return QFrame::eventFilter(watched, event);
}

void StatusWindow::insertTaskCard(TaskCard* card) {
    tasks_layout_->insertWidget(0, card);
}

void StatusWindow::addTaskCard(TaskCard* card) {
    insertTaskCard(card);
}

void StatusWindow::removeTaskCard(TaskCard* card) {
    tasks_layout_->removeWidget(card);
}

void StatusWindow::clearTasks() {
    while (tasks_layout_->count() > 1) {
        QLayoutItem* item = tasks_layout_->takeAt(0);
        if (!item) {
            continue;
        }

        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void StatusWindow::processHighlight() {
    if (should_be_highlighted_) {
        setStyleSheet("#statusWindow { background-color: #D6DFFB; border-radius: 12px; border: 2px "
                      "solid #3498db; }");
    } else {
        setStyleSheet(
            "#statusWindow { background-color: #D6DFFB; border-radius: 12px; border: none; }");
    }
}

void StatusWindow::setupLayout(const QString& name) {
    this->setMinimumHeight(150);
    this->setFixedWidth(280);
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    this->setObjectName("statusWindow");
    processHighlight();

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(8);

    auto* header_layout = new QHBoxLayout();
    status_name_ = new QLineEdit(name, this);
    status_name_->setReadOnly(true);
    status_name_->setStyleSheet("QLineEdit { font-weight: bold; font-size: 16px; color: #172b4d; "
                                "border: none; background: transparent; }");
    header_layout->addWidget(status_name_);

    settings_button_ = new QPushButton("⋮", this);
    settings_button_->setFixedSize(26, 26);
    settings_button_->setStyleSheet(
        "border: none; color: #5e6c84; font-size: 20px; font-weight: bold;");
    header_layout->addWidget(settings_button_);
    main_layout->addLayout(header_layout);

    create_task_button_ = new QPushButton("+ Добавить задачу", this);
    create_task_button_->setStyleSheet(
        "QPushButton { text-align: left; padding: 8px; border: none; border-radius: 6px; "
        "background: transparent; color: #5e6c84; }");
    main_layout->addWidget(create_task_button_);

    tasks_scroll_area_ = new QScrollArea(this);
    tasks_scroll_area_->setWidgetResizable(true);
    tasks_scroll_area_->setFrameShape(QFrame::NoFrame);
    tasks_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tasks_scroll_area_->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; border: none; }"
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 4px 0; }"
        "QScrollBar::handle:vertical { background: #a7b4db; border-radius: 4px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; "
        "}");
    tasks_scroll_area_->viewport()->setStyleSheet("background: transparent; border: none;");
    tasks_scroll_area_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    tasks_container_ = new QWidget(this);
    tasks_container_->setStyleSheet("background: transparent; border: none;");
    tasks_container_->setAcceptDrops(true);
    tasks_container_->installEventFilter(this);

    tasks_layout_ = new QVBoxLayout(tasks_container_);
    tasks_layout_->setContentsMargins(0, 0, 4, 0);
    tasks_layout_->setSpacing(10);
    tasks_layout_->addStretch();

    tasks_scroll_area_->setAcceptDrops(true);
    tasks_scroll_area_->viewport()->setAcceptDrops(true);
    tasks_scroll_area_->installEventFilter(this);
    tasks_scroll_area_->viewport()->installEventFilter(this);
    status_name_->installEventFilter(this);
    settings_button_->installEventFilter(this);
    create_task_button_->installEventFilter(this);

    tasks_scroll_area_->setWidget(tasks_container_);
    main_layout->addWidget(tasks_scroll_area_, 1);

    connect(status_name_, &QLineEdit::editingFinished, this, &StatusWindow::onStatusNameSaved);
    connect(create_task_button_, &QPushButton::clicked, this, &StatusWindow::onCreateTaskRequest);
    connect(settings_button_, &QPushButton::clicked, this, &StatusWindow::onOpenSettings);
}
