import pytest
import requests
import time

from status_test_helpers import create_user, get_first_board_id, get_statuses


@pytest.fixture
def session():
    return requests.Session()


@pytest.fixture
def base_url():
    return "http://127.0.0.1:8080"


@pytest.fixture
def login_url(base_url):
    return f"{base_url}/auth/v1/login"


@pytest.fixture
def register_url(base_url):
    return f"{base_url}/auth/v1/register"


@pytest.fixture
def board_get_all_url(base_url):
    return f"{base_url}/board/v1/get_all"


@pytest.fixture
def board_create_url(base_url):
    return f"{base_url}/board/v1/create"


@pytest.fixture
def board_edit_url(base_url):
    return f"{base_url}/board/v1/edit"


@pytest.fixture
def board_delete_url(base_url):
    return f"{base_url}/board/v1/delete"


@pytest.fixture
def board_get_url(base_url):
    return f"{base_url}/board/v1/get"


@pytest.fixture
def status_edit_url(base_url):
    return f"{base_url}/status/v1/edit"


@pytest.fixture
def status_create_url(base_url):
    return f"{base_url}/status/v1/create"


@pytest.fixture
def status_get_all_url(base_url):
    return f"{base_url}/status/v1/get_all"


@pytest.fixture
def auth_user(session, register_url, login_url):
    return create_user(session, register_url, login_url)


@pytest.fixture
def other_auth_user(session, register_url, login_url):
    return create_user(session, register_url, login_url, email_prefix="other-user")


def json_or_none(response):
    if not response.text:
        return None
    return response.json()


def build_json(default_body, omit_fields, overrides):
    body = {**default_body, **overrides}
    for field in omit_fields:
        body.pop(field, None)
    return body


@pytest.fixture(name="board_create")
def _board_create(request, session, board_create_url):
    def _inner(status_code=200, headers=None, raw_body=None, omit_fields=(), **kwargs):
        request_headers = (
            request.getfixturevalue("auth_user")["headers"] if headers is None else headers
        )

        if raw_body is not None:
            response = session.post(
                board_create_url,
                headers={**request_headers, "Content-Type": "application/json"},
                data=raw_body,
            )
        else:
            response = session.post(
                board_create_url,
                headers=request_headers,
                json=build_json(
                    {
                        "title": "Study board",
                        "description": "Spring semester",
                        "is_private": False,
                    },
                    omit_fields,
                    kwargs,
                ),
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture
def created_board(board_create, auth_user):
    body = board_create()
    return {
        "owner": auth_user,
        "board": body["data"],
        "board_id": body["data"]["id"],
    }


@pytest.fixture(name="board_edit")
def _board_edit(session, board_edit_url, auth_user, created_board):
    def _inner(status_code=200, headers=None, raw_body=None, omit_fields=(), **kwargs):
        request_headers = auth_user["headers"] if headers is None else headers

        if raw_body is not None:
            response = session.patch(
                board_edit_url,
                headers={**request_headers, "Content-Type": "application/json"},
                data=raw_body,
            )
        else:
            response = session.patch(
                board_edit_url,
                headers=request_headers,
                json=build_json(
                    {
                        "board_id": created_board["board_id"],
                        "title": "Updated study board",
                        "description": "Updated spring semester",
                        "is_private": True,
                    },
                    omit_fields,
                    kwargs,
                ),
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture(name="board_delete")
def _board_delete(session, board_delete_url, auth_user, created_board):
    def _inner(status_code=204, headers=None, raw_body=None, omit_fields=(), **kwargs):
        request_headers = auth_user["headers"] if headers is None else headers

        if raw_body is not None:
            response = session.delete(
                board_delete_url,
                headers={**request_headers, "Content-Type": "application/json"},
                data=raw_body,
            )
        else:
            response = session.delete(
                board_delete_url,
                headers=request_headers,
                json=build_json(
                    {"board_id": created_board["board_id"]},
                    omit_fields,
                    kwargs,
                ),
            )

        assert response.status_code == status_code, response.text
        return json_or_none(response)

    return _inner


@pytest.fixture
def board_id(session, board_get_all_url, auth_user):
    return get_first_board_id(session, board_get_all_url, auth_user["headers"])


@pytest.fixture
def board_statuses(session, status_get_all_url, auth_user, board_id):
    return get_statuses(session, status_get_all_url, auth_user["headers"], board_id)


@pytest.fixture
def reg_user_data():
    code = time.time_ns()

    return {
        "name": f"user_{code}",
        "email": f"user_{code}@example.com",
        "status": "student",
        "password": f"password_{code}"
    }
