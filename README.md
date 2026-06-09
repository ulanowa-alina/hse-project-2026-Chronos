# Chronos

Chronos - desktop-приложение для управления задачами, досками и личной продуктивностью. Проект состоит из Qt-клиента и C++ backend-сервиса с PostgreSQL. Клиент поддерживает локальное хранение данных, синхронизацию с сервером и базовые offline-сценарии.

## Возможности

- регистрация и авторизация пользователей через JWT
- управление профилем пользователя
- загрузка и удаление аватара
- доски задач
- статусы внутри досок
- задачи с описанием, приоритетом и дедлайном
- Pomodoro-сессии
- локальная SQLite-база на клиенте
- синхронизация локальных данных с backend

## Технологический стек

### Backend

- C++17
- Boost.Asio
- PostgreSQL
- libpqxx
- OpenSSL
- libcurl
- spdlog
- nlohmann/json

### Frontend

- C++17
- Qt6 Widgets
- Qt6 Network
- Qt6 Sql
- SQLite

## Структура проекта

```text
.
├── backend/                  # backend-сервис и docker-окружение
│   ├── migrations/           # SQL-инициализация PostgreSQL
│   ├── models/               # модели домена
│   ├── repositories/         # слой доступа к данным
│   ├── src/
│   │   ├── db/               # конфиг БД и connection pool
│   │   ├── security/         # хеширование паролей
│   │   ├── server/           # HTTP API, auth, board/task/status/pomodoro/personal
│   │   └── storage/          # работа с S3/Yandex Object Storage
│   ├── Dockerfile
│   ├── docker-compose.yml
│   └── init-env.sh
├── frontend/                 # Qt desktop-клиент
│   ├── include/              # заголовки UI и инфраструктуры
│   ├── src/                  # экраны, окна и network layer
│   ├── local_models/         # локальные модели клиента
│   ├── local_repositories/   # работа с локальной SQLite
│   ├── sync/                 # синхронизация клиента с backend
│   └── sql/local_init.sql    # схема локальной БД
├── docs/
│   ├── API.md                # описание API
│   └── *.dbml                # схемы БД
└── CMakeLists.txt
```

## Как устроен запуск

- backend запускается отдельно из директории `backend/`
- frontend собирается отдельно из директории `frontend/`
- клиент ожидает backend по адресу `http://127.0.0.1:8080`
- локальная база клиента создается как `frontend/build/chronos_local.db`

## Требования

### Для backend

- Docker
- Docker Compose

### Для frontend

- CMake >= 3.16
- Qt6 с компонентами `Widgets`, `Network`, `Sql`
- C++ компилятор с поддержкой C++17

## Быстрый запуск

### 1. Запуск backend

Перейдите в директорию backend:

```bash
cd backend
```

Сгенерируйте локальный `.env`, если его еще нет:

```bash
./init-env.sh
```

Скрипт:

- создаст `backend/.env` на основе `.env.example`
- автоматически сгенерирует `JWT_SECRET`
- автоматически сгенерирует `DB_PASSWORD`

Поднимите backend и PostgreSQL:

```bash
docker compose up --build
```

Backend будет доступен по адресу:

```text
http://127.0.0.1:8080
```

### Важно: если пересобираете `.env`, очистите БД

`docker-compose.yml` использует persistent volume `postgres_data`, поэтому PostgreSQL не сбрасывается при обычной пересборке контейнеров.

Если вы:

- удалили и заново создали `backend/.env`
- изменили `DB_NAME`, `DB_USER` или `DB_PASSWORD`
- хотите поднять backend с чистой конфигурацией окружения

сначала нужно удалить старое состояние базы:

```bash
docker compose down -v
```

После этого можно заново выполнить:

```bash
./init-env.sh
docker compose up --build
```

Иначе PostgreSQL может остаться со старым состоянием данных и старыми параметрами, из-за чего backend будет работать некорректно.

### 2. Сборка и запуск frontend

Перейдите в директорию frontend:

```bash
cd frontend
```

Сконфигурируйте проект:

```bash
cmake -S . -B build
```

Соберите клиент:

```bash
cmake --build build
```

Запустите клиент:

```bash
cmake --build build --target run
```

## Локальная база клиента

Клиент использует SQLite для локального хранения данных и синхронизации.

При запуске:

- создается файл `chronos_local.db` в директории `frontend/build`
- схема инициализируется из `frontend/sql/local_init.sql`

Это нужно для локального хранения, offline-работы и последующей синхронизации с backend.

## Переменные окружения backend

Основные переменные находятся в `backend/.env`:

```env
DB_HOST=db
DB_PORT=5432
DB_NAME=chronos_db
DB_USER=postgres
DB_PASSWORD=...
DB_POOL_SIZE=10
JWT_SECRET=...
JWT_TTL_SECONDS=86400
S3_BUCKET=chronos
S3_REGION=ru-central1
S3_ENDPOINT=https://storage.yandexcloud.net
S3_USE_PATH_STYLE=true
```

## API

Документация по API находится в файле `docs/API.md`.

Основные группы endpoint'ов:

- `auth` - регистрация и вход
- `personal` - профиль пользователя и аватар
- `board` - доски
- `task` - задачи
- `status` - статусы
- `pomodoro` - Pomodoro-сессии

## Полезные замечания

- backend пишет логи в `backend/logs/server.log`
- frontend в текущем виде ожидает backend на `127.0.0.1:8080`
- PostgreSQL инициализируется через `backend/migrations/init.sql`
- корневой `CMakeLists.txt` сейчас подключает только `frontend`, backend собирается отдельно внутри `backend/` и обычно запускается через Docker
