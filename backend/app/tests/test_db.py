import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

import pytest
import Db
from sqlalchemy import text


def test_db_connection():
    """测试数据库连接是否可用"""
    with Db.OpenDb("test_connection") as db:
        result = db.execute(text("SELECT 1")).scalar()
    assert result == 1


def test_db_tables_exist_and_entries():
    """测试关键表存在并且可查询条目数量（>=0）"""
    expected_tables = [
        "users",
        "invitation_codes",
        "devices",
        "sensor_data",
        "firmwares",
        "device_logs",
    ]

    with Db.OpenDb("test_tables") as db:
        # 验证表结构存在
        existing = {
            r[0]
            for r in db.execute(
                text("SELECT name FROM sqlite_master WHERE type='table'")
            ).fetchall()
        }

        for table in expected_tables:
            assert table in existing, f"表 {table} 不存在"

        # 验证每张表能够正常查询且行数 >= 0
        for table in expected_tables:
            row_count = db.execute(text(f"SELECT COUNT(*) FROM {table}")).scalar()
            assert isinstance(row_count, int)
            assert row_count >= 0
