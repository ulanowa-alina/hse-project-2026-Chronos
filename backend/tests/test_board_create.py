import re
from uuid import uuid4

import pytest


ISO_8601_UTC = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")


def unique_email():
    return f"board-create-{uuid4().hex}@example.com"


def register_user(session, register_url):
    password = "12345678"
    email = unique_email()

    response = session.post(
        register_url,
        json={
            "name": "Board Tester",
            "email": email,
            "status": "student",
            "password": password,
        },
    )

    assert response.status_code == 200
    user = response.json()["data"]

    return {
        "id": user["id"],
        "email": email,
        "password": password,
    }


def login_user(session, login_url, user):
    response = session.post(
        login_url,
        json={
            "email": user["email"],
            "password": user["password"],
        },
    )

    assert response.status_code == 200
    return response.json()["data"]["token"]


def auth_headers(token):
    return {"Authorization": f"Bearer {token}"}


def create_authenticated_user(session, register_url, login_url):
    user = register_user(session, register_url)
    token = login_user(session, login_url, user)
    return user, token


def assert_board_model(
    board,
    expected_user_id,
    expected_title,
    expected_description,
    expected_is_private,
):
    assert set(board.keys()) == {
        "id",
        "user_id",
        "title",
        "description",
        "is_private",
        "created_at",
        "updated_at",
    }

    assert isinstance(board["id"], int)
    assert board["id"] > 0
    assert board["user_id"] == expected_user_id
    assert board["title"] == expected_title
    assert board["description"] == expected_description
    assert board["is_private"] == expected_is_private
    assert ISO_8601_UTC.match(board["created_at"])
    assert ISO_8601_UTC.match(board["updated_at"])


def assert_error(response, status_code, code):
    assert response.status_code == status_code

    body = response.json()
    assert "error" in body
    assert body["error"]["code"] == code


def test_board_create_success_returns_board_model(
    session,
    register_url,
    login_url,
    board_create_url,
):
    user, token = create_authenticated_user(session, register_url, login_url)

    response = session.post(
        board_create_url,
        headers=auth_headers(token),
        json={
            "title": "Study board",
            "description": "Spring semester",
            "is_private": True,
        },
    )

    assert response.status_code == 200
    body = response.json()
    assert "data" in body
    assert_board_model(
        body["data"],
        expected_user_id=user["id"],
        expected_title="Study board",
        expected_description="Spring semester",
        expected_is_private=True,
    )


def test_board_create_without_description_uses_empty_string(
    session,
    register_url,
    login_url,
    board_create_url,
):
    user, token = create_authenticated_user(session, register_url, login_url)

    response = session.post(
        board_create_url,
        headers=auth_headers(token),
        json={
            "title": "Board without description",
            "is_private": False,
        },
    )

    assert response.status_code == 200
    assert_board_model(
        response.json()["data"],
        expected_user_id=user["id"],
        expected_title="Board without description",
        expected_description="",
        expected_is_private=False,
    )


def test_board_create_requires_auth(session, board_create_url):
    response = session.post(
        board_create_url,
        json={
            "title": "Unauthorized board",
            "description": "Should not be created",
            "is_private": False,
        },
    )

    assert_error(response, 401, "UNAUTHORIZED")


@pytest.mark.parametrize(
    "payload, missing_fields",
    [
        ({"is_private": False}, ["title"]),
        ({"title": "Board"}, ["is_private"]),
        ({}, ["title", "is_private"]),
    ],
)
def test_board_create_required_fields(
    payload,
    missing_fields,
    session,
    register_url,
    login_url,
    board_create_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.post(
        board_create_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "MISSING_FIELD")
    assert response.json()["error"]["details"]["missing_fields"] == missing_fields


@pytest.mark.parametrize(
    "payload",
    [
        {"title": "", "description": "Description", "is_private": False},
        {"title": "A" * 101, "description": "Description", "is_private": False},
        {"title": "Board", "description": "A" * 1001, "is_private": False},
    ],
)
def test_board_create_validation_errors(
    payload,
    session,
    register_url,
    login_url,
    board_create_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.post(
        board_create_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "VALIDATION_ERROR")


@pytest.mark.parametrize(
    "payload",
    [
        {"title": 123, "description": "Description", "is_private": False},
        {"title": "Board", "description": {"text": "Description"}, "is_private": False},
        {"title": "Board", "description": "Description", "is_private": "false"},
    ],
)
def test_board_create_invalid_field_types(
    payload,
    session,
    register_url,
    login_url,
    board_create_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.post(
        board_create_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "INVALID_FORMAT")


@pytest.mark.parametrize(
    "raw_body",
    [
        '{"title": "Board", "is_private": false',
        '{"title": "Board" "is_private": false}',
        "[]",
        '"just a string"',
    ],
)
def test_board_create_invalid_json(
    raw_body,
    session,
    register_url,
    login_url,
    board_create_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.post(
        board_create_url,
        headers={
            **auth_headers(token),
            "Content-Type": "application/json",
        },
        data=raw_body,
    )

    assert_error(response, 400, "INVALID_FORMAT")
