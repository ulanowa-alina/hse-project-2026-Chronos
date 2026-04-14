#include "status_window.h"

#include <QDataStream>
#include <QDebug>
#include <QEvent>
#include <QMenu>
#include <QMimeData>

StatusWindow::StatusWindow(int status_id, int board_id, const QString& name, QWidget* parent)
    : QFrame(parent)
    , status_id_(status_id)
    , board_id_(board_id) {
    setupLayout(name);
    setAcceptDrops(true);
}

void StatusWindow::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &StatusWindow::onNetworkResponse);
    }
}

void StatusWindow::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->statuses_edit_url_ &&
        endpoint != network_manager_->statuses_delete_url_)
        return;

    if (endpoint == network_manager_->statuses_delete_url_) {
        if (should_be_delete_ && code == 204) {
            qDebug() << "StatusWindow: Статус успешно удален";
            deleteLater();
        } else if (should_be_delete_) {
            qDebug() << "StatusWindow: Ошибка удаления. Ответ сервера:" << code;
            should_be_delete_ = false;
        }
        return;
    }

    if (code == 200) {
        qDebug() << "StatusWindow: Успешное обновление статуса";
    } else {
        qDebug() << "StatusWindow: Ошибка обновления. Ответ сервера: " << code;
    }
}

void StatusWindow::onCreateTaskRequest() {
    if (!network_manager_)
        return;

    auto* card = new TaskCard(-1, board_id_, status_id_, this);
    card->setNetworkManager(network_manager_);

    tasks_layout_->insertWidget(0, card);
    updateGeometry();
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

void StatusWindow::onStatusDeleteRequest() {
    if (status_id_ == -1 || !network_manager_) {
        deleteLater();
        return;
    }

    should_be_delete_ = true;

    QJsonObject json;
    json["status_id"] = status_id_;
    network_manager_->DELETE(network_manager_->statuses_delete_url_, json);
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

void StatusWindow::removeTaskCard(TaskCard* card) {
    tasks_layout_->removeWidget(card);
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

    connect(status_name_, &QLineEdit::editingFinished, this, [this]() {});
    connect(create_task_button_, &QPushButton::clicked, this, &StatusWindow::onCreateTaskRequest);
    connect(settings_button_, &QPushButton::clicked, this, &StatusWindow::onOpenSettings);
}
