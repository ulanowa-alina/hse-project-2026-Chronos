#include "dd_task_card.hpp"

#include "../local_repositories/local_board_repository.hpp"

#include <QDebug>
#include <QPlainTextEdit>

DdTaskCard::DdTaskCard(int task_id, int board_id, int status_id, QSqlDatabase db, QWidget* parent)
    : QFrame(parent)
    , task_id_(task_id)
    , board_id_(board_id)
    , status_id_(status_id)
    , db_(db) {

    setupLayout();
}

void DdTaskCard::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
    if (network_manager_) {
        inner_task_card_->setNetworkManager(network_manager_);
    }
}

void DdTaskCard::setupLayout() {
    this->setObjectName("ddTaskContainer");
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet("#ddTaskContainer { "
                        "   background: #F4F7FE; "
                        "   border: 1px solid #DCE4F9; "
                        "   border-radius: 16px; "
                        "}");

    this->setFixedWidth(350);
    this->setFixedHeight(180);

    main_layout_ = new QVBoxLayout(this);
    main_layout_->setContentsMargins(20, 12, 20, 14);
    main_layout_->setSpacing(8);

    board_button_ = new QPushButton("Загрузка доски...", this);
    board_button_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    board_button_->setStyleSheet("QPushButton { "
                                 "   color: #305CDE; "
                                 "   font-size: 17px; "
                                 "   font-weight: bold; "
                                 "   border: none; "
                                 "   background: transparent; "
                                 "   text-align: left; "
                                 "   padding: 0px 0px 0px 5px; "
                                 "}"
                                 "QPushButton:hover { "
                                 "   text-decoration: underline; "
                                 "   color: #1A3BB0; "
                                 "}");
    connect(board_button_, &QPushButton::clicked, this, &DdTaskCard::onBoardButtonClicked);
    main_layout_->addWidget(board_button_);

    inner_task_card_ = new TaskCard(task_id_, board_id_, status_id_, db_, this);
    freezeTaskCard();

    main_layout_->addWidget(inner_task_card_);

    loadBoardTitle();
}

void DdTaskCard::loadBoardTitle() {
    if (!db_.isOpen()) {
        board_button_->setText("Доска #" + QString::number(board_id_));
        return;
    }

    try {
        LocalBoardRepository board_repo(db_);
        const auto board = board_repo.findById(board_id_);
        if (board && !board->title_.trimmed().isEmpty()) {
            board_button_->setText(board->title_.trimmed());
        } else {
            board_button_->setText("Доска #" + QString::number(board_id_));
        }
    } catch (const std::exception& e) {
        qDebug() << "DdTaskCard: failed to load board title:" << e.what();
        board_button_->setText("Доска #" + QString::number(board_id_));
    }
}

void DdTaskCard::freezeTaskCard() {
    inner_task_card_->setAcceptDrops(false);
    inner_task_card_->setAttribute(Qt::WA_StyledBackground, true);

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
}

void DdTaskCard::setCardData(const QString& title, const QString& description,
                             const QDateTime& deadline, bool is_completed) {
    if (inner_task_card_ != nullptr) {
        inner_task_card_->setData(title, description, deadline, is_completed);
        if (auto* title_edit = inner_task_card_->findChild<QLineEdit*>()) {
            title_edit->setReadOnly(true);
            title_edit->setFocusPolicy(Qt::NoFocus);
        }

        auto* desc_edit = inner_task_card_->findChild<QTextEdit*>();
        auto* desc_plain_edit = inner_task_card_->findChild<QPlainTextEdit*>();

        QTextEdit* target_desc = desc_edit ? (QTextEdit*) desc_edit : (QTextEdit*) desc_plain_edit;

        if (target_desc) {
            target_desc->setReadOnly(true);
            target_desc->setFocusPolicy(Qt::NoFocus);
        }

        inner_task_card_->setFixedWidth(300);
        inner_task_card_->setStyleSheet("TaskCard { "
                                        "   background-color: white !important; "
                                        "   background: white !important; "
                                        "   border: none !important; "
                                        "   border-radius: 12px; "
                                        "   padding: 0px 12px 12px 12px; "
                                        "}"
                                        "TaskCard QFrame, TaskCard QWidget { "
                                        "   background: transparent !important; "
                                        "   background-color: transparent !important; "
                                        "   border: none !important; "
                                        "}"
                                        "TaskCard QLineEdit { "
                                        "   font-weight: bold; font-weight: 700; font-size: 18px; "
                                        "   color: #172B4D !important; "
                                        "   background: transparent !important; "
                                        "   border: none !important; "
                                        "}"
                                        "TaskCard QTextEdit, TaskCard QPlainTextEdit { "
                                        "   color: #5E6C84 !important; "
                                        "   background: transparent !important; "
                                        "   border: none !important; "
                                        "   padding: 0px; "
                                        "}"
                                        "TaskCard QLabel { "
                                        "   background: transparent !important; "
                                        "   border: none !important; "
                                        "}");
    }
}

void DdTaskCard::onBoardButtonClicked() {
    emit openBoardRequested(board_id_);
}
