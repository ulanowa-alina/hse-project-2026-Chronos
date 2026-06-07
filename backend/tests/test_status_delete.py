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


async def test_basic(status_delete):
    response = await status_delete()

    assert response is None


async def test_delete_without_status_id(status_delete):
    response = await status_delete(
        status_code=400,
        omit_fields=("status_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="status_id")


async def test_delete_with_invalid_status_id(status_delete):
    response = await status_delete(
        status_code=400,
        status_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="status_id")


async def test_delete_unknown_status(status_delete):
    response = await status_delete(
        status_code=404,
        status_id=999999999,
    )

    assert_error_response(response, "STATUS_NOT_FOUND")


async def test_delete_another_users_status(status_delete, other_auth_user):
    response = await status_delete(
        status_code=403,
        headers=other_auth_user["headers"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_delete_same_status_twice(status_delete):
    response = await status_delete()

    assert response is None

    response = await status_delete(status_code=404)

    assert_error_response(response, "STATUS_NOT_FOUND")


async def test_delete_without_auth(status_delete):
    response = await status_delete(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_delete_with_invalid_token(status_delete):
    response = await status_delete(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")


async def test_delete_with_invalid_json(status_delete):
    response = await status_delete(
        status_code=400,
        raw_body='{"status_id": 1',
    )

    assert_error_response(response, "INVALID_FORMAT")
