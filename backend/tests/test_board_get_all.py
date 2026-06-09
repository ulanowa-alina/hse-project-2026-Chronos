import pytest

pytestmark = pytest.mark.asyncio


def assert_board_response(board):
    assert isinstance(board["id"], int)
    assert isinstance(board["user_id"], int)
    assert isinstance(board["title"], str)
    assert isinstance(board["description"], str)
    assert isinstance(board["created_at"], str)
    assert isinstance(board["updated_at"], str)


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


async def test_basic(board_get_all):
    response = await board_get_all()

    assert "data" in response
    assert isinstance(response["data"], list)

    for board in response["data"]:
        assert_board_response(board)


async def test_created_board_is_in_list(board_get_all, created_board):
    response = await board_get_all()

    board_ids = {board["id"] for board in response["data"]}
    assert created_board["board_id"] in board_ids


async def test_other_user_does_not_see_board(board_get_all, other_auth_user, created_board):
    response = await board_get_all(
        headers=other_auth_user["headers"],
    )

    board_ids = {board["id"] for board in response["data"]}
    assert created_board["board_id"] not in board_ids


async def test_user_without_boards_gets_empty_list(board_get_all, user_without_boards):
    response = await board_get_all(
        headers=user_without_boards["headers"],
    )

    assert response["data"] == []


async def test_get_all_without_auth(board_get_all):
    response = await board_get_all(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_get_all_with_invalid_token(board_get_all):
    response = await board_get_all(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")
