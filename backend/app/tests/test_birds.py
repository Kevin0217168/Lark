import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

import pytest
from fastapi import FastAPI
from fastapi.testclient import TestClient
from unittest.mock import MagicMock, patch
from datetime import datetime, timezone

from userapi.Bird import router as bird_router
import Security


@pytest.fixture
def app():
    """创建包含鸟鸟路由的 FastAPI 应用"""
    app = FastAPI()
    app.include_router(bird_router)
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


def create_mock_bird(bird_id=1, name="小白", species="鹦鹉", status="available"):
    """创建模拟的 Bird ORM 对象"""
    mock_bird = MagicMock()
    mock_bird.id = bird_id
    mock_bird.name = name
    mock_bird.species = species
    mock_bird.birth_date = datetime(2025, 1, 1)
    mock_bird.status = status
    mock_bird.area = "雏鸟区"
    mock_bird.number = 1
    mock_bird.description = "测试描述"
    mock_bird.avatar_url = None
    mock_bird.created_at = datetime(2025, 1, 1)
    mock_bird.updated_at = datetime(2025, 1, 1)
    return mock_bird


@pytest.fixture
def default_user(app):
    """创建默认登录用户模拟"""
    fake_user = MagicMock()
    fake_user.id = 1
    fake_user.username = "test_user"
    fake_user.role = "user"
    fake_user.extra = None
    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user
    return fake_user


@pytest.fixture
def root_user(app):
    """创建 root 用户模拟"""
    fake_user = MagicMock()
    fake_user.id = 1
    fake_user.username = "root_admin"
    fake_user.role = "root"
    fake_user.extra = None
    app.dependency_overrides[Security.GetCurrentUser] = lambda: fake_user
    return fake_user


@pytest.fixture
def normal_user(app):
    """创建普通用户模拟"""
    fake_user = MagicMock()
    fake_user.id = 2
    fake_user.username = "normal_user"
    fake_user.role = "user"
    fake_user.extra = None
    return fake_user


@pytest.fixture
def normal_user_with_bird(app):
    """创建已认领雏鸟的普通用户模拟"""
    fake_user = MagicMock()
    fake_user.id = 3
    fake_user.username = "user_with_bird"
    fake_user.role = "user"
    fake_user.extra = {"adopted_bird": {"bird_id": 10, "bird_name": "test_bird"}}
    return fake_user


# ---------- GET /api/birds (列表查询) ----------
@patch("userapi.Bird.Db.GetBirds")
def test_get_birds_list(mock_get_birds, client, default_user):
    """测试查询雏鸟列表（有数据）"""
    mock_bird1 = create_mock_bird(1, "小白", "鹦鹉", "available")
    mock_bird2 = create_mock_bird(2, "小黑", "鸽子", "adopted")
    mock_get_birds.return_value = [mock_bird1, mock_bird2]

    response = client.get("/birds")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200
    assert len(data["data"]) == 2
    assert data["data"][0]["name"] == "小白"
    assert data["data"][1]["name"] == "小黑"


@patch("userapi.Bird.Db.GetBirds")
def test_get_birds_list_with_filters(mock_get_birds, client, default_user):
    """测试查询雏鸟列表（带过滤条件）"""
    mock_get_birds.return_value = []

    response = client.get("/birds?species=鹦鹉&status=available")
    assert response.status_code == 200
    mock_get_birds.assert_called_once()


@patch("userapi.Bird.Db.GetBirds")
def test_get_birds_list_empty(mock_get_birds, client, default_user):
    """测试查询雏鸟列表（无数据）"""
    mock_get_birds.return_value = []

    response = client.get("/birds")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200
    assert data["data"] == []


# ---------- GET /api/birds/{id} ----------
@patch("userapi.Bird.Db.GetBirdById")
def test_get_bird_by_id(mock_get_bird_by_id, client, default_user):
    """测试根据ID获取单只雏鸟（成功）"""
    mock_bird = create_mock_bird(1, "小白", "鹦鹉", "available")
    mock_get_bird_by_id.return_value = mock_bird

    response = client.get("/birds/1")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200
    assert data["data"]["name"] == "小白"


@patch("userapi.Bird.Db.GetBirdById")
def test_get_bird_by_id_not_found(mock_get_bird_by_id, client, default_user):
    """测试根据ID获取单只雏鸟（不存在 -> 404）"""
    mock_get_bird_by_id.return_value = None

    response = client.get("/birds/999")
    assert response.status_code == 404
    data = response.json()
    assert data["code"] == 404
    assert data["msg"] == "雏鸟不存在"


# ---------- POST /api/birds (创建雏鸟) ----------
@patch("userapi.Bird.Db.CreateBird")
def test_create_bird_as_root(mock_create_bird, client, root_user):
    """测试 root 用户创建雏鸟（成功）"""
    mock_bird = create_mock_bird(1, "新雏鸟", "鹦鹉", "available")
    mock_create_bird.return_value = mock_bird

    response = client.post(
        "/birds",
        json={
            "name": "新雏鸟",
            "species": "鹦鹉",
            "birth_date": "2025-01-01",
            "area": "雏鸟区",
            "number": 1,
            "description": "可爱的雏鸟"
        }
    )
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200


def test_create_bird_as_normal_user(client, normal_user):
    """测试普通用户创建雏鸟（403）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    response = client.post(
        "/birds",
        json={
            "name": "非法创建",
            "species": "鹦鹉",
            "birth_date": "2025-01-01"
        }
    )
    assert response.status_code == 403


# ---------- PUT /api/birds/{id} ----------
@patch("userapi.Bird.Db.GetBirdById")
@patch("userapi.Bird.Db.UpdateBird")
def test_update_bird_as_root(mock_update_bird, mock_get_bird_by_id, client, root_user):
    """测试 root 用户更新雏鸟（成功）"""
    mock_existing = create_mock_bird(1, "原名", "鹦鹉", "available")
    mock_get_bird_by_id.return_value = mock_existing

    mock_updated = create_mock_bird(1, "更新后", "鹦鹉", "available")
    mock_update_bird.return_value = mock_updated

    response = client.put("/birds/1", json={"name": "更新后"})
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200


@patch("userapi.Bird.Db.UpdateBird")
def test_update_bird_not_found(mock_update_bird, client, root_user):
    """测试更新不存在的雏鸟（404）"""
    mock_update_bird.return_value = None

    response = client.put("/birds/999", json={"name": "不存在"})
    assert response.status_code == 404
    data = response.json()
    assert data["code"] == 404


def test_update_bird_as_normal_user(client, normal_user):
    """测试普通用户更新雏鸟（403）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    response = client.put("/birds/1", json={"name": "非法更新"})
    assert response.status_code == 403


# ---------- DELETE /api/birds/{id} ----------
@patch("userapi.Bird.Db.GetBirdById")
@patch("userapi.Bird.Db.DeleteBird")
def test_delete_bird_as_root(mock_delete_bird, mock_get_bird_by_id, client, root_user):
    """测试 root 用户删除雏鸟（成功）"""
    mock_existing = create_mock_bird(1, "待删除", "鹦鹉", "available")
    mock_get_bird_by_id.return_value = mock_existing
    mock_delete_bird.return_value = True

    response = client.delete("/birds/1")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200


@patch("userapi.Bird.Db.DeleteBird")
def test_delete_bird_not_found(mock_delete_bird, client, root_user):
    """测试删除不存在的雏鸟（404）"""
    mock_delete_bird.return_value = False

    response = client.delete("/birds/999")
    assert response.status_code == 404


def test_delete_bird_as_normal_user(client, normal_user):
    """测试普通用户删除雏鸟（403）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    response = client.delete("/birds/1")
    assert response.status_code == 403


# ---------- PATCH /api/birds/{id}/status ----------
@patch("userapi.Bird.Db.UpdateBirdStatus")
def test_update_bird_status_as_root(mock_update_bird_status, client, root_user):
    """测试 root 用户更新雏鸟状态（成功）"""
    mock_bird = create_mock_bird(1, "小白", "鹦鹉", "adopted")
    mock_update_bird_status.return_value = mock_bird

    response = client.patch("/birds/1/status?new_status=adopted")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200


def test_update_bird_status_as_normal_user(client, normal_user):
    """测试普通用户更新雏鸟状态（403）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    response = client.patch("/birds/1/status?new_status=adopted")
    assert response.status_code == 403


@patch("userapi.Bird.Db.UpdateBirdStatus")
def test_update_bird_status_invalid_status(mock_update_bird_status, client, root_user):
    """测试更新雏鸟状态（无效状态 -> 400）"""
    response = client.patch("/birds/1/status?new_status=invalid_status")
    assert response.status_code == 400


@patch("userapi.Bird.Db.UpdateBirdStatus")
def test_update_bird_status_not_found(mock_update_bird_status, client, root_user):
    """测试更新不存在雏鸟的状态（404）"""
    mock_update_bird_status.return_value = None

    response = client.patch("/birds/999/status?new_status=adopted")
    assert response.status_code == 404


# ---------- POST /api/birds/{id}/adopt (认领雏鸟) ----------
@patch("userapi.Bird.Db.AdoptBird")
def test_adopt_bird_success(mock_adopt_bird, client, normal_user):
    """测试普通用户认领雏鸟（成功）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    mock_adopt_bird.return_value = {
        "success": True,
        "adopted_bird": {
            "bird_id": 1,
            "bird_name": "小白",
            "bird_species": "鹦鹉"
        },
        "message": "认领成功"
    }

    response = client.post("/birds/1/adopt")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200
    assert data["data"]["message"] == "认领成功"


@patch("userapi.Bird.Db.AdoptBird")
def test_adopt_bird_already_adopted(mock_adopt_bird, client, normal_user):
    """测试普通用户重复认领（400）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    mock_adopt_bird.return_value = {
        "success": False,
        "message": "您已经认领了一只雏鸟，不能再认领"
    }

    response = client.post("/birds/1/adopt")
    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert "已经认领" in data["msg"]


@patch("userapi.Bird.Db.AdoptBird")
def test_adopt_bird_not_available(mock_adopt_bird, client, normal_user):
    """测试认领不可认领的雏鸟（400）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    mock_adopt_bird.return_value = {
        "success": False,
        "message": "该雏鸟不可认领"
    }

    response = client.post("/birds/1/adopt")
    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert "不可认领" in data["msg"]


@patch("userapi.Bird.Db.AdoptBird")
def test_adopt_bird_not_found(mock_adopt_bird, client, normal_user):
    """测试认领不存在的雏鸟（400）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    mock_adopt_bird.return_value = {
        "success": False,
        "message": "雏鸟不存在"
    }

    response = client.post("/birds/999/adopt")
    assert response.status_code == 400


# ---------- DELETE /api/birds/adopted/me (释放雏鸟) ----------
@patch("userapi.Bird.Db.ReleaseAdoptedBird")
def test_release_bird_success(mock_release_bird, client, normal_user_with_bird):
    """测试普通用户释放雏鸟（成功）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user_with_bird

    mock_release_bird.return_value = {
        "success": True,
        "released_bird": {"bird_id": 10},
        "message": "释放成功"
    }

    response = client.delete("/birds/adopted/me")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200


@patch("userapi.Bird.Db.ReleaseAdoptedBird")
def test_release_bird_not_adopted(mock_release_bird, client, normal_user):
    """测试普通用户释放未认领的雏鸟（400）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    mock_release_bird.return_value = {
        "success": False,
        "message": "您没有已认领的雏鸟"
    }

    response = client.delete("/birds/adopted/me")
    assert response.status_code == 400
    data = response.json()
    assert data["code"] == 400
    assert "没有已认领" in data["msg"]


# ---------- GET /api/birds/adopted/me ----------
@patch("userapi.Bird.Db.GetUserAdoptedBird")
def test_get_adopted_bird_success(mock_get_adopted_bird, client, normal_user_with_bird):
    """测试获取已认领的雏鸟（成功）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user_with_bird

    mock_get_adopted_bird.return_value = {
        "bird_id": 10,
        "bird_name": "test_bird"
    }

    response = client.get("/birds/adopted/me")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200
    assert data["data"]["adopted_bird"]["bird_id"] == 10


@patch("userapi.Bird.Db.GetUserAdoptedBird")
def test_get_adopted_bird_empty(mock_get_adopted_bird, client, normal_user):
    """测试获取未认领的雏鸟（返回空）"""
    app = client.app
    app.dependency_overrides[Security.GetCurrentUser] = lambda: normal_user

    mock_get_adopted_bird.return_value = {}

    response = client.get("/birds/adopted/me")
    assert response.status_code == 200
    data = response.json()
    assert data["code"] == 200
    assert data["data"]["adopted_bird"] == {}


# ---------- 边界条件测试 ----------
@patch("userapi.Bird.Db.CreateBird")
def test_create_bird_missing_required_field(mock_create_bird, client, root_user):
    """测试创建雏鸟（缺少必填字段 -> 422）"""
    response = client.post(
        "/birds",
        json={
            "name": "新雏鸟"
        }
    )
    assert response.status_code == 422


@patch("userapi.Bird.Db.CreateBird")
def test_create_bird_internal_error(mock_create_bird, client, root_user):
    """测试创建雏鸟（服务器内部错误 -> 500）"""
    mock_create_bird.side_effect = Exception("Database error")

    response = client.post(
        "/birds",
        json={
            "name": "新雏鸟",
            "species": "鹦鹉",
            "birth_date": "2025-01-01"
        }
    )
    assert response.status_code == 500


def test_get_bird_unauthorized(client):
    """测试未登录访问（401）"""
    response = client.get("/birds")
    assert response.status_code == 401


def test_create_bird_unauthorized(client):
    """测试未登录创建（401）"""
    response = client.post(
        "/birds",
        json={
            "name": "新雏鸟",
            "species": "鹦鹉",
            "birth_date": "2025-01-01"
        }
    )
    assert response.status_code == 401
