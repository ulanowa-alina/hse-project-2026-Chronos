#include "profile_screen.h"

#include "../local_repositories/local_user_repository.hpp"

ProfileScreen::ProfileScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void ProfileScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void ProfileScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void ProfileScreen::reloadFromLocal() {
    if (!db_.isOpen()) {
        return;
    }

    LocalUserRepository repo(db_);
    std::optional<LocalUser> user;
    if (sync_coordinator_ && sync_coordinator_->currentUserId() > 0) {
        user = repo.findById(sync_coordinator_->currentUserId());
    } else {
        user = repo.getCurrentUser();
    }
    if (!user) {
        name_label_->setText("Нет данных");
        email_label_->setText("");
        status_label_->setText("");
        return;
    }

    name_label_->setText(user->name_);
    email_label_->setText(user->email_);
    status_label_->setText(user->status_);
}

void ProfileScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_button_ = new QPushButton("Chronos", this);
    logo_button_->setCursor(Qt::PointingHandCursor);
    logo_button_->setStyleSheet("QPushButton { "
                                "   background: transparent; "
                                "   border: none; "
                                "   font-weight: bold; "
                                "   color: #305CDE; "
                                "   font-size: 18px; "
                                "   font-family: 'Arial'; "
                                "   padding: 0px; "
                                "}"
                                "QPushButton:hover { "
                                "   color: #2549B3; "
                                "}");
    logout_button_ = new QPushButton("Выход из аккаунта", this);
    logout_button_->setCursor(Qt::PointingHandCursor);
    logout_button_->setStyleSheet(
        "QPushButton { "
        "   background: transparent; color: #C03438; border: none; "
        "   font-size: 16px; font-weight: 400; padding: 0px; "
        "}"
        "QPushButton:hover { color: #e74c3c; text-decoration: underline; }");

    top_bar->addWidget(logo_button_);
    top_bar->addStretch();
    top_bar->addWidget(logout_button_);
    main_layout->addLayout(top_bar);

    main_layout->addSpacing(30);

    avatar_label_ = new QLabel("🐶");
    avatar_label_->setFixedSize(120, 120);
    avatar_label_->setAlignment(Qt::AlignCenter);
    avatar_label_->setStyleSheet("background-color: #f0f2f5; "
                                 "border: 4px solid #305CDE; "
                                 "border-radius: 60px; "
                                 "font-size: 50px;");

    name_label_ = new QLabel("Иван Иванов");
    name_label_->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");

    status_label_ = new QLabel("Developer");
    status_label_->setStyleSheet("color: #305CDE; font-weight: 500; font-size: 14px;");

    email_label_ = new QLabel("example@mail.ru");
    email_label_->setStyleSheet("color: #7f8c8d; font-size: 14px;");

    main_layout->addWidget(avatar_label_, 0, Qt::AlignCenter);
    main_layout->addWidget(name_label_, 0, Qt::AlignCenter);
    main_layout->addWidget(status_label_, 0, Qt::AlignCenter);
    main_layout->addWidget(email_label_, 0, Qt::AlignCenter);

    main_layout->addStretch();

    edit_button_ = new QPushButton("Редактировать профиль");
    edit_button_->setMinimumHeight(45);
    edit_button_->setStyleSheet("QPushButton { "
                                "   background-color: #305CDE; "
                                "   color: white; "
                                "   border-radius: 10px; "
                                "   font-weight: bold; "
                                "   font-size: 15px; "
                                "}"
                                "QPushButton:hover { "
                                "   background-color: #2549B3; "
                                "}");
    main_layout->addWidget(edit_button_);

    connect(logo_button_, &QPushButton::clicked, this, &ProfileScreen::openDashboardScreen);
    connect(logout_button_, &QPushButton::clicked, this, &ProfileScreen::logoutRequested);
    connect(edit_button_, &QPushButton::clicked, this, &ProfileScreen::profileEditRequested);
}
