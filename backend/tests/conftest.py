import pytest
import requests


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
