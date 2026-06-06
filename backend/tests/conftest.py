import uuid

import httpx
import pytest
import pytest_asyncio


BASE_URL = "http://127.0.0.1:8080"

AUTH_REGISTER_URL = "/auth/v1/register"
AUTH_LOGIN_URL = "/auth/v1/login"

BOARD_CREATE_URL = "/board/v1/create"


@pytest_asyncio.fixture
async def service_client():
    async with httpx.AsyncClient(base_url=BASE_URL) as client:
        yield client


@pytest_asyncio.fixture
async def auth_user(service_client):
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
            response = await service_client.post(
                BOARD_CREATE_URL,
                headers=request_headers,
                json=build_json(
                    default_body={
                        "title": "Study board",
                        "description": "Spring semester",
                        "is_private": False,
                    },
                    omit_fields=omit_fields,
                    overrides=kwargs,
                ),
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner
