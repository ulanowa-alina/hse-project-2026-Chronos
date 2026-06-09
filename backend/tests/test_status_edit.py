import pytest

pytestmark = pytest.mark.asyncio


def assert_status_response(
        response,
        *,
        status_id,
        name="Updated status",
        position=30,
):
    assert "data" in response

    status = response["data"]

    assert status["id"] == status_id
    assert isinstance(status["board_id"], int)
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


async def test_basic(status_edit, created_status):
    response = await status_edit()

    assert_status_response(
        response,
        status_id=created_status["id"],
    )


async def test_edit_without_status_id(status_edit):
    response = await status_edit(
        status_code=400,
        omit_fields=("status_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="status_id")


async def test_edit_without_name(status_edit):
    response = await status_edit(
        status_code=400,
        omit_fields=("name",),
    )

    assert_error_response(response, "MISSING_FIELD", field="name")


async def test_edit_without_position(status_edit):
    response = await status_edit(
        status_code=400,
        omit_fields=("position",),
    )

    assert_error_response(response, "MISSING_FIELD", field="position")


async def test_edit_with_empty_name(status_edit):
    response = await status_edit(
        status_code=400,
        name="",
    )

    assert_error_response(response, "VALIDATION_ERROR", field="name")


async def test_edit_with_too_long_name(status_edit):
    response = await status_edit(
        status_code=400,
        name="a" * 51,
    )

    assert_error_response(response, "VALIDATION_ERROR", field="name")


async def test_edit_with_invalid_status_id(status_edit):
    response = await status_edit(
        status_code=400,
        status_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="status_id")


async def test_edit_with_invalid_name(status_edit):
    response = await status_edit(
        status_code=400,
        name=123,
    )

    assert_error_response(response, "INVALID_FORMAT", field="name")


async def test_edit_with_invalid_position(status_edit):
    response = await status_edit(
        status_code=400,
        position="0",
    )

    assert_error_response(response, "INVALID_FORMAT", field="position")


async def test_edit_unknown_status(status_edit):
    response = await status_edit(
        status_code=404,
        status_id=999999999,
    )

    assert_error_response(response, "STATUS_NOT_FOUND")


async def test_edit_another_users_status(status_edit, other_auth_user):
    response = await status_edit(
        status_code=403,
        headers=other_auth_user["headers"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_edit_without_auth(status_edit):
    response = await status_edit(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_edit_with_invalid_token(status_edit):
    response = await status_edit(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")


async def test_edit_with_invalid_json(status_edit):
    response = await status_edit(
        status_code=400,
        raw_body='{"status_id": 1, "name": "Todo"',
    )

    assert_error_response(response, "INVALID_FORMAT")
