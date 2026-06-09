#include "dashboard_screen.hpp"

#include "../local_repositories/local_board_repository.hpp"
#include "../local_repositories/local_pomodoro_session_repository.hpp"
#include "../local_repositories/local_task_repository.hpp"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>

DashboardScreen::DashboardScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void DashboardScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
}

void DashboardScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void DashboardScreen::reloadDashboardData() {
    loadStatistics();
    loadBoards();
    loadDeadlines();
}

void DashboardScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    setupHeader();

    auto* bottom_container = new QWidget();
    auto* bottom_layout = new QHBoxLayout(bottom_container);
    bottom_layout->setContentsMargins(0, 0, 0, 0);
    bottom_layout->setSpacing(0);

    setupSidebar();

    auto* content_widget = new QWidget();
    content_widget->setStyleSheet("background: #f4f5f7;");

    auto* content_layout = new QVBoxLayout(content_widget);
    content_layout->setContentsMargins(20, 0, 20, 0);
    content_layout->setSpacing(0);

    setupStatsFrame();
    setupDeadlinesFrame();
    setupBoardsFrame();

    stats_frame_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    deadlines_frame_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    boards_frame_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* spacer1 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto* spacer2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto* spacer3 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto* spacer4 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

    content_layout->addItem(spacer1);
    content_layout->addWidget(stats_frame_);
    content_layout->addItem(spacer2);
    content_layout->addWidget(deadlines_frame_);
    content_layout->addItem(spacer3);
    content_layout->addWidget(boards_frame_);
    content_layout->addItem(spacer4);

    bottom_layout->addWidget(sidebar_widget_);
    bottom_layout->addWidget(content_widget, 1);

    main_layout->addWidget(header_widget_);
    main_layout->addWidget(bottom_container, 1);
}

void DashboardScreen::setupSidebar() {
    sidebar_widget_ = new QFrame();
    sidebar_widget_->setFixedWidth(200);

    sidebar_widget_->setStyleSheet("QFrame { "
                                   "   background: white; "
                                   "   border-right: 1px solid #dfe1e6; "
                                   "}");

    auto* sidebar_layout = new QVBoxLayout(sidebar_widget_);
    sidebar_layout->setContentsMargins(15, 20, 15, 20);
    sidebar_layout->setSpacing(10);

    QString sidebar_btn_style = "QPushButton { "
                                "   background: #F4F7FE; "
                                "   color: #172b4d; "
                                "   border: 1px solid #DCE4F9; "
                                "   padding: 8px; "
                                "   border-radius: 6px; "
                                "   font-size: 15px; "
                                "}"
                                "QPushButton:hover { "
                                "   background: #F4F7FE; "
                                "   border-color: #305CDE; "
                                "}"
                                "QPushButton:pressed { "
                                "   background: #F4F7FE; "
                                "   border-color: #1A3BB0; "
                                "}";

    QString active_btn_style = "QPushButton { "
                               "   background: #305CDE; "
                               "   color: white; "
                               "   border: 1px solid #305CDE; "
                               "   padding: 8px; "
                               "   border-radius: 6px; "
                               "   font-size: 15px; "
                               "   font-weight: bold; "
                               "}"
                               "QPushButton:hover { "
                               "   background: #305CDE; "
                               "   border-color: #305CDE; "
                               "}"
                               "QPushButton:pressed { "
                               "   background: #1A3BB0; "
                               "   border-color: #1A3BB0; "
                               "}";

    dashboard_button_ = new QPushButton("Dashboard");
    dashboard_button_->setCursor(Qt::PointingHandCursor);
    dashboard_button_->setStyleSheet(active_btn_style);
    dashboard_button_->setFixedHeight(40);
    sidebar_layout->addWidget(dashboard_button_);

    add_board_button_ = new QPushButton("Создать доску");
    add_board_button_->setCursor(Qt::PointingHandCursor);
    add_board_button_->setStyleSheet(sidebar_btn_style);
    add_board_button_->setFixedHeight(40);
    connect(add_board_button_, &QPushButton::clicked, this, &DashboardScreen::onBoardCreateRequest);
    sidebar_layout->addWidget(add_board_button_);

    profile_button_ = new QPushButton("Профиль");
    profile_button_->setCursor(Qt::PointingHandCursor);
    profile_button_->setStyleSheet(sidebar_btn_style);
    profile_button_->setFixedHeight(40);
    connect(profile_button_, &QPushButton::clicked, this, &DashboardScreen::onProfileRequest);
    sidebar_layout->addWidget(profile_button_);

    pomodoro_button_ = new QPushButton("Режим Pomodoro");
    pomodoro_button_->setCursor(Qt::PointingHandCursor);
    pomodoro_button_->setStyleSheet(sidebar_btn_style);
    pomodoro_button_->setFixedHeight(40);
    connect(pomodoro_button_, &QPushButton::clicked, this, &DashboardScreen::onPomodoroRequest);
    sidebar_layout->addWidget(pomodoro_button_);

    sidebar_layout->addStretch();

    logout_button_ = new QPushButton("Выйти");
    logout_button_->setCursor(Qt::PointingHandCursor);
    logout_button_->setStyleSheet(
        "QPushButton { background: #ffbdad; color: #de350b; border: none; padding: 8px; "
        "border-radius: 5px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: #ff8f73; }");
    logout_button_->setFixedHeight(40);
    connect(logout_button_, &QPushButton::clicked, this, &DashboardScreen::onLogoutRequest);
    sidebar_layout->addWidget(logout_button_);
}

void DashboardScreen::setupHeader() {
    header_widget_ = new QWidget();
    header_widget_->setFixedHeight(60);
    header_widget_->setStyleSheet("background: white; border-bottom: 1px solid #dfe1e6;");

    auto* header_layout = new QHBoxLayout(header_widget_);
    header_layout->setContentsMargins(14, 0, 14, 0);
    header_layout->setSpacing(88);

    auto* logo_label = new QLabel("Chronos");
    logo_label->setStyleSheet("font-size: 22px; "
                              "font-weight: bold; "
                              "color: #305CDE; "
                              "background: transparent;");

    header_layout->addWidget(logo_label);
    header_layout->addStretch();
}

void DashboardScreen::setupStatsFrame() {
    stats_frame_ = new QFrame();
    stats_frame_->setStyleSheet("background: transparent;");
    auto* stats_layout = new QHBoxLayout(stats_frame_);
    stats_layout->setContentsMargins(0, 0, 0, 0);
    stats_layout->setSpacing(20);

    QString stat_card_style = "QFrame { "
                              "   background: white; "
                              "   border: 1px solid #dfe1e6; "
                              "   border-radius: 8px; "
                              "}";

    QString stat_value_style = "font-size: 28px; font-weight: bold; color: #305CDE; border: none; "
                               "background: transparent;";
    QString stat_label_style =
        "font-size: 14px; color: #5e6c84; border: none; background: transparent;";

    active_tasks_frame_ = new QFrame();
    active_tasks_frame_->setStyleSheet(stat_card_style);
    auto* active_tasks_layout = new QVBoxLayout(active_tasks_frame_);
    active_tasks_layout->setContentsMargins(20, 15, 20, 15);
    active_tasks_layout->setSpacing(5);

    active_tasks_label_ = new QLabel("Всего актуальных задач:");
    active_tasks_label_->setStyleSheet(stat_label_style);
    active_tasks_value_ = new QLabel("0");
    active_tasks_value_->setStyleSheet(stat_value_style);

    active_tasks_layout->addWidget(active_tasks_label_);
    active_tasks_layout->addWidget(active_tasks_value_);

    focus_hours_frame_ = new QFrame();
    focus_hours_frame_->setStyleSheet(stat_card_style);
    auto* focus_hours_layout = new QVBoxLayout(focus_hours_frame_);
    focus_hours_layout->setContentsMargins(20, 15, 20, 15);
    focus_hours_layout->setSpacing(5);

    focus_hours_label_ = new QLabel("Время в фокусировании:");
    focus_hours_label_->setStyleSheet(stat_label_style);
    focus_hours_value_ = new QLabel("0");
    focus_hours_value_->setStyleSheet(stat_value_style);

    focus_hours_layout->addWidget(focus_hours_label_);
    focus_hours_layout->addWidget(focus_hours_value_);

    completed_tasks_frame_ = new QFrame();
    completed_tasks_frame_->setStyleSheet(stat_card_style);
    auto* completed_tasks_layout = new QVBoxLayout(completed_tasks_frame_);
    completed_tasks_layout->setContentsMargins(20, 15, 20, 15);
    completed_tasks_layout->setSpacing(5);

    completed_tasks_label_ = new QLabel("Всего выполнено задач:");
    completed_tasks_label_->setStyleSheet(stat_label_style);
    completed_tasks_value_ = new QLabel("0");
    completed_tasks_value_->setStyleSheet(stat_value_style);

    completed_tasks_layout->addWidget(completed_tasks_label_);
    completed_tasks_layout->addWidget(completed_tasks_value_);

    stats_layout->addWidget(active_tasks_frame_);
    stats_layout->addWidget(completed_tasks_frame_);
    stats_layout->addWidget(focus_hours_frame_);
}
void DashboardScreen::setupDeadlinesFrame() {
    deadlines_frame_ = new QFrame();
    deadlines_frame_->setStyleSheet("QFrame#DeadlinesFrame { "
                                    "   background: white; "
                                    "   border: 1px solid #dfe1e6; "
                                    "   border-radius: 8px; "
                                    "}");
    deadlines_frame_->setObjectName("DeadlinesFrame");

    auto* deadlines_layout = new QVBoxLayout(deadlines_frame_);
    deadlines_layout->setContentsMargins(20, 15, 20, 15);
    deadlines_layout->setSpacing(10);

    deadlines_title_label_ = new QLabel("Ближайшие дедлайны");
    deadlines_title_label_->setStyleSheet("font-size: 18px; font-weight: bold; color: #172b4d; "
                                          "border: none; background: transparent;");
    deadlines_layout->addWidget(deadlines_title_label_);

    auto* deadlines_scroll_area = new QScrollArea();
    deadlines_scroll_area->setWidgetResizable(true);
    deadlines_scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    deadlines_scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    deadlines_scroll_area->setFrameShape(QFrame::NoFrame);

    deadlines_scroll_area->setFixedHeight(200);

    deadlines_scroll_area->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:horizontal { background: transparent; height: 12px; margin: 0px 4px 4px 4px; }"
        "QScrollBar::handle:horizontal { background: #a7b4db; border-radius: 4px; min-width: 40px; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: "
        "transparent; }");

    deadlines_container_ = new QWidget();
    deadlines_container_->setStyleSheet("background: transparent; border: none;");

    auto* deadlines_container_layout = new QHBoxLayout(deadlines_container_);
    deadlines_container_layout->setContentsMargins(0, 5, 0, 15);
    deadlines_container_layout->setSpacing(25);

    no_deadlines_label_ = new QLabel("У вас нет горящих задач");
    no_deadlines_label_->setStyleSheet("font-size: 14px; color: #5e6c84; font-style: italic; "
                                       "border: none; background: transparent;");
    no_deadlines_label_->setAlignment(Qt::AlignCenter);
    no_deadlines_label_->hide();

    deadlines_container_layout->addWidget(no_deadlines_label_);
    deadlines_scroll_area->setWidget(deadlines_container_);
    deadlines_layout->addWidget(deadlines_scroll_area);
}

void DashboardScreen::setupBoardsFrame() {
    boards_frame_ = new QFrame();
    boards_frame_->setStyleSheet("QFrame#BoardsFrame { "
                                 "   background: white; "
                                 "   border: 1px solid #dfe1e6; "
                                 "   border-radius: 8px; "
                                 "}");
    boards_frame_->setObjectName("BoardsFrame");

    auto* boards_layout = new QVBoxLayout(boards_frame_);
    boards_layout->setContentsMargins(20, 15, 20, 15);
    boards_layout->setSpacing(10);

    boards_title_label_ = new QLabel("Доски");
    boards_title_label_->setStyleSheet("font-size: 18px; font-weight: bold; color: #172b4d; "
                                       "border: none; background: transparent;");
    boards_layout->addWidget(boards_title_label_);

    boards_scroll_area_ = new QScrollArea();
    boards_scroll_area_->setWidgetResizable(true);
    boards_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    boards_scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    boards_scroll_area_->setFrameShape(QFrame::NoFrame);
    boards_scroll_area_->setFixedHeight(235);
    boards_scroll_area_->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:horizontal { background: transparent; height: 12px; margin: 0px 4px 4px 4px; }"
        "QScrollBar::handle:horizontal { background: #a7b4db; border-radius: 4px; min-width: 40px; "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: "
        "transparent; }");

    boards_container_ = new QWidget();
    boards_container_->setStyleSheet("background: transparent; border: none;");
    auto* boards_container_layout = new QHBoxLayout(boards_container_);
    boards_container_layout->setContentsMargins(0, 5, 0, 10);
    boards_container_layout->setSpacing(20);

    add_board_card_button_ = new QPushButton("+");
    add_board_card_button_->setFixedSize(280, 180);
    add_board_card_button_->setStyleSheet(
        "QPushButton { background: #f4f5f7; border: 2px dashed #dfe1e6; border-radius: 8px; "
        "font-size: 48px; color: #5e6c84; }"
        "QPushButton:hover { background: #ebedf0; border-color: #305CDE; }");
    connect(add_board_card_button_, &QPushButton::clicked, this,
            &DashboardScreen::onBoardCreateRequest);

    boards_container_layout->addWidget(add_board_card_button_);
    boards_container_layout->addStretch();

    boards_scroll_area_->setWidget(boards_container_);
    boards_layout->addWidget(boards_scroll_area_);
}

void DashboardScreen::loadStatistics() {
    if (!db_.isOpen()) {
        return;
    }

    LocalTaskRepository task_repo(db_);
    const std::vector<LocalTask> tasks = task_repo.findAll();

    int active_count = 0;
    int completed_count = 0;

    for (const LocalTask& task : tasks) {
        if (task.is_completed_) {
            completed_count++;
        } else {
            active_count++;
        }
    }

    active_tasks_value_->setText(QString::number(active_count));
    completed_tasks_value_->setText(QString::number(completed_count));

    LocalPomodoroSessionRepository pomodoro_repo(db_);
    const std::vector<LocalPomodoroSession> sessions = pomodoro_repo.findAll();

    int total_focus_seconds = 0;
    for (const LocalPomodoroSession& session : sessions) {
        total_focus_seconds += session.work_duration_seconds_;
    }

    int focus_hours = total_focus_seconds / 3600;
    int focus_minutes = (total_focus_seconds % 3600) / 60;
    focus_hours_value_->setText(QString("%1ч %2м").arg(focus_hours).arg(focus_minutes));
}

void DashboardScreen::loadDeadlines() {
    if (!db_.isOpen()) {
        return;
    }

    LocalTaskRepository task_repo(db_);
    const std::vector<LocalTask> tasks = task_repo.findAll();

    QVector<QPair<LocalTask, int>> tasks_with_deadlines;

    for (const LocalTask& task : tasks) {
        if (!task.deadline_.isEmpty() && !task.is_completed_) {
            tasks_with_deadlines.append(qMakePair(task, task.board_id_));
        }
    }

    std::sort(tasks_with_deadlines.begin(), tasks_with_deadlines.end(),
              [](const QPair<LocalTask, int>& a, const QPair<LocalTask, int>& b) {
                  return a.first.deadline_ < b.first.deadline_;
              });

    for (auto* card : deadline_cards_) {
        card->deleteLater();
    }
    deadline_cards_.clear();

    if (tasks_with_deadlines.isEmpty()) {
        no_deadlines_label_->show();
    } else {
        no_deadlines_label_->hide();
        int count = qMin(tasks_with_deadlines.size(), 3);
        for (int i = 0; i < count; ++i) {
            const auto& task_pair = tasks_with_deadlines[i];
            const LocalTask& task = task_pair.first;
            int board_id = task_pair.second;

            auto* card = new DdTaskCard(task.id_, board_id, task.status_id_, db_, this);

            QDateTime deadline = QDateTime::fromString(task.deadline_, Qt::ISODate);
            card->setCardData(task.title_, task.description_, deadline, task.is_completed_,
                              task.priority_color_);

            connect(card, &DdTaskCard::openBoardRequested, this, &DashboardScreen::onBoardRequest);

            auto* container_layout = qobject_cast<QHBoxLayout*>(deadlines_container_->layout());
            if (container_layout) {
                container_layout->insertWidget(container_layout->count() - 1, card);
            }
            deadline_cards_.append(card);
        }
    }
}

void DashboardScreen::loadBoards() {
    if (!db_.isOpen()) {
        return;
    }

    LocalBoardRepository board_repo(db_);
    const std::vector<LocalBoard> boards = board_repo.findAll();

    LocalTaskRepository task_repo(db_);

    clearBoards();

    auto* boards_container_layout = qobject_cast<QHBoxLayout*>(boards_container_->layout());
    if (!boards_container_layout) {
        return;
    }

    for (const LocalBoard& board : boards) {
        auto* card = new BoardCard(board.id_, this);
        QString description = board.description_;
        if (description.isEmpty()) {
            description = "Нет описания";
        }

        const std::vector<LocalTask> board_tasks = task_repo.findByBoardId(board.id_);
        int active_tasks = 0;
        int completed_tasks = 0;
        QDateTime nearest_deadline;

        for (const LocalTask& task : board_tasks) {
            if (task.is_completed_) {
                completed_tasks++;
            } else {
                active_tasks++;
            }
            if (!task.deadline_.isEmpty()) {
                QDateTime deadline = QDateTime::fromString(task.deadline_, Qt::ISODate);
                if (!nearest_deadline.isValid() || deadline < nearest_deadline) {
                    nearest_deadline = deadline;
                }
            }
        }

        card->setBoardData(board.title_, description, active_tasks, completed_tasks,
                           nearest_deadline);

        connect(card, &BoardCard::openBoardRequested, this, &DashboardScreen::onBoardRequest);

        boards_container_layout->insertWidget(boards_container_layout->count() - 1, card);
        board_cards_.append(card);
    }
}

void DashboardScreen::clearBoards() {
    for (auto* card : board_cards_) {
        card->deleteLater();
    }
    board_cards_.clear();
}

void DashboardScreen::onBoardCreateRequest() {
    emit openBoardCreateScreen();
}

void DashboardScreen::onProfileRequest() {
    emit openProfileScreen();
}

void DashboardScreen::onPomodoroRequest() {
    emit openPomodoroScreen();
}

void DashboardScreen::onBoardRequest(int board_id) {
    emit openBoardScreen(board_id);
}

void DashboardScreen::onLogoutRequest() {
    emit logoutRequested();
}

void DashboardScreen::updateDashboardButton() {
    dashboard_button_->setStyleSheet("QPushButton { "
                                     "   background: #305CDE; "
                                     "   color: white; "
                                     "   border: 1px solid #305CDE; "
                                     "   padding: 8px; "
                                     "   border-radius: 6px; "
                                     "   font-size: 12px; "
                                     "   font-weight: bold; "
                                     "}"
                                     "QPushButton:hover { "
                                     "   background: #305CDE; "
                                     "   border-color: #305CDE; "
                                     "}"
                                     "QPushButton:pressed { "
                                     "   background: #1A3BB0; "
                                     "   border-color: #1A3BB0; "
                                     "}");
}
