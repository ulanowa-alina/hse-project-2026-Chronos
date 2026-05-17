import pytest
import requests

from tests.status_test_helpers import create_user, get_first_board_id, get_statuses


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
def board_id(session, board_get_all_url, auth_user):
    return get_first_board_id(session, board_get_all_url, auth_user["headers"])


@pytest.fixture
def board_statuses(session, status_get_all_url, auth_user, board_id):
    return get_statuses(session, status_get_all_url, auth_user["headers"], board_id)
