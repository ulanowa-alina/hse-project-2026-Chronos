import pytest

pytestmark = pytest.mark.asyncio


def assert_task_response(task):
    assert isinstance(task["id"], int)
    assert isinstance(task["board_id"], int)
    assert isinstance(task["title"], str)
    assert isinstance(task["description"], str)
    assert isinstance(task["status_id"], int)
    assert isinstance(task["priority_color"], str)
    assert isinstance(task["created_at"], str)
    assert isinstance(task["updated_at"], str)

    if "deadline" in task:
        assert task["deadline"] is None or isinstance(task["deadline"], str)


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


async def test_basic(board_tasks):
    response = await board_tasks()

    assert "data" in response
    assert isinstance(response["data"], list)

    for task in response["data"]:
        assert_task_response(task)


async def test_empty_board_returns_empty_list(board_tasks):
    response = await board_tasks()

    assert response["data"] == []


async def test_created_task_is_in_board_tasks(board_tasks, created_task, created_board):
    response = await board_tasks()

    task_ids = {task["id"] for task in response["data"]}
    assert created_task["task_id"] in task_ids

    for task in response["data"]:
        assert_task_response(task)
        assert task["board_id"] == created_board["board_id"]


async def test_tasks_without_board_id(board_tasks):
    response = await board_tasks(
        status_code=400,
        omit_board_id=True,
    )

    assert_error_response(response, "MISSING_FIELD", field="board_id")


async def test_tasks_with_invalid_board_id(board_tasks):
    response = await board_tasks(
        status_code=400,
        board_id="wrong",
    )

    assert_error_response(response, "INVALID_FORMAT", field="board_id")


async def test_tasks_unknown_board(board_tasks):
    response = await board_tasks(
        status_code=404,
        board_id=999999999,
    )

    assert_error_response(response, "BOARD_NOT_FOUND")


async def test_tasks_another_users_board(board_tasks, other_created_board):
    response = await board_tasks(
        status_code=403,
        board_id=other_created_board["board_id"],
    )

    assert response["error"]["code"] in ("RESOURCE_NOT_OWNED", "FORBIDDEN")


async def test_tasks_without_auth(board_tasks):
    response = await board_tasks(
        status_code=401,
        headers={},
    )

    assert_error_response(response, "UNAUTHORIZED")


async def test_tasks_with_invalid_token(board_tasks):
    response = await board_tasks(
        status_code=401,
        headers={"Authorization": "Bearer invalid-token"},
    )

    assert_error_response(response, "INVALID_TOKEN")
