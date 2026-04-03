import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

import pytest
from fastapi import FastAPI
from fastapi.testclient import TestClient
from unittest.mock import MagicMock, patch

from userapi.Login import router as login_router
import Db
import Security


@pytest.fixture
def app():
    app = FastAPI()
    app.include_router(login_router)
    return app


@pytest.fixture
def client(app):
    return TestClient(app)


@pytest.fixture
def fake_user():
    user = MagicMock(spec=Db.M_Users)
    user.username = "user1"
    user.id = 1
    user.nickname = "用户1"
    user.role = "user"
    user.email = "user1@example.com"
    user.avatar = None
    user.banner = None
    user.invitation_code = None
    return user


@patch("Db.GetUsers")
@patch("Db.VerifyPassword")
@patch("Security.CreateAccessToken")
def test_login_success(mock_create_access_token, mock_verify_password, mock_get_users, client):
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.username = "user1"
    fake_user.password = "hashed_password"
    mock_get_users.return_value = [fake_user]
    mock_verify_password.return_value = True
    mock_create_access_token.side_effect = ["token-abc", "cookie-token-xyz"]

    response = client.post(
        "/login",
        data={"username": "user1", "password": "secret12", "grant_type": ""},
    )

    assert response.status_code == 200
    json_data = response.json()
    assert json_data["access_token"] == "token-abc"
    assert json_data["token_type"] == "bearer"
    assert response.cookies.get("long_token") == "cookie-token-xyz"


@patch("Db.GetUsers")
@patch("Db.VerifyPassword")
def test_login_invalid_password(mock_verify_password, mock_get_users, client):
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.username = "user1"
    fake_user.password = "hashed_password"
    mock_get_users.return_value = [fake_user]
    mock_verify_password.return_value = False

    response = client.post(
        "/login",
        data={"username": "user1", "password": "wrongpass", "grant_type": ""},
    )

    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert data["msg"] == "Incorrect username or password"


@patch("Db.GetUsers")
def test_login_user_not_found(mock_get_users, client):
    mock_get_users.return_value = []

    response = client.post(
        "/login",
        data={"username": "unknown", "password": "secret12", "grant_type": ""},
    )

    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert data["msg"] == "Incorrect username or password"


def test_refresh_token_and_logout(client, app, fake_user):
    app.dependency_overrides[Security.GetCurrentUserByCookie] = lambda: fake_user

    client.cookies.set("long_token", "cookie-token-xyz")

    response = client.post("/refresh")
    assert response.status_code == 200
    data = response.json()
    assert data["token_type"] == "bearer"

    response = client.post("/logout")
    assert response.status_code == 200
    data = response.json()
    assert data["data"]["username"] == "user1"

    app.dependency_overrides.clear()
