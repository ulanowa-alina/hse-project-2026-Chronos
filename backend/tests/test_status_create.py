import pytest

pytestmark = pytest.mark.asyncio


def assert_status_response(
        response,
        *,
        board_id,
        name="Review status",
        position=10,
):
    assert "data" in response

    status = response["data"]

    assert isinstance(status["id"], int)
    assert status["board_id"] == board_id
    assert status["name"] == name
    assert status["position"] == position

    if "created_at" in status:
        assert isinstance(status["created_at"], str)
    if "updated_at" in status:
        assert isinstance(status["updated_at"], str)

    return status


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


async def test_basic(status_create, created_board):
    response = await status_create()

    assert_status_response(
        response,
        board_id=created_board["board_id"],
    )


async def test_create_without_board_id(status_create):
    response = await status_create(
        status_code=400,
        omit_fields=("board_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="board_id")


async def test_create_without_name(status_create):
    response = await status_create(
        status_code=400,
        omit_fields=("name",),
    )

    assert_error_response(response, "MISSING_FIELD", field="name")


async def test_create_without_position(status_create):
    response = await status_create(
        status_code=400,
        omit_fields=("position",),
    )

    assert_error_response(response, "MISSING_FIELD", field="position")


async def test_create_with_empty_name(status_create):
    response = await status_create(
        status_code=400,
        name="",
    )

    assert_error_response(response, "VALIDATION_ERROR", field="name")


async def test_create_with_too_long_name(status_create):
    response = await status_create(
        status_code=400,
        name="a" * 51,
    )

    assert_error_response(response, "VALIDATION_ERROR", field="name")


async def test_create_with_invalid_board_id(status_create):
    response = await status_create(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_create_with_invalid_name(status_create):
    response = await status_create(
        status_code=400,
        name=123,
    )

    assert_error_response(response, "INVALID_FORMAT", field="name")


async def test_create_with_invalid_position(status_create):
    response = await status_create(
        status_code=400,
        position="0",
    )

    assert_error_response(response, "INVALID_FORMAT", field="position")


async def test_create_unknown_board(status_create):
    response = await status_create(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_create_in_another_users_board(status_create, other_created_board):
    response = await status_create(
        status_code=403,
        board_id=other_created_board["board_id"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_create_without_auth(status_create):
    response = await status_create(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_create_with_invalid_token(status_create):
    response = await status_create(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")


async def test_create_with_invalid_json(status_create):
    response = await status_create(
        status_code=400,
        raw_body='{"board_id": 1, "name": "Todo"',
    )

    assert_error_response(response, "INVALID_FORMAT")
