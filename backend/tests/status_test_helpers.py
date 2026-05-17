import uuid


def create_user(session, register_url, login_url, email_prefix="status-user"):
    suffix = uuid.uuid4().hex
    email = f"{email_prefix}-{suffix}@example.com"
    password = "12345678"

    register_response = session.post(
        register_url,
        json={
            "email": email,
            "name": f"Status User {suffix[:8]}",
            "status": "student",
            "password": password,
        },
    )
    assert register_response.status_code == 200, register_response.text

    login_response = session.post(
        login_url,
        json={
            "email": email,
            "password": password,
        },
    )
    assert login_response.status_code == 200, login_response.text

    token = login_response.json()["data"]["token"]
    return {
        "email": email,
        "password": password,
        "token": token,
        "headers": {"Authorization": f"Bearer {token}"},
    }


def get_first_board_id(session, board_get_all_url, headers):
    response = session.get(board_get_all_url, headers=headers)
    assert response.status_code == 200, response.text

    boards = response.json()["data"]
    assert boards, "Expected at least one board after registration"
    return boards[0]["id"]


def get_statuses(session, status_get_all_url, headers, board_id):
    response = session.get(
        status_get_all_url,
        headers=headers,
        params={"board_id": board_id},
    )
    assert response.status_code == 200, response.text
    return response.json()["data"]
