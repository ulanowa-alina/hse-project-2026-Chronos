#include "dd_task_card.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
const int HTTP_OK = 200;
const int HTTP_CREATED = 201;
const int HTTP_NO_CONTENT = 204;
} // namespace

DdTaskCard::DdTaskCard(int task_id, int board_id, int status_id, QWidget* parent)
    : QFrame(parent)
    , task_id_(task_id)
    , board_id_(board_id)
    , status_id_(status_id) {

    setupLayout();
}

void DdTaskCard::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &DdTaskCard::onNetworkResponse);

        inner_task_card_->setNetworkManager(network_manager_);

        QString request_endpoint =
            network_manager_->board_get_url_ + QString("?board_id=%1").arg(board_id_);
        network_manager_->GET(request_endpoint);
    }
}

void DdTaskCard::setupLayout() {
    this->setObjectName("ddTaskContainer");
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet("#ddTaskContainer { "
                        "   background: #F4F7FE; "
                        "   border: 1px solid #DCE4F9; "
                        "   border-radius: 16px; "
                        "   padding: 10px; "
                        "}");

    main_layout_ = new QVBoxLayout(this);
    main_layout_->setContentsMargins(12, 12, 12, 12);
    main_layout_->setSpacing(8);

    board_button_ = new QPushButton("Загрузка доски...", this);
    board_button_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    board_button_->setStyleSheet("QPushButton { "
                                 "   color: #305CDE; "
                                 "   font-size: 13px; "
                                 "   font-weight: bold; "
                                 "   border: none; "
                                 "   background: transparent; "
                                 "   text-align: left; "
                                 "   padding: 0px; "
                                 "}"
                                 "QPushButton:hover { "
                                 "   text-decoration: underline; "
                                 "   color: #1A3BB0; "
                                 "}");

    connect(board_button_, &QPushButton::clicked, this, &DdTaskCard::onBoardButtonClicked);
    main_layout_->addWidget(board_button_);

    inner_task_card_ = new TaskCard(task_id_, board_id_, status_id_, this);
    freezeTaskCard();

    main_layout_->addWidget(inner_task_card_);
}

void DdTaskCard::freezeTaskCard() {
    inner_task_card_->setAcceptDrops(false);

    auto* settings_btn = inner_task_card_->findChild<QPushButton*>("settings_button_");
    if (!settings_btn) {
        const auto buttons = inner_task_card_->findChildren<QPushButton*>();
        for (auto* btn : buttons) {
            btn->setVisible(false);
        }
    } else {
        settings_btn->setVisible(false);
        if (auto* comp_btn = inner_task_card_->findChild<QPushButton*>("complete_button_")) {
            comp_btn->setVisible(false);
        }
    }

    auto* title_edit = inner_task_card_->findChild<QLineEdit*>();
    if (title_edit) {
        title_edit->setReadOnly(true);
        title_edit->setFocusPolicy(Qt::NoFocus);
        title_edit->setStyleSheet(
            "font-weight: bold; font-weight: 700; font-size: 18px; border: none; "
            "background: transparent; color: #172B4D; padding: 0px; selection-background-color: "
            "transparent;");
    }
}

void DdTaskCard::setCardData(const QString& title, const QString& description,
                             const QDateTime& deadline, bool is_completed) {
    if (inner_task_card_ != nullptr) {
        inner_task_card_->setData(title, description, deadline, is_completed);
    }
}

void DdTaskCard::onBoardButtonClicked() {
    emit openBoardRequested(board_id_);
}

void DdTaskCard::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {

    QString target_endpoint =
        network_manager_->board_get_url_ + QString("?board_id=%1").arg(board_id_);
    if (endpoint != target_endpoint)
        return;

    if (code == HTTP_OK) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject response_obj = doc.object();

        QJsonObject board_data = response_obj["data"].toObject();
        QString board_name = board_data["title"].toString();

        if (!board_name.isEmpty()) {
            board_button_->setText(board_name);
        } else {
            board_button_->setText("Доска #" + QString::number(board_id_));
            qDebug() << "DdTaskCard: Доска нашлась, но она невалидна, без имени";
        }
    } else {
        qDebug() << "DdTaskCard: Доска не нашлась. Код:" << code;
        board_button_->setText("Доска #" + QString::number(board_id_));
    }
}
