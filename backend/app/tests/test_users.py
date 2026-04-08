import sys
from pathlib import Path
# 将当前文件的父目录（tests）的父目录（app）加入 sys.path
sys.path.insert(0, str(Path(__file__).parent.parent))

import pytest
from fastapi import FastAPI
from fastapi.testclient import TestClient
from unittest.mock import MagicMock, patch
from datetime import datetime, timedelta, timezone

# 导入待测试的路由和依赖模块
from userapi.User import router as user_router
from userapi.Login import router as login_router
import Db
import Security
# 假设 schema 中定义了 CommonOut 等模型，这里仅用于类型提示，非必需
from schema import CommonOut, UserItem, UserUpdateItem, UsersFilter


@pytest.fixture
def app():
    """创建包含用户路由的 FastAPI 应用"""
    app = FastAPI()
    app.include_router(user_router)
    app.include_router(login_router)
    return app


@pytest.fixture
def client(app):
    """返回测试客户端"""
    return TestClient(app)


@pytest.fixture(autouse=True)
def clear_dependency_overrides(app):
    """每个测试后自动清理依赖覆盖，防止测试间相互影响"""
    yield
    app.dependency_overrides.clear()


@pytest.fixture(autouse=True)
def default_authenticated_user(app):
    """给大多数测试设置默认登录用户，避免频繁 401"""
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.id = 1
    fake_user.username = "user1"
    fake_user.nickname = "用户1"
    fake_user.role = "user"
    fake_user.email = "user1@example.com"
    fake_user.avatar = None
    fake_user.banner = None
    fake_user.invitation_code = None
    fake_user.extra = None

    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user
    yield


# ---------- GET /users (列表查询) ----------
@patch("Db.GetUsers")
def test_get_users(mock_get_users, client, app):
    """测试查询用户列表（有数据）"""
    fake_users = [
        {
            "id": 1,
            "username": "user1",
            "nickname": "用户1",
            "role": "user",
            "email": "user1@example.com",
            "avatar": None,
            "banner": None,
            "invitation_code": None,
            "extra": None,
        },
        {
            "id": 2,
            "username": "user2",
            "nickname": "用户2",
            "role": "user",
            "email": "user2@example.com",
            "avatar": None,
            "banner": None,
            "invitation_code": None,
            "extra": None,
        },
    ]
    mock_get_users.return_value = fake_users

    response = client.get("/users?username=user")
    assert response.status_code == 200
    data = response.json()
    assert data["data"] == fake_users
    # 验证调用参数
    mock_get_users.assert_called_once()
    args, kwargs = mock_get_users.call_args
    assert kwargs.get("username") == "user"


@patch("Db.GetUsers")
def test_get_users_not_found(mock_get_users, client, app):
    """测试查询用户列表（无数据 -> 404）"""
    mock_get_users.return_value = []

    response = client.get("/users?username=nonexistent")
    assert response.status_code == 404
    data = response.json()
    assert data["code"] == 404
    assert data["msg"] == "User not found"


# ---------- GET /users/{id} ----------
@patch("Db.GetUsers")
def test_get_user_by_id(mock_get_users, client, app):
    """测试根据ID获取单个用户（成功）"""
    fake_user = [
        {
            "id": 1,
            "username": "user1",
            "nickname": "用户1",
            "role": "user",
            "email": "user1@example.com",
            "avatar": None,
            "banner": None,
            "invitation_code": None,
            "extra": None,
        }
    ]
    mock_get_users.return_value = fake_user

    response = client.get("/users/1")
    assert response.status_code == 200
    data = response.json()
    assert data["data"] == fake_user
    mock_get_users.assert_called_once()
    args, kwargs = mock_get_users.call_args
    assert kwargs.get("id") == 1


@patch("Db.GetUsers")
def test_get_user_by_id_not_found(mock_get_users, client, app):
    """测试根据ID获取单个用户（不存在 -> 404）"""
    mock_get_users.return_value = []

    response = client.get("/users/999")
    assert response.status_code == 404
    data = response.json()
    assert data["code"] == 404
    assert data["msg"] == "User not found"


# ---------- POST /users (注册) ----------
from datetime import datetime, timedelta

@patch("Db.RegisterUser")
@patch("Db.UseInvitationCode")
@patch("Db.GetInvitationCode")
@patch("Db.GetUsers")
def test_register_user(mock_get_users, mock_get_invitation_code, mock_use_invitation_code, mock_register_user, client, app):
    """测试注册新用户（成功）"""
    # 模拟用户名不存在
    mock_get_users.return_value = []
    mock_get_invitation_code.return_value = type(
        "I", (), {"remaining_uses": 1, "expire_at": datetime.now(timezone.utc) + timedelta(days=1)}
    )()
    mock_use_invitation_code.return_value = True
    new_user = {
        "id": 2,
        "username": "newuser",
        "nickname": "新用户",
        "role": "user",
        "email": "new@example.com",
        "avatar": None,
        "banner": None,
        "invitation_code": None,
        "extra": None,
    }
    mock_register_user.return_value = new_user

    response = client.post(
        "/users",
        json={
            "username": "newuser",
            "password": "secret12",
            "nickname": "新用户",
            "role": "user",
            "invitation_code": "ABC123456",
        },
    )
    assert response.status_code == 200
    data = response.json()
    assert data["data"] == new_user
    mock_get_users.assert_called_once()
    mock_register_user.assert_called_once()


@patch("Db.UseInvitationCode")
@patch("Db.GetInvitationCode")
@patch("Db.GetUsers")
def test_register_user_already_exists(mock_get_users, mock_get_invitation_code, mock_use_invitation_code, client, app):
    """测试注册时用户名已存在（400）"""
    mock_get_users.return_value = [{"id": 1, "username": "existing"}]
    mock_get_invitation_code.return_value = type(
        "I", (), {"remaining_uses": 1, "expire_at": datetime.now(timezone.utc) + timedelta(days=1)}
    )()
    mock_use_invitation_code.return_value = True

    response = client.post(
        "/users",
        json={
            "username": "existing",
            "password": "secret12",
            "nickname": "已存在用户",
            "role": "user",
            "invitation_code": "ABC123456",
        },
    )
    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert data["msg"] == "User already exist."


@patch("Db.GetInvitationCode")
@patch("Db.GetUsers")
def test_register_user_invalid_invitation(mock_get_users, mock_get_invitation_code, client, app):
    """测试注册失败：无效邀请码返回400"""
    mock_get_users.return_value = []
    mock_get_invitation_code.return_value = None

    response = client.post(
        "/users",
        json={
            "username": "newuser2",
            "password": "secret12",
            "nickname": "新用户2",
            "role": "user",
            "invitation_code": "INVALID456",
        },
    )

    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert data["msg"] == "邀请码不存在或已失效"


@patch("Db.GetInvitationCode")
@patch("Db.GetUsers")
def test_register_user_expired_invitation(mock_get_users, mock_get_invitation_code, client, app):
    """测试注册失败：邀请码已过期返回400"""
    mock_get_users.return_value = []
    mock_get_invitation_code.return_value = type(
        "I", (), {"remaining_uses": 1, "expire_at": datetime.now(timezone.utc) - timedelta(days=1)}
    )()

    response = client.post(
        "/users",
        json={
            "username": "newuser3",
            "password": "secret12",
            "nickname": "新用户3",
            "role": "user",
            "invitation_code": "EXPIRED999",
        },
    )

    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert data["msg"] == "邀请码不存在或已失效"


@patch("Db.GetInvitationCode")
@patch("Db.GetUsers")
def test_register_user_out_of_uses_invitation(mock_get_users, mock_get_invitation_code, client, app):
    """测试注册失败：邀请码可用次数已耗尽返回400"""
    mock_get_users.return_value = []
    mock_get_invitation_code.return_value = type(
        "I", (), {"remaining_uses": 0, "expire_at": datetime.now(timezone.utc) + timedelta(days=1)}
    )()

    response = client.post(
        "/users",
        json={
            "username": "newuser4",
            "password": "secret12",
            "nickname": "新用户4",
            "role": "user",
            "invitation_code": "OUTOFUSE456",
        },
    )

    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert data["msg"] == "邀请码不存在或已失效"


def test_register_user_invalid_payload(client, app):
    """测试注册失败：数据不符合 schema，返回 422"""
    response = client.post(
        "/users",
        json={
            "username": "no",
            "password": "short",
            "nickname": "",
            "role": "invalid",
            "invitation_code": "abc",
        },
    )

    assert response.status_code == 422


def test_get_users_me(client, app):
    """测试 /users/me 返回当前用户信息"""
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.id = 1
    fake_user.username = "user1"
    fake_user.nickname = "用户1"
    fake_user.role = "user"
    fake_user.email = "user1@example.com"
    fake_user.avatar = None
    fake_user.banner = None
    fake_user.invitation_code = None
    fake_user.extra = None

    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user

    response = client.get("/users/me")

    assert response.status_code == 200
    data = response.json()
    assert data["data"]["id"] == 1
    assert data["data"]["username"] == "user1"

    app.dependency_overrides.clear()


# ---------- PUT /users/{id} ----------
@patch("Db.UpdateUser")
def test_update_user(mock_update_user, client, app):
    """测试更新用户（成功）"""
    updated = {
        "id": 1,
        "username": "updated",
        "nickname": "已更新",
        "role": "user",
        "email": "updated@example.com",
        "avatar": None,
        "banner": None,
        "invitation_code": None,
        "extra": None,
    }
    mock_update_user.return_value = updated

    response = client.put("/users/1", json={"username": "updated"})
    assert response.status_code == 200
    data = response.json()
    assert data["data"] == updated
    mock_update_user.assert_called_once()
    args, kwargs = mock_update_user.call_args
    assert kwargs.get("id") == 1
    assert kwargs.get("username") == "updated"


@patch("Db.UpdateUser")
def test_update_user_not_found(mock_update_user, client, app):
    """测试更新不存在的用户（404）"""
    mock_update_user.return_value = None

    response = client.put("/users/999", json={"username": "updated"})
    assert response.status_code == 404
    data = response.json()
    assert data["code"] == 404
    assert data["msg"] == "User not found."


# ---------- DELETE /users/{id} ----------
def test_delete_user_as_root(client, app):
    """测试以 root 角色删除用户（成功）"""
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.role = "root"
    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user

    with patch("Db.DeleteUser") as mock_delete_user:
        mock_delete_user.return_value = {"id": 1}  # 返回被删除的用户对象
        response = client.delete("/users/1")
        assert response.status_code == 200
        data = response.json()
        assert data["data"] is None
        mock_delete_user.assert_called_once()


def test_delete_user_as_non_root(client, app):
    """测试以非 root 角色删除用户（403）"""
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.role = "user"
    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user

    with patch("Db.DeleteUser") as mock_delete_user:
        response = client.delete("/users/1")
        assert response.status_code == 403
        data = response.json()
        assert data["code"] == 403
        assert data["msg"] == "Permission denied."
        mock_delete_user.assert_not_called()


def test_delete_user_not_found(client, app):
    """测试删除不存在的用户（404）"""
    fake_user = MagicMock(spec=Db.M_Users)
    fake_user.role = "root"
    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user

    with patch("Db.DeleteUser") as mock_delete_user:
        mock_delete_user.return_value = None
        response = client.delete("/users/999")
        assert response.status_code == 404
        data = response.json()
        assert data["code"] == 404
        assert data["msg"] == "User not found."
        mock_delete_user.assert_called_once()