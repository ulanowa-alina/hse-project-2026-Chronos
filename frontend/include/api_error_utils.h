#ifndef API_ERROR_UTILS_H
#define API_ERROR_UTILS_H

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QString>

namespace ApiErrorUtils {

inline auto fallbackErrorMessage() -> QString {
    return QStringLiteral("Не удалось обработать запрос. Попробуйте еще раз.");
}

inline auto fieldDisplayName(const QString& field) -> QString {
    if (field == QStringLiteral("email")) {
        return QStringLiteral("email");
    }
    if (field == QStringLiteral("password")) {
        return QStringLiteral("пароль");
    }
    if (field == QStringLiteral("name")) {
        return QStringLiteral("название");
    }
    if (field == QStringLiteral("status")) {
        return QStringLiteral("статус");
    }
    if (field == QStringLiteral("title")) {
        return QStringLiteral("название");
    }
    if (field == QStringLiteral("description")) {
        return QStringLiteral("описание");
    }
    if (field == QStringLiteral("board_id")) {
        return QStringLiteral("доска");
    }
    if (field == QStringLiteral("status_id")) {
        return QStringLiteral("статус");
    }
    if (field == QStringLiteral("task_id")) {
        return QStringLiteral("задача");
    }
    if (field == QStringLiteral("position")) {
        return QStringLiteral("позиция");
    }
    if (field == QStringLiteral("priority_color")) {
        return QStringLiteral("приоритет");
    }
    if (field == QStringLiteral("deadline")) {
        return QStringLiteral("дедлайн");
    }
    if (field == QStringLiteral("avatar_s3_key")) {
        return QStringLiteral("аватар");
    }
    if (field == QStringLiteral("file_name")) {
        return QStringLiteral("имя файла");
    }
    if (field == QStringLiteral("content_type")) {
        return QStringLiteral("тип файла");
    }
    if (field == QStringLiteral("file_base64")) {
        return QStringLiteral("файл");
    }
    return field;
}

inline auto translateRawMessage(const QString& raw_message,
                                const QString& field = QString()) -> QString {
    const QString message = raw_message.trimmed();
    if (message.isEmpty()) {
        return {};
    }

    if (message == QStringLiteral("already exists")) {
        if (field == QStringLiteral("email")) {
            return QStringLiteral("Пользователь с таким email уже существует.");
        }
        if (field == QStringLiteral("name")) {
            return QStringLiteral("Статус с таким названием уже существует.");
        }
        return QStringLiteral("Такая запись уже существует.");
    }

    if (message == QStringLiteral("Invalid email or password")) {
        return QStringLiteral("Неверный email или пароль.");
    }
    if (message == QStringLiteral("Invalid email format")) {
        return QStringLiteral("Введите корректный email.");
    }
    if (message == QStringLiteral("Invalid password format")) {
        return QStringLiteral("Введите корректный пароль.");
    }
    if (message == QStringLiteral("Invalid name format")) {
        return field == QStringLiteral("name") ? QStringLiteral("Введите корректное имя.")
                                               : QStringLiteral("Введите корректное название.");
    }
    if (message == QStringLiteral("Invalid status format")) {
        return QStringLiteral("Введите корректный статус.");
    }
    if (message == QStringLiteral("Invalid avatar_s3_key format")) {
        return QStringLiteral("Некорректный формат аватара.");
    }
    if (message == QStringLiteral("Invalid file_name format")) {
        return QStringLiteral("Некорректное имя файла.");
    }
    if (message == QStringLiteral("Invalid content_type format")) {
        return QStringLiteral("Некорректный тип файла.");
    }
    if (message == QStringLiteral("Invalid file_base64 format")) {
        return QStringLiteral("Некорректный формат файла.");
    }
    if (message == QStringLiteral("Invalid base64 file payload")) {
        return QStringLiteral("Не удалось обработать содержимое файла.");
    }
    if (message == QStringLiteral("Password cannot be empty")) {
        return QStringLiteral("Пароль не может быть пустым.");
    }
    if (message == QStringLiteral("Password length cannot be less than 8 symbols")) {
        return QStringLiteral("Пароль должен содержать не менее 8 символов.");
    }
    if (message == QStringLiteral("Status cannot be empty")) {
        return QStringLiteral("Статус не может быть пустым.");
    }
    if (message == QStringLiteral("File name cannot be empty")) {
        return QStringLiteral("Имя файла не может быть пустым.");
    }
    if (message == QStringLiteral("Content type cannot be empty")) {
        return QStringLiteral("Тип файла не может быть пустым.");
    }
    if (message == QStringLiteral("File content cannot be empty")) {
        return QStringLiteral("Файл не может быть пустым.");
    }
    if (message == QStringLiteral("Name length must be between 1 and 50 symbols")) {
        return field == QStringLiteral("name")
                   ? QStringLiteral("Длина имени должна быть от 1 до 50 символов.")
                   : QStringLiteral("Длина названия должна быть от 1 до 50 символов.");
    }
    if (message == QStringLiteral("Position must be greater than or equal to 0")) {
        return QStringLiteral("Позиция должна быть больше или равна 0.");
    }
    if (message == QStringLiteral("Board id must be positive")) {
        return QStringLiteral("Идентификатор доски должен быть положительным.");
    }
    if (message == QStringLiteral("Status with this name already exists")) {
        return QStringLiteral("Статус с таким названием уже существует.");
    }
    if (message == QStringLiteral("User with this email already exists")) {
        return QStringLiteral("Пользователь с таким email уже существует.");
    }
    if (message == QStringLiteral("Missing required field")) {
        return QStringLiteral("Обязательное поле \"%1\" не заполнено.")
            .arg(fieldDisplayName(field));
    }
    if (message == QStringLiteral("Missing required fields")) {
        return QStringLiteral("Не заполнены обязательные поля.");
    }
    if (message == QStringLiteral("Invalid JSON format")) {
        return QStringLiteral("Некорректный формат данных.");
    }
    if (message == QStringLiteral("Invalid field format")) {
        return QStringLiteral("Некорректный формат поля \"%1\".").arg(fieldDisplayName(field));
    }
    if (message == QStringLiteral("Invalid field value")) {
        return QStringLiteral("Некорректное значение поля \"%1\".").arg(fieldDisplayName(field));
    }
    if (message == QStringLiteral("Validation failed")) {
        return QStringLiteral("Проверьте корректность введенных данных.");
    }
    if (message == QStringLiteral("Board not found")) {
        return QStringLiteral("Доска не найдена.");
    }
    if (message == QStringLiteral("Task not found")) {
        return QStringLiteral("Задача не найдена.");
    }
    if (message == QStringLiteral("Status not found")) {
        return QStringLiteral("Статус не найден.");
    }
    if (message == QStringLiteral("User not found")) {
        return QStringLiteral("Пользователь не найден.");
    }
    if (message == QStringLiteral("Resource belongs to another user")) {
        return QStringLiteral("Ресурс принадлежит другому пользователю.");
    }
    if (message == QStringLiteral("Method not allowed")) {
        return QStringLiteral("Метод запроса не поддерживается.");
    }
    if (message == QStringLiteral("Invalid token")) {
        return QStringLiteral("Сессия истекла. Войдите снова.");
    }
    if (message == QStringLiteral("Unauthorized")) {
        return QStringLiteral("Требуется авторизация.");
    }
    if (message == QStringLiteral("Database error")) {
        return QStringLiteral("Ошибка базы данных. Попробуйте еще раз.");
    }
    if (message == QStringLiteral("Internal error") ||
        message == QStringLiteral("Internal server error")) {
        return QStringLiteral("Внутренняя ошибка сервера. Попробуйте еще раз.");
    }

    if (message.startsWith(QStringLiteral("Invalid ")) &&
        message.endsWith(QStringLiteral(" format"))) {
        QString raw_field = message;
        raw_field.remove(0, QStringLiteral("Invalid ").size());
        raw_field.chop(QStringLiteral(" format").size());
        return QStringLiteral("Некорректный формат поля \"%1\".").arg(fieldDisplayName(raw_field));
    }

    return message;
}

inline auto parseMissingFieldsMessage(const QJsonValue& missing_fields_value) -> QString {
    if (!missing_fields_value.isArray()) {
        return {};
    }

    QStringList fields;
    const QJsonArray fields_array = missing_fields_value.toArray();
    for (const QJsonValue& value : fields_array) {
        if (!value.isString()) {
            continue;
        }
        fields.append(fieldDisplayName(value.toString()));
    }

    if (fields.isEmpty()) {
        return {};
    }

    return QStringLiteral("Не заполнены обязательные поля: %1.").arg(fields.join(", "));
}

inline auto errorCode(const QByteArray& data) -> QString {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return {};
    }
    return doc.object().value("error").toObject().value("code").toString().trimmed();
}

inline auto isDuplicateFieldError(const QByteArray& data, const QString& field) -> bool {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    const QJsonObject error_obj = doc.object().value("error").toObject();
    const QJsonObject details_obj = error_obj.value("details").toObject();
    return error_obj.value("code").toString() == QStringLiteral("DUPLICATE_RESOURCE") &&
           details_obj.value(field).toString().trimmed() == QStringLiteral("already exists");
}

inline auto parseApiErrorMessage(const QByteArray& data,
                                 const QString& fallback = fallbackErrorMessage()) -> QString {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return fallback;
    }

    const QJsonObject error_obj = doc.object().value("error").toObject();
    const QJsonObject details_obj = error_obj.value("details").toObject();
    const QString missing_fields_message =
        parseMissingFieldsMessage(details_obj.value("missing_fields"));
    if (!missing_fields_message.isEmpty()) {
        return missing_fields_message;
    }

    for (auto it = details_obj.constBegin(); it != details_obj.constEnd(); ++it) {
        if (it.value().isString()) {
            const QString detail_message = translateRawMessage(it.value().toString(), it.key());
            if (!detail_message.isEmpty()) {
                return detail_message;
            }
        }
    }

    const QString message = translateRawMessage(error_obj.value("message").toString());
    if (!message.isEmpty()) {
        return message;
    }

    return fallback;
}

} // namespace ApiErrorUtils

#endif // API_ERROR_UTILS_H
