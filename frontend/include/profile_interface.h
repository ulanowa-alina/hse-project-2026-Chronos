#ifndef PROFILE_INTERFACE_H
#define PROFILE_INTERFACE_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ProfileInterface : public QMainWindow {
    Q_OBJECT

  public:
    explicit ProfileInterface(QWidget *parent = nullptr);

  private:
    QWidget *central_widget_;

    QPushButton *edit_button_;
    QPushButton *logout_button_;

    QLabel *avatar_label_;
    QLabel *name_label_;
    QLabel *status_label_;
    QLabel *email_label_;
    QLabel *logo_label_;

    void setupLayout();
};

#endif //PROFILE_INTERFACE_H