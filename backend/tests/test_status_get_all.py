import pytest

pytestmark = pytest.mark.asyncio


def assert_status_response(status):
    assert isinstance(status["id"], int)
    assert isinstance(status["board_id"], int)
    assert isinstance(status["name"], str)
    assert isinstance(status["position"], int)

    if "created_at" in status:
        assert isinstance(status["created_at"], str)
    if "updated_at" in status:
        assert isinstance(status["updated_at"], str)


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


async def test_basic(status_get_all, created_board, created_status):
    response = await status_get_all()

    assert "data" in response
    assert isinstance(response["data"], list)

    status_ids = {status["id"] for status in response["data"]}
    assert created_status["id"] in status_ids

    for status in response["data"]:
        assert_status_response(status)
        assert status["board_id"] == created_board["board_id"]


async def test_get_all_without_board_id(status_get_all):
    response = await status_get_all(omit_board_id=True)

    assert "data" in response
    assert isinstance(response["data"], list)

    for status in response["data"]:
        assert_status_response(status)


async def test_get_all_with_invalid_board_id(status_get_all):
    response = await status_get_all(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_get_all_unknown_board(status_get_all):
    response = await status_get_all(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_get_all_another_users_board(status_get_all, other_created_board):
    response = await status_get_all(
        status_code=403,
        board_id=other_created_board["board_id"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_get_all_without_auth(status_get_all):
    response = await status_get_all(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_get_all_with_invalid_token(status_get_all):
    response = await status_get_all(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")
