#ifndef VALIDATION_UTILS_H
#define VALIDATION_UTILS_H

#include <QRegularExpression>
#include <QString>

namespace ValidationUtils {

constexpr int USER_NAME_MAX_LENGTH = 50;
constexpr int USER_STATUS_MAX_LENGTH = 255;
constexpr int PASSWORD_MIN_LENGTH = 8;
constexpr int BOARD_TITLE_MAX_LENGTH = 100;
constexpr int BOARD_DESCRIPTION_MAX_LENGTH = 1000;
constexpr int STATUS_NAME_MAX_LENGTH = 50;
constexpr int TASK_TITLE_MAX_LENGTH = 100;
constexpr int TASK_DESCRIPTION_MAX_LENGTH = 1000;

inline auto fillFieldMessage(const QString& field_name) -> QString {
    return QStringLiteral("Заполните поле %1.").arg(field_name);
}

inline auto maxLengthMessage(const QString& field_name, int max_length) -> QString {
    return QStringLiteral("Длина %1 должна быть не больше %2 символов.")
        .arg(field_name)
        .arg(max_length);
}

inline auto rangeLengthMessage(const QString& field_name, int min_length, int max_length)
    -> QString {
    return QStringLiteral("Длина %1 должна быть от %2 до %3 символов.")
        .arg(field_name)
        .arg(min_length)
        .arg(max_length);
}

inline auto isValidEmail(const QString& email) -> bool {
    static const QRegularExpression email_regex(
        QStringLiteral(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)"));
    return email_regex.match(email.trimmed()).hasMatch();
}

inline auto validateUserFields(const QString& name, const QString& email, const QString& status,
                               const QString& password = QString(), bool require_password = false)
    -> QString {
    if (name.trimmed().isEmpty()) {
        return fillFieldMessage(QStringLiteral("Имя"));
    }
    if (email.trimmed().isEmpty()) {
        return fillFieldMessage(QStringLiteral("Email"));
    }
    if (status.trimmed().isEmpty()) {
        return fillFieldMessage(QStringLiteral("Статус"));
    }
    if (require_password && password.isEmpty()) {
        return fillFieldMessage(QStringLiteral("Пароль"));
    }

    if (name.trimmed().length() > USER_NAME_MAX_LENGTH) {
        return rangeLengthMessage(QStringLiteral("имени"), 1, USER_NAME_MAX_LENGTH);
    }
    if (!isValidEmail(email)) {
        return QStringLiteral("Введите корректный email.");
    }
    if (status.trimmed().length() > USER_STATUS_MAX_LENGTH) {
        return maxLengthMessage(QStringLiteral("статуса"), USER_STATUS_MAX_LENGTH);
    }
    if (!password.isEmpty() && password.length() < PASSWORD_MIN_LENGTH) {
        return QStringLiteral("Пароль должен содержать не менее %1 символов.")
            .arg(PASSWORD_MIN_LENGTH);
    }

    return {};
}

inline auto validateBoardFields(const QString& title, const QString& description) -> QString {
    if (title.trimmed().isEmpty()) {
        return QStringLiteral("Введите название доски.");
    }
    if (title.trimmed().length() > BOARD_TITLE_MAX_LENGTH) {
        return rangeLengthMessage(QStringLiteral("названия доски"), 1, BOARD_TITLE_MAX_LENGTH);
    }
    if (description.trimmed().length() > BOARD_DESCRIPTION_MAX_LENGTH) {
        return maxLengthMessage(QStringLiteral("описания доски"), BOARD_DESCRIPTION_MAX_LENGTH);
    }
    return {};
}

inline auto validateStatusName(const QString& name) -> QString {
    if (name.trimmed().isEmpty()) {
        return QStringLiteral("Введите название статуса.");
    }
    if (name.trimmed().length() > STATUS_NAME_MAX_LENGTH) {
        return rangeLengthMessage(QStringLiteral("названия статуса"), 1, STATUS_NAME_MAX_LENGTH);
    }
    return {};
}

inline auto validateTaskFields(const QString& title, const QString& description) -> QString {
    if (title.trimmed().isEmpty()) {
        return QStringLiteral("Введите название задачи.");
    }
    if (title.trimmed().length() > TASK_TITLE_MAX_LENGTH) {
        return rangeLengthMessage(QStringLiteral("названия задачи"), 1, TASK_TITLE_MAX_LENGTH);
    }
    if (description.trimmed().length() > TASK_DESCRIPTION_MAX_LENGTH) {
        return maxLengthMessage(QStringLiteral("описания задачи"), TASK_DESCRIPTION_MAX_LENGTH);
    }
    return {};
}

} // namespace ValidationUtils

#endif // VALIDATION_UTILS_H
