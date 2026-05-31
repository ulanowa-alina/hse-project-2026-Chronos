import re

import pytest


ISO_8601_UTC = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")


def assert_board_response(board, board_id, title, description, is_private):
    assert set(board.keys()) == {
        "id",
        "user_id",
        "title",
        "description",
        "is_private",
        "created_at",
        "updated_at",
    }
    assert board["id"] == board_id
    assert isinstance(board["user_id"], int)
    assert board["user_id"] > 0
    assert board["title"] == title
    assert board["description"] == description
    assert board["is_private"] == is_private
    assert ISO_8601_UTC.match(board["created_at"])
    assert ISO_8601_UTC.match(board["updated_at"])


def assert_error_response(body, code):
    assert "error" in body
    assert body["error"]["code"] == code
    assert "message" in body["error"]


def test_board_edit_success_returns_updated_board(board_edit, created_board):
    body = board_edit()

    assert "data" in body
    assert_board_response(
        body["data"],
        board_id=created_board["board_id"],
        title="Updated study board",
        description="Updated spring semester",
        is_private=True,
    )


def test_board_edit_requires_authorization(board_edit):
    body = board_edit(status_code=401, headers={})

    assert_error_response(body, "UNAUTHORIZED")


def test_board_edit_rejects_invalid_token(board_edit):
    body = board_edit(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(body, "INVALID_TOKEN")


@pytest.mark.parametrize(
    "field_name",
    [
        "board_id",
        "title",
        "description",
        "is_private",
    ],
)
def test_board_edit_requires_fields(board_edit, created_board, field_name):
    body = board_edit(status_code=400, omit_fields=(field_name,))

    assert_error_response(body, "MISSING_FIELD")
    assert body["error"]["details"]["missing_fields"] == [field_name]


@pytest.mark.parametrize(
    "kwargs",
    [
        {"board_id": 0},
        {"board_id": -1},
        {"title": ""},
        {"title": "A" * 101},
        {"description": "A" * 1001},
    ],
)
def test_board_edit_validates_values(board_edit, kwargs):
    body = board_edit(status_code=400, **kwargs)

    assert_error_response(body, "VALIDATION_ERROR")


@pytest.mark.parametrize(
    "kwargs",
    [
        {"board_id": "1"},
        {"board_id": {"id": 1}},
        {"title": 123},
        {"description": {"text": "Description"}},
        {"is_private": "false"},
    ],
)
def test_board_edit_validates_field_types(board_edit, kwargs):
    body = board_edit(status_code=400, **kwargs)

    assert_error_response(body, "INVALID_FORMAT")


def test_board_edit_returns_not_found_for_unknown_board(board_edit):
    body = board_edit(status_code=404, board_id=999999999)

    assert_error_response(body, "BOARD_NOT_FOUND")


def test_board_edit_forbids_other_user(board_edit, other_auth_user):
    body = board_edit(status_code=403, headers=other_auth_user["headers"])

    assert_error_response(body, "RESOURCE_NOT_OWNED")


@pytest.mark.parametrize(
    "raw_body",
    [
        '{"board_id": 1, "title": "Board", "description": "Text", "is_private": false',
        '{"board_id": 1 "title": "Board"}',
        "[]",
        '"just a string"',
    ],
)
def test_board_edit_rejects_invalid_json(board_edit, raw_body):
    body = board_edit(status_code=400, raw_body=raw_body)

    assert_error_response(body, "INVALID_FORMAT")
