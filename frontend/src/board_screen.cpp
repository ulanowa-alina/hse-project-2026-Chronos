#include "board_screen.h"

#include "../local_repositories/local_board_repository.hpp"
#include "../local_repositories/local_status_repository.hpp"
#include "../local_repositories/local_task_repository.hpp"

#include <QDebug>
#include <QInputDialog>

BoardScreen::BoardScreen(int board_id, QSqlDatabase db, QWidget* parent)
    : QWidget(parent)
    , board_id_(board_id)
    , db_(db) {
    setupLayout();
}

void BoardScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
}

void BoardScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void BoardScreen::reloadBoardData() {
    if (board_id_ <= 0) {
        return;
    }

    loadFromLocalDatabase();

    if (sync_coordinator_ && network_manager_ && network_manager_->hasToken()) {
        sync_coordinator_->syncAll();
        sync_coordinator_->loadAll();
    }
}

void BoardScreen::loadFromLocalDatabase() {
    clearBoardData();

    LocalBoardRepository board_repo(db_);
    const auto board = board_repo.findById(board_id_);
    if (board) {
        board_name_label_->setText("| " + board->title_);
    } else {
        board_name_label_->setText("| Board " + QString::number(board_id_));
    }

    LocalStatusRepository status_repo(db_);
    const std::vector<LocalStatus> statuses = status_repo.findByBoardId(board_id_);
    for (const LocalStatus& status : statuses) {
        status_names_.insert(status.id_, status.name_);
        ensureStatusWindow(status.id_, status.name_);
    }

    LocalTaskRepository task_repo(db_);
    const std::vector<LocalTask> tasks = task_repo.findByBoardId(board_id_);
    for (const LocalTask& task : tasks) {
        const QString status_name = status_names_.contains(task.status_id_)
                                        ? status_names_.value(task.status_id_)
                                        : "Status " + QString::number(task.status_id_);

        StatusWindow* status = ensureStatusWindow(task.status_id_, status_name);

        auto* card = new TaskCard(task.id_, task.board_id_, task.status_id_, db_, status);
        if (sync_coordinator_) {
            card->setSyncCoordinator(sync_coordinator_);
        }
        card->setData(task.title_, task.description_);
        status->addTaskCard(card);
    }
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

    auto* status = new StatusWindow(status_id, board_id_, name, db_, this);
    if (sync_coordinator_) {
        status->setSyncCoordinator(sync_coordinator_);
    }
    board_layout_->insertWidget(board_layout_->count() - 1, status);
    status_windows_.insert(status_id, status);
    return status;
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
    if (!sync_coordinator_) {
        return;
    }

    bool flag;
    const QString name =
        QInputDialog::getText(this, "New Status", "Column Name:", QLineEdit::Normal, "", &flag);

    if (!flag || name.isEmpty()) {
        return;
    }

    LocalStatusRepository repo(db_);
    const int temp_id = repo.createLocalId();
    LocalStatus status(temp_id, board_id_, name, 0);
    status.sync_status_ = SyncStatus::PENDING;
    status.server_version_ = 0;
    repo.save(status);

    auto* new_status = new StatusWindow(temp_id, board_id_, name, db_, this);
    new_status->setSyncCoordinator(sync_coordinator_);
    board_layout_->insertWidget(board_layout_->count() - 1, new_status);
    status_windows_.insert(temp_id, new_status);

    sync_coordinator_->syncStatuses();
}

void BoardScreen::onProfileRequest() {
    emit openProfileScreen();
}
