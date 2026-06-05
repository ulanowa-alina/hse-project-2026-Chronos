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

namespace {
const int SECONDS_IN_MIN = 60;
const int SECONDS_IN_HOUR = SECONDS_IN_MIN * 60;
const int SECONDS_IN_DAY = 24 * SECONDS_IN_HOUR;

const int SMALL_TIMER_INTERVAL = 1'000; // это милисекунды
const int BIG_TIMER_INTERVAL = 60'000;  // это милисекунды

const int COMPLETE_BTN_SIZE = 22;
const int COMPLETE_BTN_RADIUS = COMPLETE_BTN_SIZE / 2;
const int BLUE_LINE_WIDTH = 3;

const int HTTP_OK = 200;
const int HTTP_CREATED = 201;
const int HTTP_NO_CONTENT = 204;
} // namespace

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

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &TaskCard::onUpdateTimer);
}

void TaskCard::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &TaskCard::onNetworkResponse);
    }
}

void TaskCard::setData(const QString& title, const QString& description, const QDateTime& deadline,
                       bool is_completed) {
    title_->setText(title);
    is_completed_ = is_completed;
    deadline_ = deadline;

    if (description.trimmed().isEmpty()) {
        description_text_->setVisible(false);
    } else {
        description_label_->setText(description);
        description_text_->setVisible(true);
    }

    if (deadline_.isValid() && !deadline_.isNull()) {
        deadline_label_->setVisible(true);
        onUpdateTimer();
        timer_->start();
    } else {
        deadline_label_->setVisible(false);
        timer_->stop();
    }

    doneVisualState();
}

void TaskCard::onUpdateTimer() {
    if (!deadline_.isValid() || is_completed_) {
        timer_->stop();
        return;
    }

    QDateTime current = QDateTime::currentDateTime();
    QString date_part = deadline_.toString("dd.MM в hh:mm");

    if (current >= deadline_) {
        deadline_label_->setText(QString("⚠️ %1 (Просрочено!)").arg(date_part));
        deadline_label_->setStyleSheet("color: #e74c3c; font-weight: bold; font-size: 12px;");
        timer_->stop();
        return;
    }

    qint64 secs_to = current.secsTo(deadline_);

    if (secs_to < SECONDS_IN_HOUR) {
        int minutes = secs_to / SECONDS_IN_MIN;
        int seconds = secs_to % SECONDS_IN_MIN;

        deadline_label_->setText(
            QString("⏳ %1 (Осталось: %2м %3с!)").arg(date_part).arg(minutes).arg(seconds));
        deadline_label_->setStyleSheet("color: #e74c3c; font-weight: bold; font-size: 12px;");

        if (timer_->interval() != SMALL_TIMER_INTERVAL) {
            timer_->setInterval(SMALL_TIMER_INTERVAL);
        }
        return;
    }

    int days = secs_to / SECONDS_IN_DAY;
    int hours = (secs_to % SECONDS_IN_DAY) / SECONDS_IN_HOUR;

    if (days > 0) {
        deadline_label_->setText(
            QString("📅 %1 (Осталось: %2д %3ч)").arg(date_part).arg(days).arg(hours));
        deadline_label_->setStyleSheet("color: #7f8c8d; font-size: 12px;");
    } else {
        deadline_label_->setText(QString("📅 %1 (Осталось: %2ч)").arg(date_part).arg(hours));
        deadline_label_->setStyleSheet("color: #e67e22; font-weight: bold; font-size: 12px;");
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
            deleteLater();
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
    if (task_id_ == -1 || network_manager_ == nullptr) {
        return;
    }

    QJsonObject json;
    json["task_id"] = task_id_;
    json["status_id"] = status_id_;

    network_manager_->PATCH(network_manager_->tasks_edit_url_, json);
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
    } else if (selected == rename_action) {
        title_->setFocus();
        title_->selectAll();
    } else if (selected == delete_action) {
        onDeleteTaskRequest();
    }
}

void TaskCard::onTitleEditRequest() {
    QString title = title_->text().trimmed();
    if (title.isEmpty() || task_id_ == -1 || network_manager_ == nullptr) {
        return;
    }

    QJsonObject json;
    json["task_id"] = task_id_;
    json["title"] = title;
    json["status_id"] = status_id_;
    network_manager_->PATCH(network_manager_->tasks_edit_url_, json);
}

void TaskCard::onMarkDoneRequest() {
    is_completed_ = !is_completed_;
    doneVisualState();

    if (network_manager_ == nullptr || task_id_ == -1) {
        return;
    }

    QJsonObject json;
    json["task_id"] = task_id_;
    json["is_completed"] = is_completed_;
    json["status_id"] = status_id_;
    network_manager_->PATCH(network_manager_->tasks_edit_url_, json);
}

void TaskCard::doneVisualState() {
    if (is_completed_) {
        title_->setReadOnly(true);
        title_->setStyleSheet("font-weight: bold; font-size: 18px; color: #a0a0a0; border: none; "
                              "background: transparent; text-decoration: line-through;");
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
}

void TaskCard::onDeleteTaskRequest() {
    if (task_id_ == -1 || network_manager_ == nullptr) {
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
    connect(title_, &QLineEdit::editingFinished, this, &TaskCard::onTitleEditRequest);

    settings_button_ = new QPushButton("⋮", this);
    settings_button_->setFixedSize(26, 26);
    settings_button_->setStyleSheet(
        "border: none; color: #5e6c84; font-size: 20px; font-weight: bold;");
    header_layout->addWidget(title_);
    header_layout->addWidget(settings_button_);
    layout->addLayout(header_layout);

    deadline_label_ = new QLabel(this);
    deadline_label_->setStyleSheet(
        "color: #7f8c8d; font-size: 12px; font-weight: 500; background: transparent;");
    layout->addWidget(deadline_label_);
    description_text_ = new QWidget(this);
    auto* desc_layout = new QHBoxLayout(description_text_);
    desc_layout->setContentsMargins(0, 4, 0, 4);
    desc_layout->setSpacing(10);

    blue_line_ = new QWidget(description_text_);
    blue_line_->setFixedWidth(BLUE_LINE_WIDTH);
    blue_line_->setStyleSheet("background-color: #305CDE; border-radius: 1.5px;");
    blue_line_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    description_label_ = new QLabel(description_text_);
    description_label_->setWordWrap(true);
    description_label_->setStyleSheet("color: #7f8c8d; font-size: 12px; background: transparent;");
    description_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    desc_layout->addWidget(blue_line_);
    desc_layout->addWidget(description_label_);
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
