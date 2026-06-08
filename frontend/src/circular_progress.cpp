#include "circular_progress.h"

CircularProgress::CircularProgress(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(220, 220);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    timer_label_ = new QLabel("30:00");
    timer_label_->setStyleSheet("font-size: 52px; font-weight: bold; color: #172b4d;");
    timer_label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(timer_label_);
}

void CircularProgress::setValue(int value) {
    value_ = qBound(0, value, 100);
    update();
}

void CircularProgress::setColor(const QColor& color) {
    color_ = color;
    update();
}

void CircularProgress::setBackgroundColor(const QColor& color) {
    background_color_ = color;
    update();
}

void CircularProgress::setTimerText(const QString& text) {
    timer_label_->setText(text);
}

void CircularProgress::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    painter.setViewport((width() - side) / 2, (height() - side) / 2, side, side);
    painter.setWindow(0, 0, 220, 220);

    QPen pen;
    pen.setWidth(12);
    pen.setCapStyle(Qt::RoundCap);

    pen.setColor(background_color_);
    painter.setPen(pen);
    painter.drawArc(20, 20, 180, 180, 0, 360 * 16);

    pen.setColor(color_);
    painter.setPen(pen);
    int start_angle = 90 * 16;
    int span_angle = -(value_ * 360 * 16) / 100;
    painter.drawArc(20, 20, 180, 180, start_angle, span_angle);
}

QSize CircularProgress::sizeHint() const {
    return QSize(220, 220);
}
