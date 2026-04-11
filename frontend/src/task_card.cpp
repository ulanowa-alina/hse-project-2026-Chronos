#include "task_card.h"

#include <QDebug>
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
        endpoint != network_manager_->tasks_create_url_)
        return;
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

    title_ = new QLineEdit(this);
    title_->setPlaceholderText("Введите название...");
    title_->setStyleSheet("font-weight: bold; font-size: 18px; border: none; "
                          "background: transparent; color: #305CDE; padding: 0px;");

    title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(title_, &QLineEdit::editingFinished, this, &TaskCard::onTaskSaveRequest);
    layout->addWidget(title_);

    description_edit_ = new QTextEdit(this);
    description_edit_->setPlaceholderText("Добавьте описание...");
    description_edit_->setMaximumHeight(100);
    description_edit_->setStyleSheet(
        "QTextEdit { "
        "   border: none; "                   // Убираем стандартную рамку
        "   border-left: 2px solid #305CDE; " // Та самая синяя полоса слева
        "   margin-top: 8px; "                // ОТСТУП ОТ ЗАГОЛОВКА
        "   padding-left: 10px; "             // Отступ текста от полосы
        "   background: transparent; "
        "   color: #7f8c8d; "
        "   font-size: 12px; "
        "}");

    layout->addWidget(description_edit_);
    layout->addStretch();
}
