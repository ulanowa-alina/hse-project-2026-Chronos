#ifndef SYNC_MANAGER_HPP
#define SYNC_MANAGER_HPP

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class SyncManager {
  public:
    virtual ~SyncManager() = default;

    virtual void sync() = 0;
    virtual void parse(const QJsonArray& data) = 0;
    virtual void parseObject(const QJsonObject& data) = 0;
    virtual void load() = 0;

    virtual QString modelName() const = 0;
    virtual QString loadEndpoint() const = 0;
    virtual bool handlesLoadEndpoint(const QString& endpoint) const = 0;
    virtual bool handlePushResponse(const QString& endpoint, const QByteArray& data, int code,
                                    int local_id, const QString& operation) = 0;
};

#endif // SYNC_MANAGER_HPP
