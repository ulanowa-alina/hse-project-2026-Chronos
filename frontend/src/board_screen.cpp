#include "board_screen.h"

#include <QDebug>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

BoardScreen::BoardScreen(int board_id, QWidget* parent)
    : QWidget(parent)
    , board_id_(board_id) {
    setupLayout();
}

void BoardScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &BoardScreen::onNetworkResponse);
    }
}

void BoardScreen::reloadBoardData() {
    if (!network_manager_ || board_id_ <= 0) {
        return;
    }

    clearBoardData();
    board_name_label_->setText(QString("| Board %1").arg(board_id_));

    network_manager_->GET(network_manager_->board_get_url_ +
                          "?board_id=" + QString::number(board_id_));
    network_manager_->GET(network_manager_->board_tasks_url_ +
                          "?board_id=" + QString::number(board_id_));
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

StatusWindow* BoardScreen::ensureStatusWindow(int status_id, const QString& name) {
    if (status_windows_.contains(status_id)) {
        return status_windows_.value(status_id);
    }

    auto* status = new StatusWindow(status_id, board_id_, name, this);
    status->setNetworkManager(network_manager_);
    board_layout_->insertWidget(board_layout_->count() - 1, status);
    status_windows_.insert(status_id, status);
    return status;
}

void BoardScreen::loadTasksFromResponse(const QByteArray& data) {
    clearBoardData();

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "BoardScreen: Некорректный JSON в ответе /board/v1/tasks";
        return;
    }

    const QJsonArray tasks = doc.object()["data"].toArray();
    for (const QJsonValue& value : tasks) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject task = value.toObject();
        const int status_id = task["status_id"].toInt(-1);
        const int task_id = task["id"].toInt(-1);
        if (status_id <= 0 || task_id <= 0) {
            continue;
        }

        const QString status_name_from_api = task["status_name"].toString();
        if (!status_name_from_api.isEmpty()) {
            status_names_.insert(status_id, status_name_from_api);
        }

        const QString status_name =
            status_names_.contains(status_id) ? status_names_.value(status_id) : "Status";
        StatusWindow* status = ensureStatusWindow(status_id, status_name);

        auto* card = new TaskCard(task_id, board_id_, status_id, status);
        card->setNetworkManager(network_manager_);
        card->setData(task["title"].toString(), task["description"].toString());
        status->addTaskCard(card);
    }
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
    logo_label_->setStyleSheet("font-size: 20px; font-weight: bold; color: #305CDE;");

    board_name_label_ = new QLabel("| Board Name", this);
    board_name_label_->setStyleSheet("font-size: 18px; color: #172b4d;");

    header_layout->addWidget(logo_label_);
    header_layout->addWidget(board_name_label_);
    header_layout->addStretch();

    status_create_button_ = new QPushButton("+ Add status", this);
    status_create_button_->setCursor(Qt::PointingHandCursor);
    status_create_button_->setStyleSheet(
        "QPushButton { background: #ebedf0; border: none; padding: 8px 15px; border-radius: 5px; "
        "font-weight: bold; }"
        "QPushButton:hover { background: #dadce2; }");
    connect(status_create_button_, &QPushButton::clicked, this,
            &BoardScreen::onStatusCreateRequest);
    header_layout->addWidget(status_create_button_);

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
    if (!network_manager_)
        return;
    bool flag;
    QString name =
        QInputDialog::getText(this, "New Status", "Column Name:", QLineEdit::Normal, "", &flag);

    if (flag && !name.isEmpty()) {
        auto* new_status = new StatusWindow(-1, board_id_, name, this);
        new_status->setNetworkManager(network_manager_);

        board_layout_->insertWidget(board_layout_->count() - 1, new_status);

        QJsonObject json;
        json["status_id"] = -1;
        json["board_id"] = board_id_;
        json["name"] = name;
        json["position"] = 0; // TODO: внедрить эту всю тему
        network_manager_->POST(network_manager_->statuses_create_url_, json);
    }
}

void BoardScreen::onProfileRequest() {
    emit openProfileScreen();
}

void BoardScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->statuses_create_url_ &&
        !endpoint.startsWith(network_manager_->board_get_url_) &&
        !endpoint.startsWith(network_manager_->board_tasks_url_))
        return;

    if (endpoint.startsWith(network_manager_->board_get_url_) ||
        endpoint.startsWith(network_manager_->board_tasks_url_)) {
        if (!isVisible()) {
            return;
        }

        const QUrl url("http://localhost" + endpoint);
        const QUrlQuery query(url);
        bool ok = false;
        const int response_board_id = query.queryItemValue("board_id").toInt(&ok);
        if (!ok || response_board_id != board_id_) {
            return;
        }
    }

    if (endpoint.startsWith(network_manager_->board_get_url_)) {
        if (code == 200) {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            const QString title = doc.object()["data"].toObject()["title"].toString();
            if (!title.isEmpty()) {
                board_name_label_->setText("| " + title);
            } else {
                board_name_label_->setText(QString("| Board %1").arg(board_id_));
            }
        } else {
            qDebug() << "BoardScreen: Ошибка получения доски:" << code;
        }
        return;
    }

    if (endpoint.startsWith(network_manager_->board_tasks_url_)) {
        if (code == 200) {
            loadTasksFromResponse(data);
        } else {
            qDebug() << "BoardScreen: Ошибка получения задач:" << code;
            clearBoardData();
        }
        return;
    }

    if (code < 200 || code >= 300) {
        qDebug() << "BoardScreen: Ошибка сервера" << code << "на" << endpoint;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    QJsonObject data_obj = root["data"].toObject();

    if (endpoint == network_manager_->statuses_create_url_) {
        int new_id = data_obj["id"].toInt();
        const QString created_name = data_obj["name"].toString();
        qDebug() << "BoardScreen: Успешное создание статуса";

        QList<StatusWindow*> windows = this->findChildren<StatusWindow*>();

        for (StatusWindow* window : windows) {
            if (window->getId() == -1) {
                window->setId(new_id);
                status_windows_.insert(new_id, window);
                if (!created_name.isEmpty()) {
                    status_names_.insert(new_id, created_name);
                }
                qDebug() << "BoardScreen: ID статуса обновлен";
                break;
            }
        }
    }
}
