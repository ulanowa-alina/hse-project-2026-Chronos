#include "blue_oval.hpp"

BlueOval::BlueOval(QWidget* parent, QColor color, int blurRadius)
    : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* blur = new QGraphicsBlurEffect(this);
    blur->setBlurRadius(blurRadius);
    setGraphicsEffect(blur);
    oval_color_ = color;
}

void BlueOval::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QRadialGradient gradient(rect().center(), width() / 2);
    gradient.setColorAt(0, oval_color_);
    gradient.setColorAt(1, QColor(oval_color_.red(), oval_color_.green(), oval_color_.blue(), 0));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect());
}
