import pytest

pytestmark = pytest.mark.asyncio

def assert_board_response(
        response,
        *,
        title = 'test board',
        description = 'test description',
):
    assert 'data' in response

    board = response['data']

    assert isinstance(board['id'], int)
    assert isinstance(board['user_id'], int)

    assert board['title'] == title
    assert board['description'] == description

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
        },
    )

    assert_error_response(response, 'MISSING_FIELD', field='title')

async def test_create_with_too_long_title(board_create):
    response = await board_create(
        status_code=400,
        title='a' * 101,
    )

    assert_error_response(response, 'VALIDATION_ERROR', field='title')

async def test_create_with_max_title_length(board_create):
    response = await board_create(
        title='a' * 100,
    )

    assert_board_response(
        response,
        title='a' * 100,
    )

async def test_create_with_too_long_description(board_create):
    response = await board_create(
        status_code=400,
        description='a' * 1001,
    )

    assert_error_response(response, 'VALIDATION_ERROR', field='description')

async def test_create_with_max_description_length(board_create):
    response = await board_create(
        description='a' * 1000,
    )

    assert_board_response(
        response,
        description='a' * 1000,
    )

async def test_create_without_description(board_create):
    response = await board_create(
        omit_fields=('description',),
    )

    assert_board_response(
        response,
        description='',
    )

async def test_create_with_null_description(board_create):
    response = await board_create(
        description=None,
    )

    assert_board_response(
        response,
        description='',
    )

async def test_create_without_auth(board_create):
    response = await board_create(
        status_code=401,
        headers={},
    )

    assert_error_response(response, 'UNAUTHORIZED')
