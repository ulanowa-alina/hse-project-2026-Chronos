#include "registration_screen.h"
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)
RegistrationScreen::RegistrationScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}
void RegistrationScreen::setupLayout() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(20, 15, 20, 20);
    main_layout->setSpacing(10);

    auto* top_bar = new QHBoxLayout();

    logo_label_ = new QLabel("Chronos");
    logo_label_->setStyleSheet(
        "font-weight: bold; color: #305CDE; font-size: 18px; font-family: 'Arial';");

    top_bar->addWidget(logo_label_);
    top_bar->addStretch();
    main_layout->addLayout(top_bar);

    main_layout->addStretch();

    login_title_label_ = new QLabel("Регистрация", this);
    login_title_label_->setStyleSheet(
        "font-size: 32px; font-weight: bold; color: #305CDE; font-family: 'Arial';");
    login_title_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(login_title_label_);

    main_layout->addSpacing(10);
    auto* avatar_layout = new QHBoxLayout();
    avatar_layout->setAlignment(Qt::AlignCenter);

    avatar_button_ = new QPushButton(this);
    int avatar_size = 100;
    avatar_button_->setFixedSize(avatar_size, avatar_size);
    avatar_button_->setCursor(Qt::PointingHandCursor);

    avatar_button_->setStyleSheet("QPushButton {"
                                  "   background-color: #F0F2F5;"
                                  "   border: 2px dashed #305CDE;"
                                  "   border-radius: 50px;"
                                  "   color: #305CDE;"
                                  "   font-size: 30px;"
                                  "   font-weight: bold;"
                                  "}"
                                  "QPushButton:hover {"
                                  "   background-color: #E1E4E8;"
                                  "}");
    avatar_button_->setText("+");

    avatar_layout->addWidget(avatar_button_);
    main_layout->addLayout(avatar_layout);

    main_layout->addSpacing(10);
    auto create_field = [this](const QString& hint, QLineEdit*& input, const QString& placeholder,
                               bool is_password = false) {
        auto* container = new QWidget(this);
        container->setStyleSheet(
            "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
        auto* lay = new QVBoxLayout(container);
        lay->setContentsMargins(15, 8, 15, 8);
        lay->setSpacing(2);

        auto* label = new QLabel(hint, container);
        label->setStyleSheet("color: #8E8E8E; font-size: 11px; border: none; font-weight: bold;");

        input = new QLineEdit(container);
        input->setPlaceholderText(placeholder);
        input->setStyleSheet(
            "border: none; font-size: 16px; background: transparent; color: #333333;");

        if (is_password) {
            input->setEchoMode(QLineEdit::Password);
        }

        lay->addWidget(label);
        lay->addWidget(input);
        return container;
    };

    main_layout->addWidget(create_field("Name", name_input_, "Enter name"));
    main_layout->addWidget(create_field("Email", email_input_, "Enter email"));
    main_layout->addWidget(create_field("Status", status_input_, "What is your position?"));
    main_layout->addWidget(create_field("Password", password_input_, "Create a password", true));

    main_layout->addSpacing(15);

    auto* footer_layout = new QHBoxLayout();
    footer_layout->setAlignment(Qt::AlignCenter);
    auto* has_acc_text = new QLabel("Уже есть аккаунт?", this);
    has_acc_text->setStyleSheet("color: black; font-size: 14px;");

    login_button_ = new QPushButton("Войти", this);
    login_button_->setCursor(Qt::PointingHandCursor);
    login_button_->setStyleSheet(
        "QPushButton { color: #E53935; text-decoration: underline; border: none; "
        "background: none; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { color: #C62828; }");
    footer_layout->addWidget(has_acc_text);
    footer_layout->addWidget(login_button_);
    main_layout->addLayout(footer_layout);

    main_layout->addStretch();
    registration_button_ = new QPushButton("Создать аккаунт", this);
    registration_button_->setMinimumHeight(50);
    registration_button_->setCursor(Qt::PointingHandCursor);
    registration_button_->setStyleSheet("QPushButton { background-color: #305CDE; color: white; "
                                        "border-radius: 12px; font-weight: bold; font-size: 16px; }"
                                        "QPushButton:hover { background-color: #2549B3; }");
    main_layout->addWidget(registration_button_);

    connect(registration_button_, &QPushButton::clicked, this,
            &RegistrationScreen::registrationRequested);
    connect(login_button_, &QPushButton::clicked, this, &RegistrationScreen::loginRequested);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
