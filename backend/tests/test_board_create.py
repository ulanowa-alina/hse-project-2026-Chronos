import pytest

pytestmark = pytest.mark.asyncio

def assert_board_response(
        response,
        *,
        title = 'test board',
        description = 'test description',
        is_private = False,
):
    assert 'data' in response

    board = response['data']

    assert isinstance(board['id'], int)
    assert isinstance(board['user_id'], int)

    assert board['title'] == title
    assert board['description'] == description
    assert board['is_private'] is is_private

    assert isinstance(board['created_at'], str)
    assert isinstance(board['updated_at'], str)

    return board

def assert_error_response(
        response,
        code,
        field = None
):
    assert 'error' in response

    error = response['error']

    assert error['code'] == code
    assert isinstance(error['message'], str)
    assert error['message']

    if field is not None:
        assert 'details' in error
        assert field in error['details']


async def test_basic(board_create):
    response = await board_create()

    assert_board_response(response)

async def test_create_with_empty_title(board_create):
    response = await board_create(
        status_code=400,
        title='',
    )

    assert_error_response(response, 'VALIDATION_ERROR', field='title')

async def test_create_without_title(board_create):
    response = await board_create(
        status_code=400,
        json={
            'description': 'Test description',
            'is_private': False,
        },
    )

    assert_error_response(response, 'MISSING_FIELD', field='title')

async def test_create_with_too_long_title(board_create):
    response = await board_create(
        status_code=400,
        title='a' * 101,
    )

    assert_error_response(response, 'VALIDATION_ERROR', field='title')

async def test_create_with_too_long_description(board_create):
    response = await board_create(
        status_code=400,
        description='a' * 1001,
    )

    assert_error_response(response, 'VALIDATION_ERROR', field='description')

async def test_create_without_is_private(board_create):
    response = await board_create(
        status_code=400,
        json={
            'title': 'Test board',
            'description': 'Test description',
        },
    )

    assert_error_response(response, 'MISSING_FIELD', field='is_private')

async def test_create_with_invalid_is_private(board_create):
    response = await board_create(
        status_code=400,
        is_private='false',
    )

    assert_error_response(response, 'INVALID_FORMAT', field='is_private')

async def test_create_without_auth(board_create):
    response = await board_create(
        status_code=401,
        headers={},
    )

    assert_error_response(response, 'UNAUTHORIZED')
