#ifndef REPOSITORY_HELPERS_HPP
#define REPOSITORY_HELPERS_HPP

#include <QDateTime>
#include <QString>

inline QString processingTimestamp(const QString& value) {
    return value.isEmpty() ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate) : value;
}

#endif // REPOSITORY_HELPERS_HPP
