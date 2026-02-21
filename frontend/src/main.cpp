#include "profile_interface.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ProfileInterface window;
    window.show();

    return app.exec();
}