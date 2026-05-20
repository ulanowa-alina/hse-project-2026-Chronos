import uuid

from status_test_helpers import create_user, get_first_board_id, get_statuses


def test_status_create_success(
    session,
    status_create_url,
    status_get_all_url,
    auth_user,
    board_id,
    board_statuses,
):
    initial_count = len(board_statuses)
    created_name = f"created-{uuid.uuid4().hex[:8]}"
    created_position = 5

    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={
            "board_id": board_id,
            "name": created_name,
            "position": created_position,
        },
    )

    assert response.status_code == 200

    body = response.json()
    assert body["data"]["id"] > 0
    assert body["data"]["board_id"] == board_id
    assert body["data"]["name"] == created_name
    assert body["data"]["position"] == created_position

    statuses_after_create = get_statuses(
        session, status_get_all_url, auth_user["headers"], board_id
    )
    assert len(statuses_after_create) == initial_count + 1

    created_status = next(
        status for status in statuses_after_create if status["id"] == body["data"]["id"]
    )
    assert created_status["name"] == created_name
    assert created_status["position"] == created_position


def test_status_create_requires_authorization(session, status_create_url):
    response = session.post(
        status_create_url,
        json={
            "board_id": 1,
            "name": "new-status",
            "position": 1,
        },
    )

    assert response.status_code == 401
    body = response.json()
    assert body["error"]["code"] == "UNAUTHORIZED"


def test_status_create_invalid_json(session, status_create_url, auth_user):
    response = session.post(
        status_create_url,
        data='{"board_id": 1, "name": "todo", "position": 0',
        headers={
            "Authorization": auth_user["headers"]["Authorization"],
            "Content-Type": "application/json",
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "INVALID_FORMAT"


def test_status_create_missing_required_fields(session, status_create_url, auth_user):
    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={"name": "new-status"},
    )

    assert response.status_code == 400

    body = response.json()
    assert body["error"]["code"] == "MISSING_FIELD"
    assert set(body["error"]["details"]["missing_fields"]) == {"board_id", "position"}


def test_status_create_invalid_board_id_format(session, status_create_url, auth_user):
    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={
            "board_id": "wrong",
            "name": "new-status",
            "position": 1,
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "INVALID_FORMAT"
    assert body["error"]["details"]["board_id"] == "Invalid board_id format"


def test_status_create_rejects_negative_position(session, status_create_url, auth_user, board_id):
    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={
            "board_id": board_id,
            "name": "new-status",
            "position": -1,
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "VALIDATION_ERROR"
    assert body["error"]["details"]["position"] == "Position must be greater than or equal to 0"


def test_status_create_rejects_empty_name(session, status_create_url, auth_user, board_id):
    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={
            "board_id": board_id,
            "name": "",
            "position": 1,
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "VALIDATION_ERROR"
    assert body["error"]["details"]["name"] == "Name length must be between 1 and 50 symbols"


def test_status_create_returns_not_found_for_unknown_board(session, status_create_url, auth_user):
    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={
            "board_id": 999999999,
            "name": "new-status",
            "position": 1,
        },
    )

    assert response.status_code == 404
    body = response.json()
    assert body["error"]["code"] == "BOARD_NOT_FOUND"


def test_status_create_rejects_duplicate_name_on_same_board(
    session,
    status_create_url,
    auth_user,
    board_id,
    board_statuses,
):
    existing_status = board_statuses[0]

    response = session.post(
        status_create_url,
        headers=auth_user["headers"],
        json={
            "board_id": board_id,
            "name": existing_status["name"],
            "position": 10,
        },
    )

    assert response.status_code == 405
    body = response.json()
    assert body["error"]["code"] == "DUPLICATE_RESOURCE"
    assert body["error"]["details"]["name"] == "already exists"


def test_status_create_forbids_access_to_another_users_board(
    session,
    register_url,
    login_url,
    status_create_url,
    board_get_all_url,
):
    owner = create_user(session, register_url, login_url, email_prefix="board-owner")
    intruder = create_user(session, register_url, login_url, email_prefix="board-intruder")

    owner_board_id = get_first_board_id(session, board_get_all_url, owner["headers"])

    response = session.post(
        status_create_url,
        headers=intruder["headers"],
        json={
            "board_id": owner_board_id,
            "name": f"intruder-{uuid.uuid4().hex[:8]}",
            "position": 2,
        },
    )

    assert response.status_code == 403
    body = response.json()
    assert body["error"]["code"] == "RESOURCE_NOT_OWNED"
