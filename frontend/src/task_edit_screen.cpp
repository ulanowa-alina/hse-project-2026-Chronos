#include "task_edit_screen.h"

#include "validation_utils.h"
#include "../local_repositories/local_task_repository.hpp"

#include <QCheckBox>
#include <QDebug>

namespace {
const QString kInlineErrorStyle =
    QStringLiteral("color: #C03438; font-size: 13px; font-weight: 500; background: transparent;");
const QString kLocalSaveErrorMessage =
    QStringLiteral("Не удалось сохранить задачу. Попробуйте еще раз.");

QString priorityLabelForValue(const QString& priority_value) {
    if (priority_value == "green") {
        return QStringLiteral("🟢Низкий");
    }
    if (priority_value == "yellow") {
        return QStringLiteral("🟡Средний");
    }
    if (priority_value == "red") {
        return QStringLiteral("🔴Высокий");
    }
    return QStringLiteral("Не установлен");
}
} // namespace

TaskEditScreen::TaskEditScreen(int task_id, int board_id, int status_id, QWidget* parent)
    : QWidget(parent)
    , task_id_(task_id)
    , board_id_(board_id)
    , status_id_(status_id) {
    setupLayout();
}

void TaskEditScreen::setNetworkManager(NetworkManager* manager) {
    network_manager_ = manager;
}

void TaskEditScreen::setSyncCoordinator(SyncCoordinator* coordinator) {
    sync_coordinator_ = coordinator;
}

void TaskEditScreen::setDatabase(QSqlDatabase db) {
    db_ = db;
}

void TaskEditScreen::setTaskId(int task_id) {
    task_id_ = task_id;
}

void TaskEditScreen::setBoardId(int board_id) {
    board_id_ = board_id;
}

void TaskEditScreen::setStatusId(int status_id) {
    status_id_ = status_id;
}

void TaskEditScreen::loadTaskData() {
    clearErrorMessage();

    if (task_id_ <= 0) {
        return;
    }

    LocalTaskRepository repo(db_);
    const auto task = repo.findById(task_id_);
    if (task) {
        title_input_->setText(task->title_);
        description_input_->setPlainText(task->description_);
        priority_combo_->setCurrentText(priorityLabelForValue(task->priority_color_));

        if (!task->deadline_.isEmpty()) {
            QDateTime deadline = QDateTime::fromString(task->deadline_, Qt::ISODate);
            if (deadline.isValid()) {
                QDateTime display_deadline = deadline.addSecs(3 * 3600);
                deadline_input_->setDateTime(display_deadline);
                deadline_checkbox_->setChecked(true);
                deadline_input_->setEnabled(true);
            } else {
                deadline_input_->clear();
                deadline_checkbox_->setChecked(false);
                deadline_input_->setEnabled(false);
            }
        } else {
            deadline_input_->clear();
            deadline_checkbox_->setChecked(false);
            deadline_input_->setEnabled(false);
        }
    }
}

void TaskEditScreen::onUpdateTaskRequest() {
    if (task_id_ <= 0 || !sync_coordinator_) {
        return;
    }

    clearErrorMessage();

    QString title = title_input_->text().trimmed();
    QString description = description_input_->toPlainText().trimmed();
    QString priority_color = priority_combo_->currentText();
    QDateTime deadline = deadline_input_->dateTime();

    const QString validation_error = ValidationUtils::validateTaskFields(title, description);
    if (!validation_error.isEmpty()) {
        showErrorMessage(validation_error);
        return;
    }

    LocalTaskRepository repo(db_);
    const auto existing = repo.findById(task_id_);
    if (!existing) {
        return;
    }

    LocalTask task = *existing;
    task.title_ = title;
    task.description_ = description;
    if (priority_color == "🟢Низкий") {
        task.priority_color_ = "green";
    } else if (priority_color == "🟡Средний") {
        task.priority_color_ = "yellow";
    } else if (priority_color == "🔴Высокий") {
        task.priority_color_ = "red";
    } else {
        task.priority_color_ = "none";
    }

    if (deadline_checkbox_->isChecked() && deadline.isValid()) {
        task.deadline_ = deadline.toUTC().toString(Qt::ISODate);
    } else {
        task.deadline_ = QString();
    }

    task.sync_status_ = SyncStatus::PENDING;
    try {
        repo.save(task);
        clearErrorMessage();
        emit taskUpdated();
        sync_coordinator_->syncTasks();
    } catch (const std::exception& e) {
        qDebug() << "TaskEditScreen: Ошибка сохранения в локальную БД:" << e.what();
        showErrorMessage(kLocalSaveErrorMessage);
    }
}

void TaskEditScreen::onCloseRequest() {
    clearErrorMessage();
    emit closeRequested();
}

void TaskEditScreen::onDeadlineCheckChanged(Qt::CheckState state) {
    deadline_input_->setEnabled(state == Qt::Checked);
}

void TaskEditScreen::showErrorMessage(const QString& message) {
    if (!error_label_) {
        return;
    }

    const QString trimmed_message = message.trimmed();
    if (trimmed_message.isEmpty()) {
        clearErrorMessage();
        return;
    }

    error_label_->setText(trimmed_message);
    error_label_->show();
}

void TaskEditScreen::clearErrorMessage() {
    if (!error_label_) {
        return;
    }

    error_label_->clear();
    error_label_->hide();
}

void TaskEditScreen::setupLayout() {
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

    title_label_ = new QLabel("Редактировать задачу", this);
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

    error_label_ = new QLabel(this);
    error_label_->setWordWrap(true);
    error_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    error_label_->setStyleSheet(kInlineErrorStyle);
    error_label_->hide();
    main_layout->addWidget(error_label_);

    auto* button_layout = new QHBoxLayout();
    button_layout->setSpacing(10);

    cancel_button_ = new QPushButton("Отмена", this);
    cancel_button_->setMinimumHeight(45);
    cancel_button_->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #333333; border-radius: 10px; "
        "font-weight: bold; font-size: 15px; }"
        "QPushButton:hover { background-color: #D0D0D0; }");

    update_button_ = new QPushButton("Сохранить", this);
    update_button_->setMinimumHeight(45);
    update_button_->setStyleSheet(
        "QPushButton { background-color: #305CDE; color: white; border-radius: 10px; "
        "font-weight: bold; font-size: 15px; }"
        "QPushButton:hover { background-color: #2549B3; }");

    button_layout->addWidget(cancel_button_);
    button_layout->addWidget(update_button_);
    main_layout->addLayout(button_layout);

    main_layout->addStretch();

    connect(update_button_, &QPushButton::clicked, this, &TaskEditScreen::onUpdateTaskRequest);
    connect(cancel_button_, &QPushButton::clicked, this, &TaskEditScreen::onCloseRequest);
    connect(close_button_, &QPushButton::clicked, this, &TaskEditScreen::onCloseRequest);
    connect(deadline_checkbox_, &QCheckBox::checkStateChanged, this,
            &TaskEditScreen::onDeadlineCheckChanged);
    connect(title_input_, &QLineEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(description_input_, &QTextEdit::textChanged, this, [this]() { clearErrorMessage(); });
    connect(priority_combo_, &QComboBox::currentTextChanged, this,
            [this](const QString&) { clearErrorMessage(); });
    connect(deadline_checkbox_, &QCheckBox::checkStateChanged, this,
            [this](Qt::CheckState) { clearErrorMessage(); });
    connect(deadline_input_, &QDateTimeEdit::dateTimeChanged, this,
            [this](const QDateTime&) { clearErrorMessage(); });
}
