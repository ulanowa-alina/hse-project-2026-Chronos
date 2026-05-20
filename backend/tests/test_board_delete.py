from uuid import uuid4

import pytest


def unique_email():
    return f"board-delete-{uuid4().hex}@example.com"


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


def assert_error(response, status_code, code):
    assert response.status_code == status_code

    body = response.json()
    assert "error" in body
    assert body["error"]["code"] == code


def delete_payload(board_id):
    return {"board_id": board_id}


def test_board_delete_success_returns_no_content(
    session,
    register_url,
    login_url,
    board_create_url,
    board_delete_url,
    board_get_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, token)

    response = session.delete(
        board_delete_url,
        headers=auth_headers(token),
        json=delete_payload(board["id"]),
    )

    assert response.status_code == 204
    assert response.text == ""

    get_response = session.get(
        board_get_url,
        headers=auth_headers(token),
        params={"board_id": board["id"]},
    )
    assert_error(get_response, 404, "BOARD_NOT_FOUND")


def test_board_delete_requires_auth(
    session,
    register_url,
    login_url,
    board_create_url,
    board_delete_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, token)

    response = session.delete(
        board_delete_url,
        json=delete_payload(board["id"]),
    )

    assert_error(response, 401, "UNAUTHORIZED")


def test_board_delete_requires_board_id(
    session,
    register_url,
    login_url,
    board_delete_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.delete(
        board_delete_url,
        headers=auth_headers(token),
        json={},
    )

    assert_error(response, 400, "MISSING_FIELD")
    assert (
        response.json()["error"]["details"]["board_id"]
        == "Field board_id is required"
    )


@pytest.mark.parametrize(
    "payload",
    [
        {"board_id": "1"},
        {"board_id": {"id": 1}},
        {"board_id": [1]},
    ],
)
def test_board_delete_invalid_board_id_type(
    payload,
    session,
    register_url,
    login_url,
    board_delete_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.delete(
        board_delete_url,
        headers=auth_headers(token),
        json=payload,
    )

    assert_error(response, 400, "INVALID_FORMAT")


@pytest.mark.parametrize("board_id", [0, -1])
def test_board_delete_invalid_board_id_value(
    board_id,
    session,
    register_url,
    login_url,
    board_delete_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.delete(
        board_delete_url,
        headers=auth_headers(token),
        json=delete_payload(board_id),
    )

    assert_error(response, 400, "VALIDATION_ERROR")


def test_board_delete_returns_not_found_for_unknown_board(
    session,
    register_url,
    login_url,
    board_delete_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.delete(
        board_delete_url,
        headers=auth_headers(token),
        json=delete_payload(999999999),
    )

    assert_error(response, 404, "BOARD_NOT_FOUND")


def test_board_delete_forbidden_for_other_user(
    session,
    register_url,
    login_url,
    board_create_url,
    board_delete_url,
):
    _, owner_token = create_authenticated_user(session, register_url, login_url)
    _, other_token = create_authenticated_user(session, register_url, login_url)
    board = create_board(session, board_create_url, owner_token)

    response = session.delete(
        board_delete_url,
        headers=auth_headers(other_token),
        json=delete_payload(board["id"]),
    )

    assert_error(response, 403, "RESOURCE_NOT_OWNED")


@pytest.mark.parametrize(
    "raw_body",
    [
        '{"board_id": 1',
        '{"board_id" 1}',
        "[]",
        '"just a string"',
    ],
)
def test_board_delete_invalid_json(
    raw_body,
    session,
    register_url,
    login_url,
    board_delete_url,
):
    _, token = create_authenticated_user(session, register_url, login_url)

    response = session.delete(
        board_delete_url,
        headers={
            **auth_headers(token),
            "Content-Type": "application/json",
        },
        data=raw_body,
    )

    assert_error(response, 400, "INVALID_FORMAT")
