#ifndef BLUE_OVAL_HPP
#define BLUE_OVAL_HPP

#include <QGraphicsBlurEffect>
#include <QPainter>
#include <QRadialGradient>
#include <QWidget>

class BlueOval : public QWidget {
  public:
    BlueOval(QWidget* parent, QColor color, int blurRadius);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QColor oval_color_;
};
#endif // BLUE_OVAL_HPP
