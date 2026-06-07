import uuid

import httpx
import pytest
import pytest_asyncio


BASE_URL = "http://127.0.0.1:8080"

AUTH_REGISTER_URL = "/auth/v1/register"
AUTH_LOGIN_URL = "/auth/v1/login"

BOARD_CREATE_URL = "/board/v1/create"
BOARD_EDIT_URL = "/board/v1/edit"
BOARD_DELETE_URL = "/board/v1/delete"

STATUS_CREATE_URL = "/status/v1/create"
STATUS_EDIT_URL = "/status/v1/edit"
STATUS_DELETE_URL = "/status/v1/delete"
STATUS_GET_ALL_URL = "/status/v1/get_all"

TASK_CREATE_URL = "/task/v1/create"
TASK_EDIT_URL = "/task/v1/edit"
TASK_DELETE_URL = "/task/v1/delete"


@pytest_asyncio.fixture
async def service_client():
    async with httpx.AsyncClient(base_url=BASE_URL) as client:
        yield client


@pytest_asyncio.fixture
async def auth_user(service_client):
    return await create_auth_user(service_client)


@pytest_asyncio.fixture
async def other_auth_user(service_client):
    return await create_auth_user(service_client)


async def create_auth_user(service_client):
    email = f"test-{uuid.uuid4()}@example.com"
    password = "password123"

    register_response = await service_client.post(
        AUTH_REGISTER_URL,
        json={
            "name": "Test User",
            "email": email,
            "status": "student",
            "password": password,
        },
    )

    assert register_response.status_code in (200, 201), register_response.text

    login_response = await service_client.post(
        AUTH_LOGIN_URL,
        json={
            "email": email,
            "password": password,
        },
    )

    assert login_response.status_code == 200, login_response.text

    login_body = login_response.json()
    token = login_body["data"]["token"]
    user = login_body["data"]["user"]

    return {
        "user": user,
        "headers": {
            "Authorization": f"Bearer {token}",
        },
    }


@pytest.fixture
def auth_headers(auth_user):
    return auth_user["headers"]


def json_or_none(response):
    if not response.content:
        return None

    return response.json()


def build_json(default_body, omit_fields=(), overrides=None):
    body = dict(default_body)

    if overrides is not None:
        body.update(overrides)

    for field in omit_fields:
        body.pop(field, None)

    return body


@pytest.fixture(name="board_create")
def _board_create(service_client, auth_headers):
    async def _inner(
            status_code=200,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.post(
                BOARD_CREATE_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "title": "test board",
                    "description": "test description",
                    "is_private": False,
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.post(
                BOARD_CREATE_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest_asyncio.fixture
async def created_board(auth_user, board_create):
    response = await board_create(headers=auth_user["headers"])
    board = response["data"]

    return {
        "owner": auth_user,
        "board": board,
        "board_id": board["id"],
    }


@pytest.fixture(name="board_edit")
def _board_edit(service_client, auth_headers, created_board):
    async def _inner(
            status_code=200,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.patch(
                BOARD_EDIT_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "board_id": created_board["board_id"],
                    "title": "Updated study board",
                    "description": "Updated spring semester",
                    "is_private": True,
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.patch(
                BOARD_EDIT_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture(name="board_delete")
def _board_delete(service_client, auth_headers, created_board):
    async def _inner(
            status_code=204,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.request(
                "DELETE",
                BOARD_DELETE_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "board_id": created_board["board_id"],
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.request(
                "DELETE",
                BOARD_DELETE_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest_asyncio.fixture
async def other_created_board(other_auth_user, board_create):
    response = await board_create(headers=other_auth_user["headers"])
    board = response["data"]

    return {
        "owner": other_auth_user,
        "board": board,
        "board_id": board["id"],
    }


@pytest_asyncio.fixture
async def other_created_status(service_client, other_created_board):
    response = await service_client.get(
        STATUS_GET_ALL_URL,
        headers=other_created_board["owner"]["headers"],
        params={"board_id": other_created_board["board_id"]},
    )

    assert response.status_code == 200, response.text
    statuses = response.json()["data"]
    assert statuses

    return statuses[0]


@pytest.fixture(name="status_create")
def _status_create(service_client, auth_headers, created_board):
    async def _inner(
            status_code=200,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.post(
                STATUS_CREATE_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "board_id": created_board["board_id"],
                    "name": "Review status",
                    "position": 10,
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.post(
                STATUS_CREATE_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest_asyncio.fixture
async def created_status(status_create):
    response = await status_create(name="Task status", position=20)
    return response["data"]


@pytest_asyncio.fixture
async def another_created_status(status_create):
    response = await status_create(name="Task review", position=100)
    return response["data"]


@pytest.fixture(name="status_edit")
def _status_edit(service_client, auth_headers, created_status):
    async def _inner(
            status_code=200,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.patch(
                STATUS_EDIT_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "status_id": created_status["id"],
                    "name": "Updated status",
                    "position": 30,
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.patch(
                STATUS_EDIT_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture(name="status_delete")
def _status_delete(service_client, auth_headers, created_status):
    async def _inner(
            status_code=204,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.request(
                "DELETE",
                STATUS_DELETE_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "status_id": created_status["id"],
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.request(
                "DELETE",
                STATUS_DELETE_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture(name="status_get_all")
def _status_get_all(service_client, auth_headers, created_board):
    async def _inner(
            status_code=200,
            headers=None,
            params=None,
            board_id=None,
            omit_board_id=False,
    ):
        request_headers = auth_headers if headers is None else headers

        if params is None:
            if omit_board_id:
                request_params = {}
            else:
                request_params = {
                    "board_id": created_board["board_id"] if board_id is None else board_id,
                }
        else:
            request_params = params

        response = await service_client.get(
            STATUS_GET_ALL_URL,
            headers=request_headers,
            params=request_params,
        )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture(name="task_create")
def _task_create(service_client, auth_headers, created_board, created_status):
    async def _inner(
            status_code=200,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.post(
                TASK_CREATE_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "board_id": created_board["board_id"],
                    "title": "Write lecture notes",
                    "description": "Prepare notes for the next seminar",
                    "status_id": created_status["id"],
                    "priority_color": "blue",
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.post(
                TASK_CREATE_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest_asyncio.fixture
async def created_task(task_create):
    response = await task_create()
    task = response["data"]

    return {
        "task": task,
        "task_id": task["id"],
    }


@pytest.fixture(name="task_edit")
def _task_edit(service_client, auth_headers, created_task, another_created_status):
    async def _inner(
            status_code=200,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.patch(
                TASK_EDIT_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "task_id": created_task["task_id"],
                    "title": "Updated lecture notes",
                    "description": "Polish notes before publishing",
                    "status_id": another_created_status["id"],
                    "priority_color": "red",
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.patch(
                TASK_EDIT_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture(name="task_delete")
def _task_delete(service_client, auth_headers, created_task):
    async def _inner(
            status_code=204,
            headers=None,
            raw_body=None,
            json=None,
            omit_fields=(),
            **kwargs,
    ):
        request_headers = auth_headers if headers is None else headers

        if raw_body is not None:
            response = await service_client.request(
                "DELETE",
                TASK_DELETE_URL,
                headers={
                    **request_headers,
                    "Content-Type": "application/json",
                },
                content=raw_body,
            )
        else:
            body = json if json is not None else build_json(
                default_body={
                    "task_id": created_task["task_id"],
                },
                omit_fields=omit_fields,
                overrides=kwargs,
            )

            response = await service_client.request(
                "DELETE",
                TASK_DELETE_URL,
                headers=request_headers,
                json=body,
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner
