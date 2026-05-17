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
    assert "user" in body["data"]
    assert body["data"]["user"]["email"] == "user@example.com"


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
    assert body["error"]["message"] == "UNAUTHORIZED"

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
    assert body["error"]["message"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "field" in body["error"]["details"]
    assert body["error"]["details"]["field"] == "email"

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
    assert body["error"]["message"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "field" in body["error"]["details"]
    assert body["error"]["details"]["field"] == "password"

def test_login_empty_data(login_url, session):
    response = session.post(
        login_url,
        json={}
    )

    assert response.status_code == 400

    body = response.json()

    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["message"] == "MISSING_FIELD"

    assert "details" in body["error"]
    assert "field" in body["error"]["details"]
    assert body["error"]["details"]["field"] == "password"
    assert body["error"]["details"]["field"] == "email"



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
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghxml@example.com"
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
    assert body["error"]["message"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "field" in body["error"]["details"]
    assert body["error"]["details"]["field"] == "email"



@pytest.mark.parametrize("password", ["1234567"[:i] for i in range(7)])
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
    assert body["error"]["message"] == "VALIDATION_ERROR"

    assert "details" in body["error"]
    assert "field" in body["error"]["details"]
    assert body["error"]["details"]["field"] == "password"

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
    assert body["error"]["message"] == "INVALID_FORMAT"


def test_login_nonexistent_user(login_url, session):
    response = session.post(
        login_url,
        json={
            "email": "non_exist@example.com",
            "password": "12345678"
        }
    )

    assert response.status_code == 404

    body = response.json()
    assert "error" in body
    assert "message" in body["error"]
    assert body["error"]["message"] == "USER_NOT_FOUND"
