#include "local_db.h"

#include <QDebug>
#include <QFile>
#include <QUuid>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

LocalDatabaseManager::LocalDatabaseManager(QWidget* parent)
    : QObject(parent) {
    QUuid uuid;
    connection_name_ = uuid.createUuid().toString();
}

bool LocalDatabaseManager::open(const QString& db_name) {
    db_ = QSqlDatabase::addDatabase("QSQLITE", connection_name_);
    db_.setDatabaseName(db_name);

    if (!db_.open()) {
        qDebug() << "Local DB:" << db_.lastError().text();
        return false;
    }

    qDebug() << "Local DB: database created succesfully: " << db_name;
    return true;
}

void LocalDatabaseManager::close() {
    if (db_.isOpen()) {
        db_.close();
    }
}

bool LocalDatabaseManager::createDb(const QString& sql_file_path) {
    if (!db_.isOpen()) {
        qDebug() << "Local DB: Database is not open";
        return false;
    }

    QFile file(sql_file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Local DB: Cannot open SQL file" << sql_file_path;
        return false;
    }

    QString sql_script = file.readAll();
    file.close();

    QStringList queries = sql_script.split(';', Qt::SkipEmptyParts);

    for (QString query_text : queries) {
        query_text = query_text.trimmed();
        if (query_text.isEmpty()) {
            continue;
        }

        QSqlQuery query(db_);
        if (!query.exec(query_text)) {
            qDebug() << "Local DB: SQL error " << query.lastError().text();
            qDebug() << "Failed query:" << query_text;
            return false;
        }
    }

    qDebug() << "Local DB: schema created successfully";
    return true;
}
