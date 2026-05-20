from http.client import responses

import pytest
import requests

#TODO:Занести user-а в базу:
# "email": "user@example.com",
# "password": "12345678"
def test_login_success(login_url, session):
    response = session.post(
        login_url,
        json={
            "email": "user@example.com",
            "password": "12345678"
        },
    )

    assert response.status_code == 200

    body = response.json()
    assert "data" in body
    assert "token" in body["data"]
    assert  isinstance(body["data"]["token"], str)
    assert  len(body["data"]["token"]) != 0

    assert "user" in body["data"]

    assert "id" in body["data"]["user"]
    assert isinstance(body["data"]["user"]["id"], int)

    assert "email" in body["data"]["user"]
    assert body["data"]["user"]["email"] == "user@example.com"

    assert "name" in body["data"]["user"]
    assert isinstance(body["data"]["user"]["name"], str)

    assert "status" in body["data"]["user"]
    assert isinstance(body["data"]["user"]["status"], str)

    assert "created_at" in body["data"]["user"]
    assert isinstance(body["data"]["user"]["created_at"], str)






def test_login_wrong_password(login_url, session):
    response = session.post(
        login_url,
        json={
            "email": "user@example.com",
            "password": "wrong_password"
        }
    )

    assert response.status_code == 401

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "UNAUTHORIZED"

def test_login_empty_email(login_url, session):
    response = session.post(
        login_url,
        json={
            "password": "12345678"
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "missing_fields" in body["error"]["details"]
    assert body["error"]["details"]["missing_fields"] == ["email"]

def test_login_empty_password(login_url, session):
    response = session.post(
        login_url,
        json={
            "email": "user@example.com"
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "missing_fields" in body["error"]["details"]
    assert body["error"]["details"]["missing_fields"] == ["password"]

import pytest


@pytest.mark.parametrize(
    "my_json, invalid_field",
    [
        ({"email": 123, "password": "12345678"}, "email"),
        ({"email": 12.5, "password": "12345678"}, "email"),
        ({"email": True, "password": "12345678"}, "email"),
        ({"email": None, "password": "12345678"}, "email"),
        ({"email": [], "password": "12345678"}, "email"),
        ({"email": ["user@example.com"], "password": "12345678"}, "email"),
        ({"email": {}, "password": "12345678"}, "email"),
        ({"email": {"value": "user@example.com"}, "password": "12345678"}, "email"),

        ({"email": "user@example.com", "password": 12345678}, "password"),
        ({"email": "user@example.com", "password": 12.5}, "password"),
        ({"email": "user@example.com", "password": True}, "password"),
        ({"email": "user@example.com", "password": None}, "password"),
        ({"email": "user@example.com", "password": []}, "password"),
        ({"email": "user@example.com", "password": ["12345678"]}, "password"),
        ({"email": "user@example.com", "password": {}}, "password"),
        ({"email": "user@example.com", "password": {"value": "12345678"}}, "password"),
    ],
)
def test_login_invalid_data(my_json, invalid_field, login_url, session):
    response = session.post(
        login_url,
        json=my_json,
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert body["error"]["code"] == "INVALID_FORMAT"

def test_login_empty_data(login_url, session):
    response = session.post(
        login_url,
        json={}
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "missing_fields" in body["error"]["details"]
    assert len(body["error"]["details"]["missing_fields"]) == 2
    assert "password" in body["error"]["details"]["missing_fields"]
    assert  "email" in body["error"]["details"]["missing_fields"]




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
def test_login_invalid_email_format(email, login_url, session):
    response = session.post(
        login_url,
        json={
            "email": email,
            "password": "12345678"
        }
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "email" in body["error"]["details"]


def test_login_specific_password(login_url, session):
    response = session.post(
        login_url,
        json = {
            "email": "user@example.com",
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

@pytest.mark.parametrize("password", ["1234567"[:i] for i in range(1, 8)])
def test_login_invalid_password_format(password, login_url, session):
    response = session.post(
        login_url,
        json={
            "email": "user@example.com",
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

@pytest.mark.parametrize("json_test", [
    '{"email": "user@example.com", "password": "12345678"',
    '{"email": "user@example.com" "password": "12345678"}',
    '{"email": "user@example.com", "password": }',
    "{'email': 'user@example.com', 'password': '12345678'}",
    'just string',
    '""',
    '[]'
])
def test_login_invalid_json(json_test, login_url, session):
    response = session.post(
        login_url,
        data=json_test,
        headers={"Content-Type": "application/json"}
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "INVALID_FORMAT"


def test_login_nonexistent_user(login_url, session):
    response = session.post(
        login_url,
        json={
            "email": "non_exist@example.com",
            "password": "12345678"
        }
    )

    assert response.status_code == 401

    body = response.json()
    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["code"] == "UNAUTHORIZED"
