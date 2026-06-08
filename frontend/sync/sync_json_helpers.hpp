#ifndef SYNC_JSON_HELPERS_HPP
#define SYNC_JSON_HELPERS_HPP

#include <QDateTime>
#include <QJsonObject>
#include <QString>

inline QString jsonTimestamp(const QJsonObject& obj, const char* field,
                             const char* fallback_field = nullptr) {
    QString value = obj.value(field).toString();
    if (value.isEmpty() && fallback_field != nullptr) {
        value = obj.value(fallback_field).toString();
    }
    if (value.isEmpty()) {
        value = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
    return value;
}

#endif // SYNC_JSON_HELPERS_HPP
