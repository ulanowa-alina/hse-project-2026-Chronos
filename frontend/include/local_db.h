#ifndef LOCAL_DB_H
#define LOCAL_DB_H

#include <QObject>
#include <QString>
#include <QWidget>
#include <QtSql/QSqlDatabase>

class LocalDatabaseManager : public QObject {
    Q_OBJECT

  public:
    explicit LocalDatabaseManager(QWidget* parent = nullptr);

    bool open(const QString& db_name);
    void close();
    bool createDb(const QString& sql_file_path);
    QSqlDatabase getDatabase() const {
        return db_;
    }

  private:
    QString connection_name_;
    QSqlDatabase db_;
};

#endif // LOCAL_DB_H
