import pytest

pytestmark = pytest.mark.asyncio


def assert_task_response(
        response,
        *,
        board_id,
        status_id,
        title="Write lecture notes",
        description="Prepare notes for the next seminar",
        priority_color="blue",
):
    assert "data" in response

    task = response["data"]

    assert isinstance(task["id"], int)
    assert task["board_id"] == board_id
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


async def test_basic(task_create, created_board, created_status):
    response = await task_create()

    assert_task_response(
        response,
        board_id=created_board["board_id"],
        status_id=created_status["id"],
    )


async def test_create_without_board_id(task_create):
    response = await task_create(
        status_code=400,
        omit_fields=("board_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="board_id")


async def test_create_without_title(task_create):
    response = await task_create(
        status_code=400,
        omit_fields=("title",),
    )

    assert_error_response(response, "MISSING_FIELD", field="title")


async def test_create_without_status_id(task_create):
    response = await task_create(
        status_code=400,
        omit_fields=("status_id",),
    )

    assert_error_response(response, "MISSING_FIELD", field="status_id")


async def test_create_with_empty_title(task_create):
    response = await task_create(
        status_code=400,
        title="",
    )

    assert_error_response(response, "VALIDATION_ERROR", field="title")


async def test_create_with_too_long_title(task_create):
    response = await task_create(
        status_code=400,
        title="a" * 101,
    )

    assert_error_response(response, "VALIDATION_ERROR", field="title")


async def test_create_with_invalid_board_id(task_create):
    response = await task_create(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_create_with_invalid_status_id(task_create):
    response = await task_create(
        status_code=400,
        status_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="status_id")


async def test_create_unknown_board(task_create):
    response = await task_create(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_create_unknown_status(task_create):
    response = await task_create(
        status_code=404,
        status_id=999999999,
    )

    assert_error_response(response, "STATUS_NOT_FOUND")


async def test_create_in_another_users_board(
        task_create,
        other_created_board,
        other_created_status,
):
    response = await task_create(
        status_code=403,
        board_id=other_created_board["board_id"],
        status_id=other_created_status["id"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_create_without_auth(task_create):
    response = await task_create(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_create_with_invalid_token(task_create):
    response = await task_create(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")


async def test_create_with_invalid_json(task_create):
    response = await task_create(
        status_code=400,
        raw_body='{"board_id": 1, "title": "Task"',
    )

    assert_error_response(response, "INVALID_FORMAT")
