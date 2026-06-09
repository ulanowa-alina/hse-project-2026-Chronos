#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"
EXAMPLE_FILE="$SCRIPT_DIR/.env.example"

if [ -f "$ENV_FILE" ]; then
    echo ".env already exists, skipping generation"
    exit 0
fi

if [ ! -f "$EXAMPLE_FILE" ]; then
    echo "Missing .env.example"
    exit 1
fi

if ! command -v openssl >/dev/null 2>&1; then
    echo "openssl is required to generate JWT_SECRET"
    exit 1
fi

JWT_SECRET="$(openssl rand -hex 32)"
DB_PASSWORD="$(openssl rand -hex 16)"
TMP_FILE="$(mktemp)"

awk -v jwt_secret="$JWT_SECRET" -v db_password="$DB_PASSWORD" '
    /^JWT_SECRET=/ {
        print "JWT_SECRET=" jwt_secret
        next
    }
    /^DB_PASSWORD=/ {
        print "DB_PASSWORD=" db_password
        next
    }
    { print }
' "$EXAMPLE_FILE" > "$TMP_FILE"

mv "$TMP_FILE" "$ENV_FILE"

echo ".env created successfully"
echo "JWT_SECRET was generated automatically"
