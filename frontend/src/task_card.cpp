#include "task_card.h"

#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QDrag>
#include <QLineF>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QVBoxLayout>

TaskCard::TaskCard(int task_id, int board_id, int status_id, QWidget* parent)
    : QFrame(parent)
    , task_id_(task_id)
    , board_id_(board_id)
    , status_id_(status_id) {
    setupLayout();
    if (task_id_ == -1) {
        title_->setPlaceholderText("Введите название...");
        title_->setFocus();
    }
}

void TaskCard::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &TaskCard::onNetworkResponse);
    }
}

void TaskCard::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->tasks_edit_url_ &&
        endpoint != network_manager_->tasks_create_url_ &&
        endpoint != network_manager_->tasks_delete_url_)
        return;

    if (endpoint == network_manager_->tasks_delete_url_) {
        if (should_be_delete_ && (code == 200 || code == 204)) {
            qDebug() << "TaskCard: Задача успешно удалена";
            deleteLater();
        } else if (should_be_delete_) {
            qDebug() << "TaskCard: Ошибка удаления. Код:" << code;
            should_be_delete_ = false;
        }
        return;
    }

    if (code == 200 || code == 201) {
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

void TaskCard::mousePressEvent(QMouseEvent* event) {
    title_->setFocus();
    QFrame::mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
        drag_start_position_ = event->pos();
    }
}

void TaskCard::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if (QLineF(event->pos(), drag_start_position_).length() > QApplication::startDragDistance()) {
        QMimeData* mime = new QMimeData;

        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << task_id_ << board_id_ << status_id_;
        mime->setData("application/task", data);

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mime);

        QPixmap task_image = grab();
        drag->setPixmap(task_image);
        drag->setHotSpot(event->pos());
        drag->exec(Qt::MoveAction);
    }
}

void TaskCard::updateTaskStatus() {
    if (task_id_ == -1 || !network_manager_)
        return;

    QJsonObject json;
    json["task_id"] = task_id_;
    json["status_id"] = status_id_;

    network_manager_->PATCH(network_manager_->tasks_edit_url_, json);
}

void TaskCard::onTaskSaveRequest() {
    QString title = title_->text().trimmed();

    if (task_id_ == -1 && title.isEmpty()) {
        this->deleteLater();
        return;
    }

    if (title.isEmpty())
        return;

    if (!network_manager_)
        return;

    QJsonObject json;
    if (task_id_ == -1) {
        json["board_id"] = board_id_;
        json["title"] = title;
        json["description"] = description_edit_->toPlainText();
        json["status_id"] = status_id_;
        json["priority_color"] = "gray";
        network_manager_->POST(network_manager_->tasks_create_url_, json);
    } else {
        json["task_id"] = task_id_;
        json["title"] = title;
        json["description"] = description_edit_->toPlainText();
        json["status_id"] = status_id_;
        network_manager_->PATCH(network_manager_->tasks_edit_url_, json);
    }
}

void TaskCard::onOpenSettings() {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: white; border: 1px solid #d0d2d6; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 8px 20px; color: #172b4d; }"
        "QMenu::item:selected { background: #f4f5f7; }");

    QAction* rename_action = menu.addAction("✏️ Переименовать");
    QAction* edit_description_action = menu.addAction("📝 Изменить описание");
    QAction* delete_action = menu.addAction("🗑️ Удалить");

    QAction* selected =
        menu.exec(settings_button_->mapToGlobal(QPoint(0, settings_button_->height())));

    if (selected == rename_action) {
        onTitleEditRequest();
    } else if (selected == edit_description_action) {
        onDescriptionEditRequest();
    } else if (selected == delete_action) {
        onDeleteTaskRequest();
    }
}

void TaskCard::onTitleEditRequest() {
    title_->setFocus();
    title_->selectAll();
}

void TaskCard::onDescriptionEditRequest() {
    description_edit_->setFocus();
    QTextCursor cursor = description_edit_->textCursor();
    cursor.select(QTextCursor::Document);
    description_edit_->setTextCursor(cursor);
}

void TaskCard::onDeleteTaskRequest() {
    if (task_id_ == -1 || !network_manager_) {
        deleteLater();
        return;
    }

    should_be_delete_ = true;

    QJsonObject json;
    json["task_id"] = task_id_;
    network_manager_->DELETE(network_manager_->tasks_delete_url_, json);
}

void TaskCard::setupLayout() {
    this->setObjectName("taskCard");
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setMinimumHeight(100);
    this->setMaximumHeight(130);

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

    title_ = new QLineEdit(this);
    title_->setPlaceholderText("Введите название...");
    title_->setStyleSheet("font-weight: bold; font-size: 18px; border: none; "
                          "background: transparent; color: #305CDE; padding: 0px;");
    title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(title_, &QLineEdit::editingFinished, this, &TaskCard::onTaskSaveRequest);

    settings_button_ = new QPushButton("⋮", this);
    settings_button_->setFixedSize(26, 26);
    settings_button_->setStyleSheet(
        "border: none; color: #5e6c84; font-size: 20px; font-weight: bold;");
    header_layout->addWidget(title_);
    header_layout->addWidget(settings_button_);
    layout->addLayout(header_layout);

    description_edit_ = new QTextEdit(this);
    description_edit_->setPlaceholderText("Добавьте описание...");
    description_edit_->setMaximumHeight(100);
    description_edit_->setStyleSheet("QTextEdit { "
                                     "   border: none; "
                                     "   border-left: 2px solid #305CDE; "
                                     "   margin-top: 8px; "
                                     "   padding-left: 10px; "
                                     "   background: transparent; "
                                     "   color: #7f8c8d; "
                                     "   font-size: 12px; "
                                     "}");

    layout->addWidget(description_edit_);
    layout->addStretch();

    connect(settings_button_, &QPushButton::clicked, this, &TaskCard::onOpenSettings);
}
