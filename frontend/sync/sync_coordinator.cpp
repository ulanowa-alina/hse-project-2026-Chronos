#include "sync_coordinator.hpp"

#include "../local_repositories/local_board_repository.hpp"
#include "../local_repositories/local_user_repository.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>

bool codeIsError(int code) {
    return code < 200 || code >= 300;
}

SyncCoordinator::SyncCoordinator(QSqlDatabase& db, NetworkManager* network_manager, QObject* parent)
    : QObject(parent)
    , db_(db)
    , network_manager_(network_manager)
    , user_manager_(std::make_unique<UserSyncManager>(db_, network_manager_))
    , board_manager_(std::make_unique<BoardSyncManager>(db_, network_manager_))
    , status_manager_(std::make_unique<StatusSyncManager>(db_, network_manager_))
    , task_manager_(std::make_unique<TaskSyncManager>(db_, network_manager_)) {
    managers_ = {user_manager_.get(), board_manager_.get(), status_manager_.get(),
                 task_manager_.get()};

    periodic_timer_ = new QTimer(this);
    connect(periodic_timer_, &QTimer::timeout, this, &SyncCoordinator::onPeriodicSync);
}

void SyncCoordinator::clearLocalData() {
    QSqlQuery query(db_);
    query.exec("DELETE FROM tasks");
    query.exec("DELETE FROM statuses");
    query.exec("DELETE FROM boards");
    query.exec("DELETE FROM users");
    current_user_id_ = -1;
}

void SyncCoordinator::beginUserSession(const QJsonObject& user_obj) {
    clearLocalData();
    current_user_id_ = user_obj["id"].toInt();
    user_manager_->saveFromLogin(user_obj);
    loadAll();
}

void SyncCoordinator::loadCurrentUser() {
    LocalUserRepository repo(db_);
    const auto user = repo.getCurrentUser();
    if (user) {
        current_user_id_ = user->id_;
    }
}

void SyncCoordinator::loadAll() {
    if (remote_loading_) {
        return;
    }

    remote_loading_ = true;
    waiting_initial_boards_ = true;
    waiting_initial_children_ = false;
    user_manager_->load();
    board_manager_->load();
}

void SyncCoordinator::loadChildrenAfterBoards() {
    waiting_initial_children_ = true;
    status_manager_->load();
    task_manager_->load();
}

void SyncCoordinator::finishInitialLoadCycle() {
    waiting_initial_boards_ = false;
    waiting_initial_children_ = false;
    remote_loading_ = false;
    emitInitialData();
}

void SyncCoordinator::syncAll() {
    board_manager_->sync();
    status_manager_->sync();
    task_manager_->sync();
    user_manager_->sync();
}

void SyncCoordinator::syncBoards() {
    board_manager_->sync();
}

void SyncCoordinator::syncStatuses() {
    status_manager_->sync();
}

void SyncCoordinator::syncTasks() {
    task_manager_->sync();
}

void SyncCoordinator::syncUsers() {
    user_manager_->sync();
}

void SyncCoordinator::setPassword(const QString& password) {
    user_manager_->setPassword(password);
}

int SyncCoordinator::currentUserId() const {
    return current_user_id_;
}

SyncManager* SyncCoordinator::managerByModel(const QString& entity) const {
    for (SyncManager* manager : managers_) {
        if (manager->modelName() == entity) {
            return manager;
        }
    }
    return nullptr;
}

void SyncCoordinator::handleResponse(const QString& endpoint, const QByteArray& data, int code) {
    const bool is_boards = endpoint == network_manager_->boards_get_all_url_;
    const bool is_statuses = endpoint.startsWith(network_manager_->statuses_get_all_url_);

    if (codeIsError(code)) {
        if (waiting_initial_boards_ && is_boards) {
            finishInitialLoadCycle();
        } else if (waiting_initial_children_ && is_statuses) {
            finishInitialLoadCycle();
        }
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        if (waiting_initial_boards_ && is_boards) {
            finishInitialLoadCycle();
        }
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonValue payload = root["data"];

    for (SyncManager* manager : managers_) {
        if (!manager->handlesLoadEndpoint(endpoint)) {
            continue;
        }

        if (payload.isArray()) {
            manager->parse(payload.toArray());
        } else if (payload.isObject()) {
            manager->parseObject(payload.toObject());
        }
        break;
    }

    if (waiting_initial_boards_ && is_boards) {
        waiting_initial_boards_ = false;
        loadChildrenAfterBoards();
    } else if (waiting_initial_children_ && is_statuses) {
        finishInitialLoadCycle();
    }

    if (!remote_loading_) {
        emit dataChanged();
    }
}

void SyncCoordinator::handleSyncResponse(const QString& endpoint, const QByteArray& data, int code,
                                         const QString& entity, int local_id,
                                         const QString& operation) {
    SyncManager* manager = managerByModel(entity);
    if (!manager) {
        return;
    }

    if (manager->handlePushResponse(endpoint, data, code, local_id, operation)) {
        if (entity == "board") {
            status_manager_->sync();
            task_manager_->sync();
        }
        emit dataChanged();
    }
}

int SyncCoordinator::defaultBoardId() const {
    LocalBoardRepository repo(db_);
    const auto board_id = repo.findFirstBoard();
    return board_id.value_or(-1);
}

bool SyncCoordinator::hasLocalData() const {
    LocalBoardRepository repo(db_);
    return repo.findFirstBoard().has_value();
}

void SyncCoordinator::emitInitialData() {
    emit initialDataReady(defaultBoardId());
}

void SyncCoordinator::startPeriodicSync(int interval_ms) {
    if (periodic_timer_->isActive()) {
        periodic_timer_->stop();
    }
    periodic_timer_->start(interval_ms);
}

void SyncCoordinator::stopPeriodicSync() {
    periodic_timer_->stop();
}

void SyncCoordinator::onPeriodicSync() {
    if (!network_manager_->hasToken() || remote_loading_) {
        return;
    }
    syncAll();
    loadAll();
}
