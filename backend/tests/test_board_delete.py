import pytest

pytestmark = pytest.mark.asyncio


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


async def test_basic(board_delete):
    response = await board_delete()

    assert response is None


async def test_delete_without_board_id(board_delete):
    response = await board_delete(
        status_code=400,
        omit_fields=("board_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="board_id")


async def test_delete_with_invalid_board_id(board_delete):
    response = await board_delete(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_delete_with_invalid_json(board_delete):
    response = await board_delete(
        status_code=400,
        raw_body='{"board_id": 1',
    )

    assert_error_response(response, "INVALID_FORMAT")


async def test_delete_unknown_board(board_delete):
    response = await board_delete(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_delete_another_users_board(board_delete, other_auth_user):
    response = await board_delete(
        status_code=403,
        headers=other_auth_user["headers"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_delete_same_board_twice(board_delete):
    response = await board_delete()

    assert response is None

    response = await board_delete(status_code=404)

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_get_deleted_board_returns_not_found(board_delete, board_get, created_board):
    response = await board_delete()

    assert response is None

    response = await board_get(
        status_code=404,
        board_id=created_board["board_id"],
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_delete_without_auth(board_delete):
    response = await board_delete(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_delete_with_invalid_token(board_delete):
    response = await board_delete(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")
