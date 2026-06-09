#include "task_create_screen.h"

#include <QCheckBox>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>

TaskCreateScreen::TaskCreateScreen(int board_id, int status_id, QWidget* parent)
    : QWidget(parent)
    , board_id_(board_id)
    , status_id_(status_id) {
    setupLayout();
}

void TaskCreateScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
}

void TaskCreateScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void TaskCreateScreen::setBoardId(int board_id) {
    board_id_ = board_id;
}

void TaskCreateScreen::setStatusId(int status_id) {
    status_id_ = status_id;
}

void TaskCreateScreen::setDatabase(QSqlDatabase* db) {
    db_ = db;
}

void TaskCreateScreen::onCreateTaskRequest() {
    QString title = title_input_->text().trimmed();
    QString description = description_input_->toPlainText().trimmed();
    QString priority_color = priority_combo_->currentText();
    if (priority_color == "🟢Низкий") {
        priority_color = "green";
    } else if (priority_color == "🟡Средний") {
        priority_color = "yellow";
    } else if (priority_color == "🔴Высокий") {
        priority_color = "red";
    } else {
        priority_color = "none";
    }

    QDateTime deadline = deadline_input_->dateTime();

    if (title.isEmpty()) {
        qDebug() << "TaskCreateScreen: Название задачи не может быть пустым";
        return;
    }

    if (db_) {
        try {
            LocalTaskRepository repo(*db_);
            int local_id = repo.createLocalId();

            QString deadline_str;
            if (deadline_checkbox_->isChecked() && deadline.isValid()) {
                deadline_str = deadline.toUTC().toString(Qt::ISODate);
            }

            LocalTask local_task(local_id, board_id_, title, status_id_, priority_color,
                                 description, deadline_str, false, QString(), QString(), QString(),
                                 SyncStatus::PENDING, 0);

            repo.save(local_task);
            qDebug() << "TaskCreateScreen: Задача сохранена локально с ID:" << local_id;

            clearFields();
            emit taskCreated();

            if (sync_coordinator_) {
                sync_coordinator_->syncTasks();
            }
        } catch (const std::exception& e) {
            qDebug() << "TaskCreateScreen: Ошибка сохранения в локальную БД:" << e.what();
        }
    }
}

void TaskCreateScreen::onCloseRequest() {
    emit closeRequested();
}

void TaskCreateScreen::onDeadlineCheckChanged(Qt::CheckState state) {
    deadline_input_->setEnabled(state == Qt::Checked);
}

void TaskCreateScreen::setupLayout() {
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

    title_label_ = new QLabel("Создать задачу", this);
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
    title_input_->setPlaceholderText("Название задачи");
    title_input_->setStyleSheet(
        "border: none; font-size: 16px; background: transparent; color: #333333;");
    main_layout->addWidget(create_field("Название", title_input_));

    main_layout->addSpacing(10);

    description_input_ = new QTextEdit(this);
    description_input_->setPlaceholderText("Описание задачи");
    description_input_->setMaximumHeight(100);
    description_input_->setStyleSheet(
        "border: none; font-size: 14px; background: transparent; color: #333333;");
    main_layout->addWidget(create_field("Описание", description_input_));

    main_layout->addSpacing(10);

    priority_combo_ = new QComboBox(this);
    priority_combo_->addItem("Не установлен");
    priority_combo_->addItem("🟢Низкий");
    priority_combo_->addItem("🟡Средний");
    priority_combo_->addItem("🔴Высокий");
    priority_combo_->setStyleSheet(
        "border: none; font-size: 16px; background: transparent; color: #333333;");
    main_layout->addWidget(create_field("Уровень приоритетности", priority_combo_));

    main_layout->addSpacing(10);

    auto* deadline_container = new QWidget(this);
    deadline_container->setStyleSheet(
        "background: white; border: 1px solid #D1D1D1; border-radius: 10px;");
    auto* deadline_lay = new QVBoxLayout(deadline_container);
    deadline_lay->setContentsMargins(15, 8, 15, 8);
    deadline_lay->setSpacing(8);

    auto* deadline_label = new QLabel("Дедлайн", deadline_container);
    deadline_label->setStyleSheet(
        "color: #8E8E8E; font-size: 12px; border: none; font-weight: bold;");

    auto* deadline_inner_lay = new QHBoxLayout();
    deadline_inner_lay->setContentsMargins(0, 0, 0, 0);
    deadline_inner_lay->setSpacing(10);

    deadline_checkbox_ = new QCheckBox("Установить дедлайн", this);
    deadline_checkbox_->setStyleSheet("border: none; font-size: 14px; color: #333333;");
    deadline_checkbox_->setChecked(false);

    deadline_input_ = new QDateTimeEdit(this);
    deadline_input_->setCalendarPopup(true);
    deadline_input_->setDateTime(QDateTime::currentDateTime().addDays(1));
    deadline_input_->setDisplayFormat("dd.MM.yyyy hh:mm");
    deadline_input_->setStyleSheet(
        "border: none; font-size: 14px; background: transparent; color: #333333;");
    deadline_input_->setEnabled(false);

    deadline_inner_lay->addWidget(deadline_checkbox_);
    deadline_inner_lay->addWidget(deadline_input_);

    deadline_lay->addWidget(deadline_label);
    deadline_lay->addLayout(deadline_inner_lay);
    main_layout->addWidget(deadline_container);

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

    connect(create_button_, &QPushButton::clicked, this, &TaskCreateScreen::onCreateTaskRequest);
    connect(cancel_button_, &QPushButton::clicked, this, &TaskCreateScreen::onCloseRequest);
    connect(close_button_, &QPushButton::clicked, this, &TaskCreateScreen::onCloseRequest);
    connect(deadline_checkbox_, &QCheckBox::checkStateChanged, this,
            &TaskCreateScreen::onDeadlineCheckChanged);
}

void TaskCreateScreen::clearFields() {
    title_input_->clear();
    description_input_->clear();
    priority_combo_->setCurrentIndex(0);
    deadline_checkbox_->setChecked(false);
    deadline_input_->setEnabled(false);
    deadline_input_->setDateTime(QDateTime::currentDateTime().addDays(1));
}
