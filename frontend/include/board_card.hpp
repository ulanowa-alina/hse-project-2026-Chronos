#ifndef BOARD_CARD_HPP
#define BOARD_CARD_HPP

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

class BoardCard : public QFrame {
    Q_OBJECT

  public:
    explicit BoardCard(int board_id, QWidget* parent = nullptr);

    int getBoardId() const {
        return board_id_;
    }

    void setBoardData(const QString& title, const QString& description, int active_tasks,
                      int completed_tasks, const QDateTime& nearest_deadline);

  signals:
    void openBoardRequested(int board_id);

  protected:
    void mousePressEvent(QMouseEvent* event) override;

  private:
    int board_id_;

    QLabel* title_label_{nullptr};
    QLabel* description_label_{nullptr};
    QProgressBar* progress_bar_{nullptr};
    QLabel* progress_text_label_{nullptr};
    QLabel* active_tasks_label_{nullptr};
    QLabel* deadline_label_{nullptr};

    void setupLayout();
};

#endif // BOARD_CARD_HPP
