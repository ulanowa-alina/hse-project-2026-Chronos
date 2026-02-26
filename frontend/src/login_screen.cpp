#include "login_screen.h"
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-owning-memory)
LoginScreen::LoginScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void LoginScreen::setupLayout() {
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



    login_title_label_ = new QLabel("Вход", this);
    login_title_label_->setStyleSheet(
        "font-size: 24px; "
        "font-weight: bold; "
        "color: #305CDE; "
        "font-family: 'Arial';"
    );
    login_title_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(login_title_label_);

    main_layout->addSpacing(30);


    auto* email_container = new QWidget(this);
    email_container->setStyleSheet("background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
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

    auto* pass_container = new QWidget(this);
    pass_container->setStyleSheet("background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* pass_lay = new QVBoxLayout(pass_container);
    pass_lay->setContentsMargins(15, 8, 15, 8);
    pass_lay->setSpacing(2);

    auto* pass_hint = new QLabel("Password", pass_container);
    pass_hint->setStyleSheet("color: #8E8E8E; font-size: 12px; border: none;");

    password_input_ = new QLineEdit(pass_container); // Создаем пустым
    password_input_->setEchoMode(QLineEdit::Password); // Скрываем вводимые символы
    password_input_->setPlaceholderText("Create a password"); // Подсказка в виде точек
    password_input_->setStyleSheet("border: none; font-size: 16px; background: transparent;");

    pass_lay->addWidget(pass_hint);
    pass_lay->addWidget(password_input_);
    main_layout->addWidget(pass_container);

    main_layout->addSpacing(15);

    auto* footer_layout = new QHBoxLayout();
    footer_layout->setAlignment(Qt::AlignCenter);

    auto* no_acc_text = new QLabel("Нет аккаунта?", this);
    no_acc_text->setStyleSheet("color: black; font-size: 14px;");

    auto* reg_btn = new QPushButton("Зарегистрироваться", this);
    reg_btn->setCursor(Qt::PointingHandCursor);
    reg_btn->setStyleSheet(
        "QPushButton { color: #E53935; text-decoration: underline; border: none; "
        "background: none; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { color: #C62828; }"
    );

    footer_layout->addWidget(no_acc_text);
    footer_layout->addWidget(reg_btn);
    main_layout->addLayout(footer_layout);

    main_layout->addStretch();
    main_layout->addStretch();

    login_button_ = new QPushButton("Войти");
    login_button_->setMinimumHeight(45);
    login_button_->setStyleSheet("QPushButton { "
                                "   background-color: #305CDE; "
                                "   color: white; "
                                "   border-radius: 10px; "
                                "   font-weight: bold; "
                                "   font-size: 15px; "
                                "}"
                                "QPushButton:hover { "
                                "   background-color: #2549B3; "
                                "}");
    main_layout->addWidget(login_button_);

    connect(login_button_, &QPushButton::clicked, this, &LoginScreen::loginRequested);
    //connect(reg_btn, &QPushButton::clicked, this, &LoginScreen::registrationRequested);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
