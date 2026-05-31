import re

import pytest


ISO_8601_UTC = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")


def assert_board_response(board, title, description, is_private):
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


def test_board_create_success_returns_board_model(board_create):
    body = board_create(
        title="Study board",
        description="Spring semester",
        is_private=True,
    )

    assert "data" in body
    assert_board_response(
        body["data"],
        title="Study board",
        description="Spring semester",
        is_private=True,
    )


def test_board_create_without_description_uses_empty_string(board_create):
    body = board_create(
        omit_fields=("description",),
        title="Board without description",
        is_private=False,
    )

    assert_board_response(
        body["data"],
        title="Board without description",
        description="",
        is_private=False,
    )


def test_board_create_requires_authorization(board_create):
    body = board_create(status_code=401, headers={})

    assert_error_response(body, "UNAUTHORIZED")


def test_board_create_rejects_invalid_token(board_create):
    body = board_create(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(body, "INVALID_TOKEN")


@pytest.mark.parametrize(
    "omit_fields, missing_fields",
    [
        (("title",), ["title"]),
        (("is_private",), ["is_private"]),
        (("title", "is_private"), ["title", "is_private"]),
    ],
)
def test_board_create_requires_fields(board_create, omit_fields, missing_fields):
    body = board_create(status_code=400, omit_fields=omit_fields)

    assert_error_response(body, "MISSING_FIELD")
    assert body["error"]["details"]["missing_fields"] == missing_fields


@pytest.mark.parametrize(
    "kwargs",
    [
        {"title": ""},
        {"title": "A" * 101},
        {"description": "A" * 1001},
    ],
)
def test_board_create_validates_values(board_create, kwargs):
    body = board_create(status_code=400, **kwargs)

    assert_error_response(body, "VALIDATION_ERROR")


@pytest.mark.parametrize(
    "kwargs",
    [
        {"title": 123},
        {"description": {"text": "Description"}},
        {"is_private": "false"},
    ],
)
def test_board_create_validates_field_types(board_create, kwargs):
    body = board_create(status_code=400, **kwargs)

    assert_error_response(body, "INVALID_FORMAT")


@pytest.mark.parametrize(
    "raw_body",
    [
        '{"title": "Board", "is_private": false',
        '{"title": "Board" "is_private": false}',
        "[]",
        '"just a string"',
    ],
)
def test_board_create_rejects_invalid_json(board_create, raw_body):
    body = board_create(status_code=400, raw_body=raw_body)

    assert_error_response(body, "INVALID_FORMAT")
