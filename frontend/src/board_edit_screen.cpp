#include "board_edit_screen.h"

#include "../local_repositories/local_board_repository.hpp"

#include <QDebug>

BoardEditScreen::BoardEditScreen(int board_id, QWidget* parent)
    : QWidget(parent)
    , board_id_(board_id) {
    setupLayout();
}

void BoardEditScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
}

void BoardEditScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void BoardEditScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void BoardEditScreen::setBoardId(int board_id) {
    board_id_ = board_id;
}

void BoardEditScreen::loadBoardData() {
    if (board_id_ <= 0) {
        return;
    }

    LocalBoardRepository repo(db_);
    const auto board = repo.findById(board_id_);
    if (board) {
        title_input_->setText(board->title_);
        description_input_->setPlainText(board->description_);
    }
}

void BoardEditScreen::onUpdateBoardRequest() {
    if (board_id_ <= 0 || !sync_coordinator_) {
        return;
    }

    QString title = title_input_->text().trimmed();
    QString description = description_input_->toPlainText().trimmed();

    if (title.isEmpty()) {
        qDebug() << "BoardEditScreen: Название доски не может быть пустым";
        return;
    }

    LocalBoardRepository repo(db_);
    const auto existing = repo.findById(board_id_);
    if (!existing) {
        return;
    }

    LocalBoard board = *existing;
    board.title_ = title;
    board.description_ = description;
    board.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(board);
        emit boardUpdated();
        sync_coordinator_->syncBoards();
    } catch (const std::exception& e) {
        qDebug() << "BoardEditScreen: Ошибка сохранения в локальную БД:" << e.what();
    }
}

void BoardEditScreen::onCloseRequest() {
    emit closeRequested();
}

void BoardEditScreen::setupLayout() {
    setStyleSheet("background-color: #F5F5F5; border: none;");

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_label_ = new QLabel("Chronos", this);
    logo_label_->setStyleSheet(
        "font-weight: bold; color: #305CDE; font-size: 18px; font-family: 'Arial'; border: none;");

    close_button_ = new QPushButton("✕", this);
    close_button_->setFixedSize(30, 30);
    close_button_->setStyleSheet("QPushButton { background: transparent; color: #8E8E8E; "
                                 "font-size: 18px; font-weight: bold; border: none; }"
                                 "QPushButton:hover { color: #333333; }");

    top_bar->addWidget(logo_label_);
    top_bar->addStretch();
    top_bar->addWidget(close_button_);
    main_layout->addLayout(top_bar);

    main_layout->addSpacing(20);

    title_label_ = new QLabel("Редактировать доску", this);
    title_label_->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: #305CDE; font-family: 'Arial'; border: none;");
    title_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(title_label_);

    main_layout->addSpacing(20);

    auto create_field = [this](const QString& hint, QWidget* widget) {
        auto* container = new QWidget(this);
        container->setStyleSheet(
            "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
        auto* lay = new QVBoxLayout(container);
        lay->setContentsMargins(15, 8, 15, 8);
        lay->setSpacing(2);

        auto* label = new QLabel(hint, container);
        label->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none; font-weight: bold;");

        lay->addWidget(label);
        lay->addWidget(widget);
        return container;
    };

    title_input_ = new QLineEdit(this);
    title_input_->setPlaceholderText("Название доски");
    title_input_->setStyleSheet(
        "border: none; font-size: 16px; background: transparent; color: #333333;");
    main_layout->addWidget(create_field("Название", title_input_));

    main_layout->addSpacing(10);

    description_input_ = new QTextEdit(this);
    description_input_->setPlaceholderText("Описание доски");
    description_input_->setMaximumHeight(100);
    description_input_->setStyleSheet(
        "border: none; font-size: 14px; background: transparent; color: #333333;");
    main_layout->addWidget(create_field("Описание", description_input_));

    main_layout->addSpacing(20);

    auto* button_layout = new QHBoxLayout();
    button_layout->setSpacing(10);

    cancel_button_ = new QPushButton("Отмена", this);
    cancel_button_->setMinimumHeight(45);
    cancel_button_->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #333333; border-radius: 10px; "
        "font-weight: bold; font-size: 15px; }"
        "QPushButton:hover { background-color: #D0D0D0; }");

    update_button_ = new QPushButton("Сохранить", this);
    update_button_->setMinimumHeight(45);
    update_button_->setStyleSheet(
        "QPushButton { background-color: #305CDE; color: white; border-radius: 10px; "
        "font-weight: bold; font-size: 15px; }"
        "QPushButton:hover { background-color: #2549B3; }");

    button_layout->addWidget(cancel_button_);
    button_layout->addWidget(update_button_);
    main_layout->addLayout(button_layout);

    main_layout->addStretch();

    connect(update_button_, &QPushButton::clicked, this, &BoardEditScreen::onUpdateBoardRequest);
    connect(cancel_button_, &QPushButton::clicked, this, &BoardEditScreen::onCloseRequest);
    connect(close_button_, &QPushButton::clicked, this, &BoardEditScreen::onCloseRequest);
}
