#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <iostream>
#include <pqxx/pqxx>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Chronos");
    window.resize(400, 200);

    QVBoxLayout* layout = new QVBoxLayout(&window);

    pqxx::connection c(
        "host=localhost port=5432 user=postgres password=mysecretpassword dbname=chronos_db");

    QLabel* labelTitle = new QLabel("<h2>Hello World!</h2>");

    layout->addWidget(labelTitle);

    window.show();
    return app.exec();
}
