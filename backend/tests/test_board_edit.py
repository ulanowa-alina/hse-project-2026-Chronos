import re
from uuid import uuid4

import pytest


ISO_8601_UTC = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")


def unique_email():
    return f"board-edit-{uuid4().hex}@example.com"


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


def create_board(session, board_create_url, token):
    response = session.post(
        board_create_url,
        headers=auth_headers(token),
        json={
            "title": "Study board",
            "description": "Spring semester",
            "is_private": False,
        },
    )

    assert response.status_code == 200
    return response.json()["data"]


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


def valid_edit_payload(board_id):
    return {
        "board_id": board_id,
        "title": "Updated study board",
        "description": "Updated spring semester",
        "is_private": True,
    }


def test_board_edit_success_returns_updated_board_model(
    session,
    register_url,
    login_url,
    board_create_url,
    board_edit_url,
):
    user, token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, token)

    response = session.patch(
        board_edit_url,
        headers=auth_headers(token),
        json=valid_edit_payload(board["id"]),
    )

    assert response.status_code == 200

    updated_board = response.json()["data"]
    assert updated_board["id"] == board["id"]
    assert_board_model(
        updated_board,
        expected_user_id=user["id"],
        expected_title="Updated study board",
        expected_description="Updated spring semester",
        expected_is_private=True,
    )


def test_board_edit_requires_auth(
    session,
    register_url,
    login_url,
    board_create_url,
    board_edit_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, token)

    response = session.patch(
        board_edit_url,
        json=valid_edit_payload(board["id"]),
    )

    assert_error(response, 401, "UNAUTHORIZED")


@pytest.mark.parametrize(
    "field_name",
    [
        "board_id",
        "title",
        "description",
        "is_private",
    ],
)
def test_board_edit_required_fields(
    field_name,
    session,
    register_url,
    login_url,
    board_create_url,
    board_edit_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, token)
    payload = valid_edit_payload(board["id"])
    payload.pop(field_name)

    response = session.patch(
        board_edit_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "MISSING_FIELD")
    assert response.json()["error"]["details"]["missing_fields"] == [field_name]


@pytest.mark.parametrize(
    "payload",
    [
        {
            "board_id": 0,
            "title": "Updated board",
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": -1,
            "title": "Updated board",
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": 1,
            "title": "",
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": 1,
            "title": "A" * 101,
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": 1,
            "title": "Updated board",
            "description": "A" * 1001,
            "is_private": False,
        },
    ],
)
def test_board_edit_validation_errors(
    payload,
    session,
    register_url,
    login_url,
    board_edit_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.patch(
        board_edit_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "VALIDATION_ERROR")


@pytest.mark.parametrize(
    "payload",
    [
        {
            "board_id": "1",
            "title": "Updated board",
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": {"id": 1},
            "title": "Updated board",
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": 1,
            "title": 123,
            "description": "Description",
            "is_private": False,
        },
        {
            "board_id": 1,
            "title": "Updated board",
            "description": {"text": "Description"},
            "is_private": False,
        },
        {
            "board_id": 1,
            "title": "Updated board",
            "description": "Description",
            "is_private": "false",
        },
    ],
)
def test_board_edit_invalid_field_types(
    payload,
    session,
    register_url,
    login_url,
    board_edit_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.patch(
        board_edit_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "INVALID_FORMAT")


def test_board_edit_returns_not_found_for_unknown_board(
    session,
    register_url,
    login_url,
    board_edit_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.patch(
        board_edit_url,
        headers=auth_headers(token),
        json=valid_edit_payload(999999999),
    )

    assert_error(response, 404, "BOARD_NOT_FOUND")


def test_board_edit_forbidden_for_other_user(
    session,
    register_url,
    login_url,
    board_create_url,
    board_edit_url,
):
    _, owner_token = create_authenticated_user(session, register_url, login_url)
    _, other_token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, owner_token)

    response = session.patch(
        board_edit_url,
        headers=auth_headers(other_token),
        json=valid_edit_payload(board["id"]),
    )

    assert_error(response, 403, "RESOURCE_NOT_OWNED")


@pytest.mark.parametrize(
    "raw_body",
    [
        '{"board_id": 1, "title": "Board", "description": "Text", "is_private": false',
        '{"board_id": 1 "title": "Board"}',
        "[]",
        '"just a string"',
    ],
)
def test_board_edit_invalid_json(
    raw_body,
    session,
    register_url,
    login_url,
    board_edit_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.patch(
        board_edit_url,
        headers={
            **auth_headers(token),
            "Content-Type": "application/json",
        },
        data=raw_body,
    )

    assert_error(response, 400, "INVALID_FORMAT")
