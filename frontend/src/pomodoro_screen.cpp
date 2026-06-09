#include "pomodoro_screen.h"

#include "../local_repositories/local_pomodoro_session_repository.hpp"
#include "../local_repositories/local_user_repository.hpp"

#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <algorithm>

PomodoroScreen::PomodoroScreen(QWidget* parent)
    : QWidget(parent) {
    stacked_widget_ = new QStackedWidget(this);

    setupWelcomeScreen();
    setupGoalSelectionScreen();
    setupTimerScreen();

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addWidget(stacked_widget_);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PomodoroScreen::onTimerTick);

    switchState(ScreenState::Welcome);
}

void PomodoroScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &PomodoroScreen::onNetworkResponse);
    }
}

void PomodoroScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void PomodoroScreen::setupWelcomeScreen() {
    welcome_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(welcome_widget_);
    layout->setContentsMargins(36, 48, 36, 48);
    layout->setSpacing(24);

    welcome_label_ = new QLabel("<h2 style='color: #172b4d;'>Режим Pomodoro</h2>"
                                "<p style='color: #5e6c84; font-size: 14px;'>"
                                "Техника Pomodoro помогает сохранять фокус и продуктивность. "
                                "Работайте 30 минут, затем отдыхайте 5 минут. "
                                "Повторяйте циклы для достижения максимальной эффективности"
                                "</p>");
    welcome_label_->setWordWrap(true);
    welcome_label_->setAlignment(Qt::AlignCenter);
    welcome_label_->setMaximumWidth(320);
    welcome_label_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    start_focus_button_ = new QPushButton("Начать фокусировку");
    start_focus_button_->setCursor(Qt::PointingHandCursor);
    start_focus_button_->setStyleSheet(
        "QPushButton { background: #305CDE; color: white; border: none; padding: 15px 30px; "
        "border-radius: 8px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2654CC; }");
    start_focus_button_->setFixedHeight(50);
    connect(start_focus_button_, &QPushButton::clicked, this, &PomodoroScreen::onStartFocusClicked);

    layout->addStretch();
    layout->addWidget(welcome_label_, 0, Qt::AlignCenter);
    layout->addWidget(start_focus_button_, 0, Qt::AlignCenter);
    layout->addStretch();

    stacked_widget_->addWidget(welcome_widget_);
}

void PomodoroScreen::setupGoalSelectionScreen() {
    goal_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(goal_widget_);
    layout->setContentsMargins(50, 100, 50, 100);
    layout->setSpacing(30);

    goal_label_ = new QLabel(
        "<h2 style='color: #172b4d;'>Установите цель</h2>"
        "<p style='color: #5e6c84; font-size: 14px;'>"
        "Укажите, сколько минут вы хотите работать, или пропустите этот шаг для свободного режима"
        "</p>");
    goal_label_->setWordWrap(true);
    goal_label_->setAlignment(Qt::AlignCenter);

    auto* spinbox_layout = new QHBoxLayout();
    spinbox_layout->addStretch();

    goal_spinbox_ = new QSpinBox();
    goal_spinbox_->setRange(1, 480);
    goal_spinbox_->setValue(60);
    goal_spinbox_->setSuffix(" мин");
    goal_spinbox_->setStyleSheet("QSpinBox { padding: 10px; border: 2px solid #dfe1e6; "
                                 "border-radius: 5px; font-size: 16px; }"
                                 "QSpinBox:focus { border-color: #305CDE; }");
    goal_spinbox_->setFixedWidth(150);

    spinbox_layout->addWidget(goal_spinbox_);
    spinbox_layout->addStretch();

    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();

    confirm_goal_button_ = new QPushButton("Подтвердить");
    confirm_goal_button_->setCursor(Qt::PointingHandCursor);
    confirm_goal_button_->setStyleSheet(
        "QPushButton { background: #305CDE; color: white; border: none; padding: 12px 25px; "
        "border-radius: 6px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #2654CC; }");
    connect(confirm_goal_button_, &QPushButton::clicked, this, &PomodoroScreen::onGoalConfirmed);

    skip_goal_button_ = new QPushButton("Без цели");
    skip_goal_button_->setCursor(Qt::PointingHandCursor);
    skip_goal_button_->setStyleSheet(
        "QPushButton { background: #ebedf0; color: #172b4d; border: none; padding: 12px 25px; "
        "border-radius: 6px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #dadce2; }");
    connect(skip_goal_button_, &QPushButton::clicked, this, &PomodoroScreen::onSkipGoalClicked);

    button_layout->addWidget(confirm_goal_button_);
    button_layout->addWidget(skip_goal_button_);
    button_layout->addStretch();

    layout->addStretch();
    layout->addWidget(goal_label_, 0, Qt::AlignCenter);
    layout->addLayout(spinbox_layout);
    layout->addLayout(button_layout);
    layout->addStretch();

    stacked_widget_->addWidget(goal_widget_);
}

void PomodoroScreen::setupTimerScreen() {
    timer_widget_ = new QWidget();
    auto* layout = new QVBoxLayout(timer_widget_);
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(30);

    state_label_ = new QLabel("РАБОТА");
    state_label_->setStyleSheet("font-size: 24px; font-weight: bold; color: #305CDE;");
    state_label_->setAlignment(Qt::AlignCenter);

    circular_progress_ = new CircularProgress();
    circular_progress_->setValue(100);

    stop_button_ = new QPushButton("Завершить фокусировку");
    stop_button_->setCursor(Qt::PointingHandCursor);
    stop_button_->setStyleSheet(
        "QPushButton { background: #de350b; color: white; border: none; padding: 12px 25px; "
        "border-radius: 6px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #c92a0a; }");
    connect(stop_button_, &QPushButton::clicked, this, &PomodoroScreen::onStopClicked);

    layout->addStretch();
    layout->addWidget(state_label_, 0, Qt::AlignCenter);
    layout->addWidget(circular_progress_, 0, Qt::AlignCenter);
    layout->addWidget(stop_button_, 0, Qt::AlignCenter);
    layout->addStretch();

    stacked_widget_->addWidget(timer_widget_);
}

void PomodoroScreen::switchState(ScreenState state) {
    current_state_ = state;

    switch (state) {
        case ScreenState::Welcome:
            stacked_widget_->setCurrentWidget(welcome_widget_);
            break;
        case ScreenState::GoalSelection:
            stacked_widget_->setCurrentWidget(goal_widget_);
            break;
        case ScreenState::Timer:
            stacked_widget_->setCurrentWidget(timer_widget_);
            break;
    }
}

void PomodoroScreen::onStartFocusClicked() {
    switchState(ScreenState::GoalSelection);
}

void PomodoroScreen::onGoalConfirmed() {
    has_goal_ = true;
    goal_minutes_ = goal_spinbox_->value();
    switchState(ScreenState::Timer);
    startTimer();
}

void PomodoroScreen::onSkipGoalClicked() {
    has_goal_ = false;
    goal_minutes_ = 0;
    switchState(ScreenState::Timer);
    startTimer();
}

void PomodoroScreen::startTimer() {
    is_work_phase_ = true;
    total_work_seconds_ = nextWorkPhaseSeconds();
    remaining_seconds_ = total_work_seconds_;
    completed_cycles_ = 0;
    total_work_time_seconds_ = 0;
    last_saved_work_minutes_ = 0;
    local_session_id_ = 0;

    state_label_->setText("РАБОТА");
    state_label_->setStyleSheet("font-size: 24px; font-weight: bold; color: #305CDE;");
    circular_progress_->setTimerText(formatTime(remaining_seconds_));
    circular_progress_->setValue(100);
    circular_progress_->setColor(QColor("#305CDE"));

    timer_->start(1000);
}

void PomodoroScreen::stopTimer() {
    timer_->stop();
    saveSession();

    completed_cycles_ = 0;
    total_work_time_seconds_ = 0;

    switchState(ScreenState::Welcome);
}

void PomodoroScreen::onTimerTick() {
    if (remaining_seconds_ <= 0) {
        return;
    }

    remaining_seconds_--;

    if (is_work_phase_) {
        total_work_time_seconds_++;
        const int worked_minutes = total_work_time_seconds_ / 60;
        if (worked_minutes > last_saved_work_minutes_) {
            saveLocalSession();
        }
    }

    circular_progress_->setTimerText(formatTime(remaining_seconds_));

    int total_seconds = is_work_phase_ ? total_work_seconds_ : total_break_seconds_;
    int progress =
        static_cast<int>((static_cast<double>(remaining_seconds_) / total_seconds) * 100);
    circular_progress_->setValue(progress);

    if (remaining_seconds_ <= 0) {
        if (is_work_phase_) {
            completed_cycles_++;
            is_work_phase_ = false;
            remaining_seconds_ = total_break_seconds_;

            state_label_->setText("ОТДЫХ");
            state_label_->setStyleSheet("font-size: 24px; font-weight: bold; color: #00875a;");
            circular_progress_->setColor(QColor("#00875a"));
        } else {
            is_work_phase_ = true;
            total_work_seconds_ = nextWorkPhaseSeconds();
            remaining_seconds_ = total_work_seconds_;

            state_label_->setText("РАБОТА");
            state_label_->setStyleSheet("font-size: 24px; font-weight: bold; color: #305CDE;");
            circular_progress_->setColor(QColor("#305CDE"));
        }

        circular_progress_->setTimerText(formatTime(remaining_seconds_));
        circular_progress_->setValue(100);

        if (has_goal_) {
            int goal_seconds = goal_minutes_ * 60;
            if (total_work_time_seconds_ >= goal_seconds) {
                stopTimer();
                return;
            }
        }
    }
}

int PomodoroScreen::nextWorkPhaseSeconds() const {
    if (!has_goal_) {
        return default_work_seconds_;
    }

    const int goal_seconds = goal_minutes_ * 60;
    const int remaining_goal_seconds = goal_seconds - total_work_time_seconds_;
    return std::min(default_work_seconds_, remaining_goal_seconds);
}

void PomodoroScreen::onStopClicked() {
    stopTimer();
}

void PomodoroScreen::saveSession() {
    if (total_work_time_seconds_ <= 0) {
        return;
    }

    const int saved_work_minutes = (total_work_time_seconds_ + 59) / 60;

    saveLocalSession();

    if (!network_manager_) {
        return;
    }

    QJsonObject json;
    json["goal_minutes"] = has_goal_ ? goal_minutes_ : QJsonValue();
    json["work_duration_seconds"] = saved_work_minutes * 60;
    json["break_duration_seconds"] = total_break_seconds_;
    json["completed_cycles"] = completed_cycles_;

    network_manager_->POST(network_manager_->pomodoro_create_url_, json);
}

void PomodoroScreen::saveLocalSession() {
    if (!db_.isOpen() || total_work_time_seconds_ <= 0) {
        return;
    }

    try {
        LocalUserRepository user_repo(db_);
        const auto user = user_repo.getCurrentUser();
        if (!user) {
            return;
        }

        const int saved_work_minutes = (total_work_time_seconds_ + 59) / 60;
        if (saved_work_minutes <= 0) {
            return;
        }

        LocalPomodoroSessionRepository session_repo(db_);
        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        if (local_session_id_ <= 0) {
            local_session_id_ = session_repo.getNextId();
        }

        LocalPomodoroSession session(local_session_id_, user->id_, saved_work_minutes * 60,
                                     total_break_seconds_, has_goal_ ? goal_minutes_ : 0,
                                     completed_cycles_, now, QString(), now, now, QString(),
                                     SyncStatus::PENDING, 0);

        if (session_repo.findById(local_session_id_)) {
            session_repo.update(session);
        } else {
            session_repo.insert(session);
        }

        last_saved_work_minutes_ = saved_work_minutes;
        emit sessionSaved();
    } catch (const std::exception& e) {
        qDebug() << "PomodoroScreen: Error saving local session:" << e.what();
    }
}

void PomodoroScreen::saveLocalSessionFromServer(const QJsonObject& obj) {
    if (!db_.isOpen() || !obj.contains("id")) {
        return;
    }

    try {
        LocalPomodoroSessionRepository session_repo(db_);
        const int server_id = obj["id"].toInt();
        const int goal_minutes =
            obj.value("goal_minutes").isNull() ? 0 : obj.value("goal_minutes").toInt();
        const QString started_at = obj.value("started_at").toString();
        const QString completed_at =
            obj.value("completed_at").isNull() ? QString() : obj.value("completed_at").toString();
        const QString updated_at = completed_at.isEmpty() ? started_at : completed_at;

        if (local_session_id_ > 0 && local_session_id_ != server_id) {
            session_repo.deleteById(local_session_id_);
        }

        LocalPomodoroSession session(
            server_id, obj["user_id"].toInt(), obj["work_duration_seconds"].toInt(),
            obj["break_duration_seconds"].toInt(), goal_minutes, obj["completed_cycles"].toInt(),
            started_at, completed_at, started_at, updated_at, QString(), SyncStatus::SYNCED, 1);

        if (session_repo.findById(server_id)) {
            session_repo.update(session);
        } else {
            session_repo.insert(session);
        }

        local_session_id_ = server_id;
        last_saved_work_minutes_ = session.work_duration_seconds_ / 60;
        emit sessionSaved();
    } catch (const std::exception& e) {
        qDebug() << "PomodoroScreen: Error saving server session locally:" << e.what();
    }
}

QString PomodoroScreen::formatTime(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void PomodoroScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data, int code) {
    if (endpoint != network_manager_->pomodoro_create_url_) {
        return;
    }

    if (code >= 200 && code < 300) {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const QJsonObject session = doc.object().value("data").toObject();
            saveLocalSessionFromServer(session);
        }
        qDebug() << "PomodoroScreen: Session saved successfully";
    } else {
        qDebug() << "PomodoroScreen: Error saving session:" << code;
    }
}
