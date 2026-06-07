import pytest

pytestmark = pytest.mark.asyncio


def assert_task_response(
        response,
        *,
        task_id,
        status_id,
        title="Updated lecture notes",
        description="Polish notes before publishing",
        priority_color="red",
):
    assert "data" in response

    task = response["data"]

    assert task["id"] == task_id
    assert isinstance(task["board_id"], int)
    assert task["title"] == title
    assert task["description"] == description
    assert task["status_id"] == status_id
    assert task["priority_color"] == priority_color

    assert isinstance(task["created_at"], str)
    assert isinstance(task["updated_at"], str)

    if "deadline" in task:
        assert task["deadline"] is None or isinstance(task["deadline"], str)

    return task


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


async def test_basic(task_edit, created_task, another_created_status):
    response = await task_edit()

    assert_task_response(
        response,
        task_id=created_task["task_id"],
        status_id=another_created_status["id"],
    )


async def test_edit_without_task_id(task_edit):
    response = await task_edit(
        status_code=400,
        omit_fields=("task_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="task_id")


async def test_edit_without_title(task_edit):
    response = await task_edit(
        status_code=400,
        omit_fields=("title",),
    )

    assert_error_response(response, "MISSING_FIELD", field="title")


async def test_edit_with_empty_title(task_edit):
    response = await task_edit(
        status_code=400,
        title="",
    )

    assert_error_response(response, "VALIDATION_ERROR", field="title")


async def test_edit_with_too_long_title(task_edit):
    response = await task_edit(
        status_code=400,
        title="a" * 101,
    )

    assert_error_response(response, "VALIDATION_ERROR", field="title")


async def test_edit_with_invalid_task_id(task_edit):
    response = await task_edit(
        status_code=400,
        task_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="task_id")


async def test_edit_unknown_task(task_edit):
    response = await task_edit(
        status_code=404,
        task_id=999999999,
    )

    assert_error_response(response, "TASK_NOT_FOUND")


async def test_edit_unknown_status(task_edit):
    response = await task_edit(
        status_code=404,
        status_id=999999999,
    )

    assert_error_response(response, "STATUS_NOT_FOUND")


async def test_edit_another_users_task(task_edit, other_auth_user):
    response = await task_edit(
        status_code=403,
        headers=other_auth_user["headers"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_edit_without_auth(task_edit):
    response = await task_edit(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_edit_with_invalid_token(task_edit):
    response = await task_edit(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")


async def test_edit_with_invalid_json(task_edit):
    response = await task_edit(
        status_code=400,
        raw_body='{"task_id": 1, "title": "Updated"',
    )

    assert_error_response(response, "INVALID_FORMAT")
