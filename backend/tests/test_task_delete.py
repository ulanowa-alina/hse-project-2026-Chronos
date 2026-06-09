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


async def test_basic(task_delete):
    response = await task_delete()

    assert response is None


async def test_delete_without_task_id(task_delete):
    response = await task_delete(
        status_code=400,
        omit_fields=("task_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="task_id")


async def test_delete_with_invalid_task_id(task_delete):
    response = await task_delete(
        status_code=400,
        task_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="task_id")


async def test_delete_unknown_task(task_delete):
    response = await task_delete(
        status_code=404,
        task_id=999999999,
    )

    assert_error_response(response, "TASK_NOT_FOUND")


async def test_delete_another_users_task(task_delete, other_auth_user):
    response = await task_delete(
        status_code=403,
        headers=other_auth_user["headers"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_delete_same_task_twice(task_delete):
    response = await task_delete()

    assert response is None

    response = await task_delete(status_code=404)

    assert_error_response(response, "TASK_NOT_FOUND")


async def test_delete_without_auth(task_delete):
    response = await task_delete(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_delete_with_invalid_token(task_delete):
    response = await task_delete(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")


async def test_delete_with_invalid_json(task_delete):
    response = await task_delete(
        status_code=400,
        raw_body='{"task_id": 1',
    )

    assert_error_response(response, "INVALID_FORMAT")
