import pytest

pytestmark = pytest.mark.asyncio


def assert_board_response(
        response,
        *,
        board_id,
        title="Updated study board",
        description="Updated spring semester",
):
    assert "data" in response

    board = response["data"]

    assert board["id"] == board_id
    assert isinstance(board["user_id"], int)

    assert board["title"] == title
    assert board["description"] == description

    assert isinstance(board["created_at"], str)
    assert isinstance(board["updated_at"], str)

    return board


def assert_error_response(
        response,
        code,
        field=None,
):
    assert "error" in response

    error = response["error"]

    assert error["code"] == code
    assert isinstance(error["message"], str)
    assert error["message"]

    if field is not None:
        assert "details" in error
        assert field in error["details"]


async def test_basic(board_edit, created_board):
    response = await board_edit()

    board = assert_board_response(
        response,
        board_id=created_board["board_id"],
    )

    assert board["id"] == created_board["board_id"]


async def test_edit_without_board_id(board_edit):
    response = await board_edit(
        status_code=400,
        omit_fields=("board_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="board_id")


async def test_edit_without_title(board_edit):
    response = await board_edit(
        status_code=400,
        omit_fields=("title",),
    )

    assert_error_response(response, "MISSING_FIELD", field="title")


async def test_edit_with_empty_title(board_edit):
    response = await board_edit(
        status_code=400,
        title="",
    )

    assert_error_response(response, "VALIDATION_ERROR", field="title")


async def test_edit_with_too_long_title(board_edit):
    response = await board_edit(
        status_code=400,
        title="a" * 101,
    )

    assert_error_response(response, "VALIDATION_ERROR", field="title")


async def test_edit_with_max_title_length(board_edit, created_board):
    response = await board_edit(
        title="a" * 100,
    )

    assert_board_response(
        response,
        board_id=created_board["board_id"],
        title="a" * 100,
    )


async def test_edit_with_too_long_description(board_edit):
    response = await board_edit(
        status_code=400,
        description="a" * 1001,
    )

    assert_error_response(response, "VALIDATION_ERROR", field="description")


async def test_edit_with_max_description_length(board_edit, created_board):
    response = await board_edit(
        description="a" * 1000,
    )

    assert_board_response(
        response,
        board_id=created_board["board_id"],
        description="a" * 1000,
    )


async def test_edit_with_invalid_board_id(board_edit):
    response = await board_edit(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_edit_with_null_description(board_edit):
    response = await board_edit(
        status_code=400,
        description=None,
    )

    assert_error_response(response, "INVALID_FORMAT", field="description")


async def test_edit_with_invalid_json(board_edit):
    response = await board_edit(
        status_code=400,
        raw_body='{"board_id": 1, "title": "Updated"',
    )

    assert_error_response(response, "INVALID_FORMAT")


async def test_edit_unknown_board(board_edit):
    response = await board_edit(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_edit_another_users_board(board_edit, other_auth_user):
    response = await board_edit(
        status_code=403,
        headers=other_auth_user["headers"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_edit_without_auth(board_edit):
    response = await board_edit(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_edit_with_invalid_token(board_edit):
    response = await board_edit(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")
