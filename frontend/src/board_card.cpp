#include "board_card.hpp"

#include <QFontMetrics>
#include <QMouseEvent>
#include <algorithm>

BoardCard::BoardCard(int board_id, QWidget* parent)
    : QFrame(parent)
    , board_id_(board_id) {
    setupLayout();
}

void BoardCard::setBoardData(const QString& title, const QString& description, int active_tasks,
                             int completed_tasks, const QDateTime& nearest_deadline) {
    title_label_->setText(title);

    if (description.trimmed().isEmpty()) {
        description_label_->setText("Описания нет. Нажмите, чтобы открыть доску.");
        description_label_->setStyleSheet(
            "background: transparent; "
            "color: #94A3B8; font-style: italic; font-size: 13px; line-height: 18px;");
    } else {
        QFontMetrics metrics(description_label_->font());
        QString elided_text =
            metrics.elidedText(description, Qt::ElideRight, description_label_->width() * 2);
        description_label_->setText(elided_text);
        description_label_->setStyleSheet(
            "background: transparent; "
            "color: #94A3B8; font-style: italic; font-size: 13px; line-height: 18px;");
    }

    int total_tasks = active_tasks + completed_tasks;
    int progress_percent = 0;
    if (total_tasks > 0) {
        progress_percent = (completed_tasks * 100) / total_tasks;
    }

    progress_bar_->setValue(progress_percent);
    progress_text_label_->setText(QString("%1%").arg(progress_percent));

    active_tasks_label_->setText(QString("Активных задач: %1").arg(active_tasks));

    if (!nearest_deadline.isValid() || nearest_deadline.isNull()) {
        deadline_label_->setText("Нет дедлайнов");
    } else {
        QDateTime display_deadline = nearest_deadline.addSecs(3 * 3600);
        QString date_str = display_deadline.toString("dd.MM.yyyy");
        deadline_label_->setText(QString("Ближайший дедлайн: %1").arg(date_str));
    }
}

void BoardCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit openBoardRequested(board_id_);
    }
    QFrame::mousePressEvent(event);
}

void BoardCard::setupLayout() {
    this->setObjectName("boardCard");
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->setFixedSize(280, 180);

    this->setStyleSheet("#boardCard { "
                        "   background: #F4F7FE; "
                        "   border: 1px solid #E2E8F0; "
                        "   border-radius: 16px; "
                        "}"
                        "#boardCard:hover { "
                        "   border: 1px solid #305CDE; "
                        "}"
                        "#boardCard QLabel { "
                        "   background: transparent; "
                        "}");

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(16, 16, 16, 16);
    main_layout->setSpacing(0);

    title_label_ = new QLabel(this);
    title_label_->setStyleSheet("font-weight: bold; font-size: 20px; color: #172B4D;");
    main_layout->addWidget(title_label_);

    main_layout->addSpacing(10);

    description_label_ = new QLabel(this);
    description_label_->setWordWrap(true);
    description_label_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    description_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    main_layout->addWidget(description_label_);

    auto* progress_layout = new QHBoxLayout();
    progress_layout->setSpacing(8);

    progress_bar_ = new QProgressBar(this);
    progress_bar_->setRange(0, 100);
    progress_bar_->setStyleSheet("QProgressBar { "
                                 "   border: none; "
                                 "   background-color: #E2E8F0; "
                                 "   height: 12px; "
                                 "   border-radius: 6px; "
                                 "   color: transparent; "
                                 "}"
                                 "QProgressBar::chunk { "
                                 "   background-color: #305CDE; "
                                 "   border-radius: 6px; "
                                 "}");

    progress_text_label_ = new QLabel("0%", this);
    progress_text_label_->setStyleSheet("font-weight: bold; font-size: 12px; color: #305CDE;");

    progress_layout->addWidget(progress_bar_, 1);
    progress_layout->addWidget(progress_text_label_);
    main_layout->addLayout(progress_layout);

    main_layout->addSpacing(12);

    auto* footer_layout = new QVBoxLayout();
    footer_layout->setSpacing(4);

    active_tasks_label_ = new QLabel(this);
    active_tasks_label_->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 500;");

    deadline_label_ = new QLabel(this);
    deadline_label_->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 500;");
    deadline_label_->setAlignment(Qt::AlignLeft);

    footer_layout->addWidget(active_tasks_label_);
    footer_layout->addWidget(deadline_label_);
    main_layout->addLayout(footer_layout);
}
