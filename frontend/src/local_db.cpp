#include "local_db.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

namespace {

QStringList splitSqlStatements(const QString& script) {
    QStringList statements;
    QString current;
    int block_depth = 0;

    for (int i = 0; i < script.size(); ++i) {
        const QChar ch = script[i];

        if (ch == ';' && block_depth == 0) {
            const QString statement = current.trimmed();
            if (!statement.isEmpty()) {
                statements.push_back(statement);
            }
            current.clear();
            continue;
        }

        current += ch;

        if (i + 4 < script.size() && script.mid(i, 5).compare("BEGIN", Qt::CaseInsensitive) == 0) {
            const QChar prev = i > 0 ? script[i - 1] : QChar();
            const QChar next = script[i + 5];
            if (!prev.isLetterOrNumber() && !next.isLetterOrNumber()) {
                ++block_depth;
            }
        } else if (i + 2 < script.size() &&
                   script.mid(i, 3).compare("END", Qt::CaseInsensitive) == 0) {
            const QChar prev = i > 0 ? script[i - 1] : QChar();
            const QChar next = script[i + 3];
            if (!prev.isLetterOrNumber() && !next.isLetterOrNumber() && block_depth > 0) {
                --block_depth;
            }
        }
    }

    const QString tail = current.trimmed();
    if (!tail.isEmpty()) {
        statements.push_back(tail);
    }

    return statements;
}

} // namespace

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

    qDebug() << "Local DB: database created succesfully:"
             << QDir::current().absoluteFilePath(db_name);
    return true;
}

void LocalDatabaseManager::close() {
    if (db_.isOpen()) {
        db_.close();
    }
}

bool LocalDatabaseManager::isOpen() const {
    return db_.isOpen();
}

bool LocalDatabaseManager::hasSchema() const {
    if (!db_.isOpen()) {
        return false;
    }

    QSqlQuery query(db_);
    if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='users'")) {
        return false;
    }

    if (!query.next()) {
        return false;
    }

    QSqlQuery trigger_query(db_);
    if (!trigger_query.exec(
            "SELECT COUNT(*) FROM sqlite_master WHERE type='trigger' AND name LIKE 'trg_%'")) {
        return false;
    }

    if (!trigger_query.next()) {
        return false;
    }

    return trigger_query.value(0).toInt() >= 4;
}

bool LocalDatabaseManager::createSchema(const QString& sql_file_path) {
    if (!db_.isOpen()) {
        qDebug() << "Local DB: Database is not open";
        return false;
    }

    if (hasSchema()) {
        return true;
    }

    return createDb(sql_file_path);
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

    const QString sql_script = file.readAll();
    file.close();

    const QStringList queries = splitSqlStatements(sql_script);

    for (const QString& query_text : queries) {
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
