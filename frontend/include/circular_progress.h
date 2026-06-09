#ifndef CIRCULAR_PROGRESS_H
#define CIRCULAR_PROGRESS_H

#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

class CircularProgress : public QWidget {
    Q_OBJECT
  public:
    explicit CircularProgress(QWidget* parent = nullptr);

    void setValue(int value);
    void setColor(const QColor& color);
    void setBackgroundColor(const QColor& color);
    void setTimerText(const QString& text);

  protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override;

  private:
    int value_{100};
    QColor color_{"#305CDE"};
    QColor background_color_{"#f4f5f7"};
    QLabel* timer_label_{nullptr};
};

#endif // CIRCULAR_PROGRESS_H
