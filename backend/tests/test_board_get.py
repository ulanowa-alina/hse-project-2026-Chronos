import pytest

pytestmark = pytest.mark.asyncio


def assert_board_response(
        response,
        *,
        board_id,
        title="test board",
        description="test description",
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


async def test_basic(board_get, created_board):
    response = await board_get()

    assert_board_response(
        response,
        board_id=created_board["board_id"],
    )


async def test_get_without_board_id(board_get):
    response = await board_get(
        status_code=400,
        omit_board_id=True,
    )

    assert_error_response(response, "MISSING_FIELD", field="board_id")


async def test_get_with_invalid_board_id(board_get):
    response = await board_get(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_get_unknown_board(board_get):
    response = await board_get(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_get_another_users_board(board_get, other_created_board):
    response = await board_get(
        status_code=403,
        board_id=other_created_board["board_id"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_get_without_auth(board_get):
    response = await board_get(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_get_with_invalid_token(board_get):
    response = await board_get(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")
