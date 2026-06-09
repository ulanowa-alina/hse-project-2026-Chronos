#include "welcome_screen.hpp"

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void WelcomeScreen::setupLayout() {
    auto* main_grid = new QGridLayout(this);
    main_grid->setContentsMargins(0, 0, 0, 0);

    auto* blob = new BlueOval(this, QColor(48, 92, 222, 120), 80);
    blob->setFixedSize(1100, 600);
    main_grid->addWidget(blob, 0, 0, Qt::AlignCenter);

    auto* content_container = new QWidget();
    content_container->setStyleSheet("background: transparent;");
    auto* content_layout = new QVBoxLayout(content_container);
    content_layout->setSpacing(30);
    content_layout->setContentsMargins(0, 50, 0, 50);

    auto* title_label = new QLabel("Chronos");
    title_label->setStyleSheet("font-size: 72px; "
                               "font-weight: bold; "
                               "color: #002092; "
                               "background: transparent;");
    title_label->setAlignment(Qt::AlignCenter);

    auto* desc = new QLabel("Управляй временем и личной продуктивностью");
    desc->setStyleSheet("font-size: 22px; color: #002092; background: transparent;");
    desc->setAlignment(Qt::AlignCenter);

    start_button_ = new QPushButton("Начать работу");
    start_button_->setCursor(Qt::PointingHandCursor);
    start_button_->setStyleSheet("QPushButton { "
                                 "   background-color: #002092; "
                                 "   border: none; "
                                 "   color: white; "
                                 "   padding: 12px 40px; "
                                 "   border-radius: 8px; "
                                 "   font-size: 18px; "
                                 "   font-weight: bold; "
                                 "} "
                                 "QPushButton:hover { "
                                 "   background-color: #002bc5; "
                                 "   color: white; "
                                 "}");
    connect(start_button_, &QPushButton::clicked, this, &WelcomeScreen::loginRequested);

    content_layout->addWidget(title_label);
    content_layout->addWidget(desc);
    content_layout->addWidget(start_button_, 0, Qt::AlignCenter);
    content_layout->addStretch();
    main_grid->addWidget(content_container, 0, 0, Qt::AlignCenter);

    main_grid->setRowStretch(0, 1);
}
