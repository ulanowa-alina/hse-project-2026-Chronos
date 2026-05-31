#include "profile_edit_screen.h"

#include "../local_repositories/local_user_repository.hpp"

#include <QVBoxLayout>

ProfileEditScreen::ProfileEditScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void ProfileEditScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void ProfileEditScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void ProfileEditScreen::reloadFromLocal() {
    if (!db_.isOpen()) {
        return;
    }

    LocalUserRepository repo(db_);
    const auto user = repo.getCurrentUser();
    if (!user) {
        return;
    }

    name_input_->setText(user->name_);
    email_input_->setText(user->email_);
    status_input_->setText(user->status_);
}

void ProfileEditScreen::onProfileEditRequest() {
    if (!sync_coordinator_ || !db_.isOpen()) {
        return;
    }

    LocalUserRepository repo(db_);
    const auto user = repo.getCurrentUser();
    if (!user) {
        return;
    }

    LocalUser updated = *user;
    updated.name_ = name_input_->text();
    updated.email_ = email_input_->text();
    updated.status_ = status_input_->text();
    updated.sync_status_ = SyncStatus::PENDING;
    repo.save(updated);

    pending_password_ = password_input_->text();
    password_input_->clear();

    if (!pending_password_.isEmpty()) {
        sync_coordinator_->setPassword(pending_password_);
        pending_password_.clear();
    }
    sync_coordinator_->syncUsers();
    emit profileRequested();
}

void ProfileEditScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_label_ = new QLabel("Chronos");
    logo_label_->setStyleSheet(
        "font-weight: bold; color: #305CDE; font-size: 18px; font-family: 'Arial';");

    cancel_button_ = new QPushButton("Отмена", this);
    cancel_button_->setCursor(Qt::PointingHandCursor);
    cancel_button_->setStyleSheet(
        "QPushButton { "
        "   background: transparent; color: #C03438; border: none; "
        "   font-size: 16px; font-weight: 400; padding: 0px; "
        "}"
        "QPushButton:hover { color: #e74c3c; text-decoration: underline; }");

    top_bar->addWidget(logo_label_);
    top_bar->addStretch();
    top_bar->addWidget(cancel_button_);
    main_layout->addLayout(top_bar);

    main_layout->addStretch();

    auto* name_container = new QWidget(this);
    name_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* name_lay = new QVBoxLayout(name_container);
    name_lay->setContentsMargins(15, 8, 15, 8);
    name_lay->setSpacing(2);

    auto* name_hint = new QLabel("Name", name_container);
    name_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    name_input_ = new QLineEdit(name_container);

    name_input_->setPlaceholderText("Enter new name");
    name_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    name_lay->addWidget(name_hint);
    name_lay->addWidget(name_input_);
    main_layout->addWidget(name_container);

    main_layout->addSpacing(10);

    auto* email_container = new QWidget(this);
    email_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* email_lay = new QVBoxLayout(email_container);
    email_lay->setContentsMargins(15, 8, 15, 8);
    email_lay->setSpacing(2);

    auto* email_hint = new QLabel("Email", email_container);
    email_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    email_input_ = new QLineEdit(email_container);

    email_input_->setPlaceholderText("Enter email");
    email_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    email_lay->addWidget(email_hint);
    email_lay->addWidget(email_input_);
    main_layout->addWidget(email_container);

    main_layout->addSpacing(10);

    auto* status_container = new QWidget(this);
    status_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* status_lay = new QVBoxLayout(status_container);
    status_lay->setContentsMargins(15, 8, 15, 8);
    status_lay->setSpacing(2);

    auto* status_hint = new QLabel("Status", status_container);
    status_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    status_input_ = new QLineEdit(status_container);

    status_input_->setPlaceholderText("Enter status");
    status_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    status_lay->addWidget(status_hint);
    status_lay->addWidget(status_input_);
    main_layout->addWidget(status_container);

    main_layout->addSpacing(10);

    auto* pass_container = new QWidget(this);
    pass_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* pass_lay = new QVBoxLayout(pass_container);
    pass_lay->setContentsMargins(15, 8, 15, 8);
    pass_lay->setSpacing(2);

    auto* pass_hint = new QLabel("Password", pass_container);
    pass_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    password_input_ = new QLineEdit(pass_container);
    password_input_->setEchoMode(QLineEdit::Password);
    password_input_->setPlaceholderText("Enter password");
    password_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    pass_lay->addWidget(pass_hint);
    pass_lay->addWidget(password_input_);
    main_layout->addWidget(pass_container);

    main_layout->addSpacing(15);

    main_layout->addStretch();
    main_layout->addStretch();

    save_button_ = new QPushButton("Сохранить изменения");
    save_button_->setMinimumHeight(45);
    save_button_->setStyleSheet("QPushButton { "
                                "   background-color: #305CDE; "
                                "   color: white; "
                                "   border-radius: 10px; "
                                "   font-weight: bold; "
                                "   font-size: 15px; "
                                "}"
                                "QPushButton:hover { "
                                "   background-color: #2549B3; "
                                "}");
    main_layout->addWidget(save_button_);

    connect(cancel_button_, &QPushButton::clicked, this, &ProfileEditScreen::profileRequested);
    connect(save_button_, &QPushButton::clicked, this, &ProfileEditScreen::onProfileEditRequest);
}
