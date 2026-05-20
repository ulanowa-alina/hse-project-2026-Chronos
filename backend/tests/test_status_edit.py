import uuid

from status_test_helpers import create_user, get_first_board_id, get_statuses


def test_status_edit_success(
    session,
    status_edit_url,
    status_get_all_url,
    auth_user,
    board_id,
    board_statuses,
):
    status_to_edit = board_statuses[0]
    updated_name = f"updated-{uuid.uuid4().hex[:8]}"
    updated_position = 7

    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={
            "status_id": status_to_edit["id"],
            "name": updated_name,
            "position": updated_position,
        },
    )

    assert response.status_code == 200

    body = response.json()
    assert body["data"]["id"] == status_to_edit["id"]
    assert body["data"]["board_id"] == board_id
    assert body["data"]["name"] == updated_name
    assert body["data"]["position"] == updated_position

    statuses_after_update = get_statuses(
        session, status_get_all_url, auth_user["headers"], board_id
    )
    updated_status = next(
        status for status in statuses_after_update if status["id"] == status_to_edit["id"]
    )

    assert updated_status["name"] == updated_name
    assert updated_status["position"] == updated_position


def test_status_edit_requires_authorization(session, status_edit_url):
    response = session.patch(
        status_edit_url,
        json={
            "status_id": 1,
            "name": "updated-name",
            "position": 1,
        },
    )

    assert response.status_code == 401
    body = response.json()
    assert body["error"]["code"] == "UNAUTHORIZED"


def test_status_edit_invalid_json(session, status_edit_url, auth_user):
    response = session.patch(
        status_edit_url,
        data='{"status_id": 1, "name": "todo", "position": 0',
        headers={
            "Authorization": auth_user["headers"]["Authorization"],
            "Content-Type": "application/json",
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "INVALID_FORMAT"


def test_status_edit_missing_required_fields(session, status_edit_url, auth_user):
    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={"name": "updated-name"},
    )

    assert response.status_code == 400

    body = response.json()
    assert body["error"]["code"] == "MISSING_FIELD"
    assert set(body["error"]["details"]["missing_fields"]) == {"status_id", "position"}


def test_status_edit_invalid_status_id_format(session, status_edit_url, auth_user):
    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={
            "status_id": "wrong",
            "name": "updated-name",
            "position": 1,
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "INVALID_FORMAT"
    assert body["error"]["details"]["status_id"] == "Invalid status_id format"


def test_status_edit_rejects_negative_position(
    session,
    status_edit_url,
    auth_user,
    board_statuses,
):
    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={
            "status_id": board_statuses[0]["id"],
            "name": "updated-name",
            "position": -1,
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "VALIDATION_ERROR"
    assert body["error"]["details"]["position"] == "Position must be greater than or equal to 0"


def test_status_edit_rejects_empty_name(session, status_edit_url, auth_user, board_statuses):
    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={
            "status_id": board_statuses[0]["id"],
            "name": "",
            "position": 1,
        },
    )

    assert response.status_code == 400
    body = response.json()
    assert body["error"]["code"] == "VALIDATION_ERROR"
    assert body["error"]["details"]["name"] == "Name length must be between 1 and 50 symbols"


def test_status_edit_returns_not_found_for_unknown_status(session, status_edit_url, auth_user):
    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={
            "status_id": 999999999,
            "name": "updated-name",
            "position": 1,
        },
    )

    assert response.status_code == 404
    body = response.json()
    assert body["error"]["code"] == "STATUS_NOT_FOUND"


def test_status_edit_rejects_duplicate_name_on_same_board(
    session,
    status_edit_url,
    auth_user,
    board_statuses,
):
    status_to_edit = board_statuses[0]
    existing_status = board_statuses[1]

    response = session.patch(
        status_edit_url,
        headers=auth_user["headers"],
        json={
            "status_id": status_to_edit["id"],
            "name": existing_status["name"],
            "position": status_to_edit["position"],
        },
    )

    assert response.status_code == 405
    body = response.json()
    assert body["error"]["code"] == "DUPLICATE_RESOURCE"
    assert body["error"]["details"]["name"] == "already exists"


def test_status_edit_forbids_access_to_another_users_status(
    session,
    register_url,
    login_url,
    status_edit_url,
    status_get_all_url,
    board_get_all_url,
):
    owner = create_user(session, register_url, login_url, email_prefix="status-owner")
    intruder = create_user(session, register_url, login_url, email_prefix="status-intruder")

    owner_board_id = get_first_board_id(session, board_get_all_url, owner["headers"])
    owner_statuses = get_statuses(session, status_get_all_url, owner["headers"], owner_board_id)
    status_to_edit = owner_statuses[0]

    response = session.patch(
        status_edit_url,
        headers=intruder["headers"],
        json={
            "status_id": status_to_edit["id"],
            "name": "intruder-update",
            "position": 3,
        },
    )

    assert response.status_code == 403
    body = response.json()
    assert body["error"]["code"] == "RESOURCE_NOT_OWNED"
