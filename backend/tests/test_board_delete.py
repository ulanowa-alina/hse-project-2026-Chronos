import pytest


def assert_error_response(body, code):
    assert "error" in body
    assert body["error"]["code"] == code
    assert "message" in body["error"]


def test_board_delete_success_returns_no_content(board_delete):
    body = board_delete()

    assert body is None


def test_board_delete_removes_existing_board(board_delete):
    board_delete()

    body = board_delete(status_code=404)

    assert_error_response(body, "BOARD_NOT_FOUND")


def test_board_delete_requires_authorization(board_delete):
    body = board_delete(status_code=401, headers={})

    assert_error_response(body, "UNAUTHORIZED")


def test_board_delete_rejects_invalid_token(board_delete):
    body = board_delete(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(body, "INVALID_TOKEN")


def test_board_delete_requires_board_id(board_delete):
    body = board_delete(status_code=400, omit_fields=("board_id",))

    assert_error_response(body, "MISSING_FIELD")
    assert body["error"]["details"]["board_id"] == "Field board_id is required"


@pytest.mark.parametrize(
    "kwargs",
    [
        {"board_id": "1"},
        {"board_id": {"id": 1}},
        {"board_id": [1]},
    ],
)
def test_board_delete_validates_board_id_type(board_delete, kwargs):
    body = board_delete(status_code=400, **kwargs)

    assert_error_response(body, "INVALID_FORMAT")


@pytest.mark.parametrize("board_id", [0, -1])
def test_board_delete_validates_board_id_value(board_delete, board_id):
    body = board_delete(status_code=400, board_id=board_id)

    assert_error_response(body, "VALIDATION_ERROR")


def test_board_delete_returns_not_found_for_unknown_board(board_delete):
    body = board_delete(status_code=404, board_id=999999999)

    assert_error_response(body, "BOARD_NOT_FOUND")


def test_board_delete_forbids_other_user(board_delete, other_auth_user):
    body = board_delete(status_code=403, headers=other_auth_user["headers"])

    assert_error_response(body, "RESOURCE_NOT_OWNED")


@pytest.mark.parametrize(
    "raw_body",
    [
        '{"board_id": 1',
        '{"board_id" 1}',
        "[]",
        '"just a string"',
    ],
)
def test_board_delete_rejects_invalid_json(board_delete, raw_body):
    body = board_delete(status_code=400, raw_body=raw_body)

    assert_error_response(body, "INVALID_FORMAT")
