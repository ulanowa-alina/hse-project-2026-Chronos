#include "board_create_screen.h"

#include <QDebug>
#include <QJsonObject>

BoardCreateScreen::BoardCreateScreen(QWidget* parent)
    : QWidget(parent) {
    setupLayout();
}

void BoardCreateScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;

    if (network_manager_) {
        connect(network_manager_, &NetworkManager::responseReceived, this,
                &BoardCreateScreen::onNetworkResponse);
    }
}

void BoardCreateScreen::onNetworkResponse(const QString& endpoint, const QByteArray& data,
                                          int code) {
    if (!isVisible()) {
        return;
    }

    if (endpoint != network_manager_->boards_create_url_)
        return;

    if (endpoint == network_manager_->boards_create_url_) {
        if (code == 200 || code == 201) {
            qDebug() << "BoardCreateScreen: Доска успешно создана";
            clearFields();
            emit boardCreated();
        } else {
            qDebug() << "BoardCreateScreen: Ошибка создания доски:" << code;
        }
    }
}

void BoardCreateScreen::onCreateBoardRequest() {
    if (!network_manager_) {
        return;
    }

    QString title = title_input_->text().trimmed();
    QString description = description_input_->toPlainText().trimmed();

    if (title.isEmpty()) {
        qDebug() << "BoardCreateScreen: Название доски не может быть пустым";
        return;
    }

    QJsonObject json;
    json["title"] = title;
    json["description"] = description;

    network_manager_->POST(network_manager_->boards_create_url_, json);
}

void BoardCreateScreen::onCloseRequest() {
    emit closeRequested();
}

void BoardCreateScreen::setupLayout() {
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

    title_label_ = new QLabel("Создать доску", this);
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

    create_button_ = new QPushButton("Создать", this);
    create_button_->setMinimumHeight(45);
    create_button_->setStyleSheet(
        "QPushButton { background-color: #305CDE; color: white; border-radius: 10px; "
        "font-weight: bold; font-size: 15px; }"
        "QPushButton:hover { background-color: #2549B3; }");

    button_layout->addWidget(cancel_button_);
    button_layout->addWidget(create_button_);
    main_layout->addLayout(button_layout);

    main_layout->addStretch();

    connect(create_button_, &QPushButton::clicked, this, &BoardCreateScreen::onCreateBoardRequest);
    connect(cancel_button_, &QPushButton::clicked, this, &BoardCreateScreen::onCloseRequest);
    connect(close_button_, &QPushButton::clicked, this, &BoardCreateScreen::onCloseRequest);
}

void BoardCreateScreen::clearFields() {
    title_input_->clear();
    description_input_->clear();
}
