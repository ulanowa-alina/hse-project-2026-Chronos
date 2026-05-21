import pytest
import requests
import re

ISO_8601_UTC = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")

def test_register_success(register_url, session, reg_user_data):
    response = session.post(
        register_url,
        json=reg_user_data
    )

    assert response.status_code == 200

    body = response.json()

    assert "data" in body
    assert "id" in body["data"]
    assert isinstance(body["data"]["id"], int)
    assert body["data"]["id"] > 0

    assert "email" in body["data"]
    assert body["data"]["email"] == reg_user_data["email"]

    assert "name" in body["data"]
    assert body["data"]["name"] == reg_user_data["name"]

    assert "status" in body["data"]
    assert body["data"]["status"] == reg_user_data["status"]

    assert "created_at" in body["data"]
    assert isinstance(body["data"]["created_at"], str)
    assert ISO_8601_UTC.match(body["data"]["created_at"])

    assert "password" not in body["data"]




@pytest.mark.parametrize("name", [
    "qwertyuiopasdfghjklzxcvbnmqwertyuiopasdfghjklzxcvbnm",
    ""
])
def test_register_invalid_name(name, register_url, session, reg_user_data):
    response = session.post(
        register_url,
        json={
            "name": name,
            "email": reg_user_data["email"],
            "status": reg_user_data["status"],
            "password": reg_user_data["password"]
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "name" in body["error"]["details"]



@pytest.mark.parametrize("email", [
    "user",
    "@example.com",
    "user@",
    "user@com",
    "user@.com",
    "user@example.",
    "user name@example.com",
    "user@example com",
    "user@exam@ple.com",
    "user#name@example.com",
    "user@example..com",
    ".user@example.com",
    "user.@example.com",
    "ваня@example.com",
    "user@почта.рф",
    " ",
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqaewsredtzsexdrcftvgbhnjkmlmkjnhbgvfcdxszxdcfvgbhnjkmcfyvgubhdcfvgbhnjmjnhbgvcxddcfvgbhnjmkjnhbgvfcdinjmkjhgfdxcfgvhqwertyuioasdfghjkzxcvbnmqwertyuioasdfghjkzxcvbnbjnkmjhgfdtcfvghbjknrstuqwertyuiopasdfghjklzxcvbnmvwxyzabcdefghxml@example.com"
])
def test_register_invalid_email(email, register_url, session, reg_user_data):
    response = session.post(
        register_url,
        json={
            "name": reg_user_data["name"],
            "email": email,
            "status": reg_user_data["status"],
            "password": reg_user_data["password"]
        }
    )
    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "email" in body["error"]["details"]


@pytest.mark.parametrize("password", ["1234567"[:i] for i in range(1, 8)])
def test_register_invalid_password(password, register_url, session, reg_user_data):
    response = session.post(
        register_url,
        json={
            "name": reg_user_data["name"],
            "email": reg_user_data["email"],
            "status": reg_user_data["status"],
            "password": password
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "password" in body["error"]["details"]
    assert body["error"]["details"]["password"] == "Password length cannot be less than 8 symbols"


def test_register_empty_password(register_url, session, reg_user_data):
    response = session.post(
        register_url,
        json = {
            "name": reg_user_data["name"],
            "email": reg_user_data["email"],
            "status": reg_user_data["status"],
            "password": ""
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert  'password' in body["error"]["details"]
    assert  body["error"]["details"]["password"] == "Password cannot be empty"


@pytest.mark.parametrize("status", [
    "",
    "qwertyuiopasdfghjklzxcvbnmqwertyuiopasdfghjklzxcvbnm"
])
def test_register_invalid_status(status, register_url, session, reg_user_data):
    response = session.post(
        register_url,
        json={
            "name": reg_user_data["name"],
            "email": reg_user_data["email"],
            "status": status,
            "password": reg_user_data["password"]
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "status" in body["error"]["details"]


@pytest.mark.parametrize("my_json, invalid_field", [
    ({
         "name": 123,
         "email": "non_existance@example.com",
         "status": "student",
         "password": "password123"
     }, "name"),
    ({
         "name": 12.5,
         "email": "non_existance@example.com",
         "status": "student",
         "password": "password123"
     }, "name"),
    ({
         "name": True,
         "email": "non_existance@example.com",
         "status": "student",
         "password": "password123"
     }, "name"),
    ({
         "name": None,
         "email": "non_existance@example.com",
         "status": "student",
         "password": "password123"
     }, "name"),
    ({
         "name": [],
         "email": "non_existance@example.com",
         "status": "student",
         "password": "password123"
     }, "name"),
    ({
         "name": {},
         "email": "non_existance@example.com",
         "status": "student",
         "password": "password123"
     }, "name"),
    ({
         "name": "user",
         "email": 123,
         "status": "student",
         "password": "password123"
     }, "email"),
    ({
         "name": "user",
         "email": 12.5,
         "status": "student",
         "password": "password123"
     }, "email"),
    ({
         "name": "user",
         "email": True,
         "status": "student",
         "password": "password123"
     }, "email"),
    ({
         "name": "user",
         "email": None,
         "status": "student",
         "password": "password123"
     }, "email"),
    ({
         "name": "user",
         "email": [],
         "status": "student",
         "password": "password123"
     }, "email"),
    ({
         "name": "user",
         "email": {},
         "status": "student",
         "password": "password123"
     }, "email"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": 123,
         "password": "password123"
     }, "status"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": 12.5,
         "password": "password123"
     }, "status"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": True,
         "password": "password123"
     }, "status"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": None,
         "password": "password123"
     }, "status"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": [],
         "password": "password123"
     }, "status"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": {},
         "password": "password123"
     }, "status"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": "student",
         "password": 123
     }, "password"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": "student",
         "password": 12.5
     }, "password"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": "student",
         "password": True
     }, "password"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": "student",
         "password": None
     }, "password"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": "student",
         "password": []
     }, "password"),
    ({
         "name": "user",
         "email": "non_existance@example.com",
         "status": "student",
         "password": {}
     }, "password"),
])
def test_register_invalid_data(my_json, invalid_field, register_url, session):
    response = session.post(
        register_url,
        json=my_json
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "INVALID_FORMAT"

    assert "details" in body["error"]
    assert invalid_field in body["error"]["details"]


@pytest.mark.parametrize("json_test", [
    '{"name": "user", "email": "user@example.com", "status": "student", "password": "password123"',
    '{"name": "user", "email": "user@example.com" "status": "student", "password": "password123"}',
    '{"name": "user", "email": "user@example.com", "status": "student", "password": }',
    "{'name': 'user', 'email': 'user@example.com', 'status': 'student', 'password': 'password123'}",
    '{"name": "user", "email": "user@example.com", "status": "student", "password": "password123",}',
    '{"name": "user",, "email": "user@example.com", "status": "student", "password": "password123"}',
    '{"name": "user", "email": "user@example.com", "status": "student", "password": "password123"}}',
    '[{"name": "user", "email": "user@example.com", "status": "student", "password": "password123"}',
    '{"name": "user", "email": user@example.com, "status": "student", "password": "password123"}',
    '{"name": "user, "email": "user@example.com", "status": "student", "password": "password123"}',
    '{"name": "user", "email": "user@example.com", "status": "student", "password": "password123"',
    '{"name": "user", "email": "user@example.com", "status": "student", "password": "pass' + chr(10) + 'word"}',
    '{"name": "user", \x00 "email": "user@example.com", "status": "student"}',
    '{"name": "user", "email": "user@example.com", "status": "student", "password": "password123" "extra": true}',
    'just string',
    '""',
    '[]',
    '123',
    'true',
    'null',
    '     ',
    '',
    '{"name": "user", "email": "user@example.com", "status": 0123, "password": "password123"}',
    '{"name": "user", "email": "user@example.com", "status": 12., "password": "password123"}',
    '{',
    '{"name":',
    '{"name": "user"'
])
def test_register_invalid_json(json_test, register_url, session):
    response = session.post(
        register_url,
        data=json_test,
        headers={"Content-Type": "application/json"}
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "INVALID_FORMAT"



@pytest.mark.parametrize("my_json", [
    {
        "email": "missing_check@example.com",
        "status": "student",
        "password": "password123"
    },
    {
        "name": "user",
        "status": "student",
        "password": "password123"
    },
    {
        "name": "user",
        "email": "missing_check@example.com",
        "password": "password123"
    },
    {
        "name": "user",
        "email": "missing_check@example.com",
        "status": "student"
    },
    {
        "status": "student",
        "password": "password123"
    },
    {
        "email": "missing_check@example.com",
        "password": "password123"
    },
    {
        "email": "missing_check@example.com",
        "status": "student"
    },
    {
        "name": "user",
        "password": "password123"
    },
    {
        "name": "user",
        "status": "student"
    },
    {
        "name": "user",
        "email": "missing_check@example.com"
    },
    {
        "password": "password123"
    },
    {
        "status": "student"
    },
    {
        "email": "missing_check@example.com"
    },
    {
        "name": "user"
    },
    {}
])
def test_register_missing_field(my_json, register_url, session):
    required = ["name", "email", "status", "password"]
    missed = [s for s in required if s not in my_json]

    response = session.post(
        register_url,
        json=my_json
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "missing_fields" in body["error"]["details"]

    for field in missed:
        assert field in body["error"]["details"]["missing_fields"]


def test_register_existent_email(register_url, session):
    response = session.post(
        register_url,
        json={
            "name": "user",
            "email": "user@example.com",
            "status": "student",
            "password": "password123"
        }
    )

    assert response.status_code == 405

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "EMAIL_ALREADY_EXISTS"
    assert "details" in body["error"]
    assert body["error"]["details"]["email"] == "already exists"
