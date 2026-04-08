import re
from typing import Annotated, List, Optional, Literal, Generic, TypeVar, Dict, Any
from pydantic import BaseModel, Field, field_validator
from datetime import datetime

T = TypeVar("T")


class CommonOut(BaseModel, Generic[T]):
    code: int = Field(default=200)
    msg: str = Field(default="successful!")
    data: Optional[T] = None


class UsersFilter(BaseModel):
    model_config = {"extra": "forbid"}

    id: Optional[int] = Field(
        title="用户id", description="数据库用户唯一主键id", default=None
    )
    username: Optional[str] = Field(
        title="用户名", description="数据库唯一用户名", default=None
    )
    nickname: Optional[str] = Field(
        title="用户昵称", description="数据库用户昵称", default=None
    )
    role: Optional[Literal["root", "user", "clouduser"]] = Field(
        title="用户权限", description="数据库用户权限", default=None
    )
    email: Optional[str] = Field(
        title="用户邮箱", description="数据库用户邮箱", default=None
    )


class UserItem(BaseModel):
    username: str = Field(
        title="用户名",
        description="数据库唯一用户名",
        examples=["mint"],
        min_length=3,
        max_length=20,
        pattern=r"^[a-zA-Z0-9_]+$",  # 只允许字母、数字、下划线
    )
    password: str = Field(
        title="用户密码",
        description="用户密码",
        examples=["12345678a"],
        min_length=8,
        max_length=32,
    )
    nickname: str = Field(
        title="用户昵称",
        description="数据库用户昵称(不唯一)",
        examples=["猫猫"],
        min_length=1,
        max_length=50,
    )
    role: Literal["root", "user", "clouduser"] = Field(
        title="用户权限", description="数据库用户权限", examples=["root"]
    )
    avatar: Optional[str] = Field(
        title="用户头像",
        description="用户头像图片访问地址",
        examples=[""],
        default=None,
    )
    extra: Optional[Dict[str, Any]] = Field(
        title="额外信息",
        description="用户额外信息，字典类型",
        default=None,
    )
    invitation_code: str = Field(
        title="邀请码",
        description="注册邀请码",
        examples=["ABC123456"],
        min_length=9,
        max_length=11,
    )

    @field_validator("password")
    @classmethod
    def validate_password_strength(cls, v: Optional[str]) -> Optional[str]:
        if v is None:
            return v
        # 密码必须包含至少一个数字和一个字母
        if not re.search(r"\d", v):
            raise ValueError("密码必须包含至少一个数字")
        if not re.search(r"[a-zA-Z]", v):
            raise ValueError("密码必须包含至少一个字母")
        return v

    @field_validator("username")
    @classmethod
    def validate_username_format(cls, v: Optional[str]) -> Optional[str]:
        if v is None:
            return v
        # 用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾
        if not re.match(r"^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$", v):
            raise ValueError(
                "用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾"
            )
        return v

    @field_validator("avatar")
    @classmethod
    def validate_image_url(cls, v: Optional[str]) -> Optional[str]:
        if v == None or v == "":
            return v
        # 简单的 URL 格式校验（可扩展为更严格的检查）
        if not v.startswith(("http://", "https://")):
            raise ValueError("图片地址必须以 http:// 或 https:// 开头")
        # 可继续校验文件扩展名等
        return v


class UserUpdateItem(BaseModel):
    username: Optional[str] = Field(
        title="用户名", description="数据库唯一用户名", examples=["mint"], default=None
    )
    password: Optional[str] = Field(
        title="用户密码", description="用户密码", examples=["123456789"], default=None
    )
    nickname: Optional[str] = Field(
        title="用户昵称",
        description="数据库用户昵称(不唯一)",
        examples=["猫猫"],
        default=None,
    )
    role: Optional[Literal["root", "user", "clouduser"]] = Field(
        title="用户权限", description="数据库用户权限", examples=["root"], default=None
    )
    avatar: Optional[str] = Field(
        title="用户头像",
        description="用户头像图片访问地址",
        examples=[""],
        default=None,
    )
    banner: Optional[str] = Field(
        title="用户自定义主图",
        description="用户自定义主图图片访问地址",
        examples=[""],
        default=None,
    )
    email: Optional[str] = Field(
        title="用户邮箱",
        description="数据库用户邮箱",
        examples=["BI6PPD@163.com"],
        default=None,
    )
    extra: Optional[Dict[str, Any]] = Field(
        title="额外信息",
        description="用户额外信息，字典类型",
        default=None,
    )

    @field_validator("password")
    @classmethod
    def validate_password_strength(cls, v: Optional[str]) -> Optional[str]:
        if v is None:
            return v
        # 密码必须包含至少一个数字和一个字母
        if not re.search(r"\d", v):
            raise ValueError("密码必须包含至少一个数字")
        if not re.search(r"[a-zA-Z]", v):
            raise ValueError("密码必须包含至少一个字母")
        return v

    @field_validator("username")
    @classmethod
    def validate_username_format(cls, v: Optional[str]) -> Optional[str]:
        if v is None:
            return v
        # 用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾
        if not re.match(r"^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$", v):
            raise ValueError(
                "用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾"
            )
        return v

    @field_validator("avatar", "banner")
    @classmethod
    def validate_image_url(cls, v: Optional[str]) -> Optional[str]:
        if v is None:
            return v
        # 简单的 URL 格式校验（可扩展为更严格的检查）
        if not v.startswith(("http://", "https://")):
            raise ValueError("图片地址必须以 http:// 或 https:// 开头")
        # 可继续校验文件扩展名等
        return v


class LoginItem(BaseModel):
    username: str = Field(
        title="用户名", description="数据库唯一用户名", examples=["mint"]
    )
    password: str = Field(
        title="用户密码", description="用户密码", examples=["123456789"]
    )


R404_USER_NOT_FOUND = {
    404: {
        "model": CommonOut[None],
        "description": "User not found",
        "content": {
            "application/json": {"example": {"code": 404, "msg": "User not found"}}
        },
    }
}

R400_USER_ALREADY_EXIST = {
    400: {
        "model": CommonOut[None],
        "description": "User already exist.",
        "content": {
            "application/json": {"example": {"code": 400, "msg": "User already exist."}}
        },
    }
}

R400_USER_IS_NOT_UNIQUE = {
    400: {
        "model": CommonOut[None],
        "description": "User is not unique.",
        "content": {
            "application/json": {"example": {"code": 400, "msg": "User is not unique."}}
        },
    }
}

R400_LOGIN_INCORRECT = {
    400: {
        "model": CommonOut[None],
        "description": "Incorrect username or password.",
        "content": {
            "application/json": {
                "example": {"code": 400, "msg": "Incorrect username or password"}
            }
        },
    }
}

R200_LOGIN_SUCCESS = {
    200: {
        "description": "Login is successful.",
        "content": {
            "application/json": {
                "example": {"access_token": "token", "token_type": "bearer"}
            }
        },
    }
}

R403_FORBIDDEN = {
    403: {
        "description": "Permission denied.",
        "content": {
            "application/json": {"example": {"code": 403, "msg": "Permission denied."}}
        },
    }
}


class DeviceFilter(BaseModel):
    model_config = {"extra": "forbid"}

    id: Optional[int] = Field(
        default=None, title="设备ID", description="数据库设备唯一主键id"
    )
    name: Optional[str] = Field(default=None, title="设备名称", description="设备名称")
    device_type: Optional[str] = Field(
        default=None, title="设备类型", description="设备类型"
    )
    area: Optional[str] = Field(
        default=None, title="设备所在区域", description="设备所在区域"
    )
    number: Optional[int] = Field(
        default=None, title="区域内编号", description="区域内编号", ge=0
    )
    isOnline: Optional[bool] = Field(
        default=None, title="在线状态", description="在线状态"
    )
    status: Optional[Literal["stream", "standby", "error", "warning", "none"]] = Field(
        default=None, title="设备状态", description="设备状态"
    )


class DeviceItem(BaseModel):
    secret: str = Field(
        ...,
        min_length=1,
        title="设备密钥",
        description="设备密钥",
        examples=["b1f9562544a348c98c57a66b32a92d32"],
    )
    name: str = Field(
        ...,
        min_length=1,
        title="设备名称",
        examples=["ESP32"],
    )
    device_type: Optional[str] = Field(
        default=None, min_length=1, title="设备类型", examples=["ESP32-CAM"]
    )
    area: str = Field(..., min_length=1, title="设备所在区域", examples=["雏鸟区"])
    number: int = Field(..., ge=0, title="区域内编号", examples=[1])
    isOnline: Optional[bool] = Field(default=False, title="是否在线")
    status: Optional[Literal["stream", "standby", "error", "warning", "none"]] = Field(
        default="none", title="设备状态"
    )


class DeviceUpdateItem(BaseModel):
    name: Optional[str] = Field(default=None, min_length=1, title="设备名称")
    device_type: Optional[str] = Field(default=None, min_length=1, title="设备类型")
    area: Optional[str] = Field(default=None, min_length=1, title="设备所在区域")
    number: Optional[int] = Field(default=None, ge=0, title="区域内编号")
    isOnline: Optional[bool] = Field(default=None, title="是否在线")
    status: Optional[Literal["stream", "standby", "error", "warning", "none"]] = Field(
        default=None, title="设备状态"
    )


R404_DEVICE_NOT_FOUND = {
    404: {
        "model": CommonOut[None],
        "description": "Device not found",
        "content": {
            "application/json": {
                "example": {"code": 404, "msg": "Device not found", "data": None}
            }
        },
    }
}

R400_DEVICE_ALREADY_EXIST = {
    400: {
        "model": CommonOut[None],
        "description": "Device already exist.",
        "content": {
            "application/json": {
                "example": {"code": 400, "msg": "Device already exist.", "data": None}
            }
        },
    }
}


# ==================== 传感器数据模型 ====================
class SensorDataItem(BaseModel):
    """单条传感器数据"""

    id: int
    timestamp: datetime
    temperature: float
    humidity: float

    class Config:
        from_attributes = True


class DeviceSensorData(BaseModel):
    """设备分组传感器数据"""

    device_id: int
    data: List[SensorDataItem]


class SensorDataFilter(BaseModel):
    """传感器数据查询过滤条件"""

    model_config = {"extra": "forbid"}
    start_time: Optional[datetime] = Field(
        default=None, title="起始时间", description="查询起始时间（包含）"
    )
    end_time: Optional[datetime] = Field(
        default=None, title="结束时间", description="查询结束时间（包含）"
    )
    skip: int = Field(default=0, ge=0, title="跳过记录数")
    limit: int = Field(default=100, ge=1, le=1000, title="返回记录数")


class SensorDataCreate(BaseModel):
    """设备上报传感器数据"""

    secret: str = Field(..., title="设备密钥")
    temperature: float = Field(..., title="温度")
    humidity: float = Field(..., title="湿度")
    timestamp: Optional[datetime] = Field(
        default=None, title="数据时间戳", description="若不传则使用服务器当前时间"
    )


class SensorDataDelete(BaseModel):
    """删除传感器数据的条件"""

    device_id: int = Field(..., title="设备ID")
    start_time: Optional[datetime] = Field(None, title="起始时间")
    end_time: Optional[datetime] = Field(None, title="结束时间")


class StatisticData(BaseModel):
    """温度或湿度统计信息"""

    avg: float = Field(..., description="平均值")
    max: float = Field(..., description="最大值")
    min: float = Field(..., description="最小值")


class SummaryData(BaseModel):
    """数据统计摘要"""

    total: int = Field(..., description="统计的数据总条数")
    temperature: StatisticData = Field(..., description="温度统计")
    humidity: StatisticData = Field(..., description="湿度统计")


class GroupedDataItem(BaseModel):
    """分组获取传感器的返回结构"""

    group: int = Field(..., description="分组序号")
    start_time: datetime = Field(..., description="分组起始时间")
    end_time: datetime = Field(..., description="分组结束时间")
    total: int = Field(..., description="该分组内的数据条数")
    avg_temperature: float = Field(..., description="平均温度")
    avg_humidity: float = Field(..., description="平均湿度")


# ====================
R200_SENSOR_SUMMARY = {
    200: {
        "description": "成功返回统计摘要",
        "content": {
            "application/json": {
                "example": {
                    "code": 200,
                    "msg": "successful!",
                    "data": {
                        "total": 150,
                        "temperature": {"avg": 21.5, "max": 25.8, "min": 18.2},
                        "humidity": {"avg": 43.2, "max": 58.5, "min": 35.0},
                    },
                }
            }
        },
    }
}

R200_SENSOR_GROUP = {
    200: {
        "description": "成功返回分组统计数据",
        "content": {
            "application/json": {
                "example": {
                    "code": 200,
                    "msg": "successful!",
                    "data": [
                        {
                            "group": 0,
                            "start_time": "2025-03-26T10:00:00",
                            "end_time": "2025-03-26T10:30:00",
                            "total": 25,
                            "avg_temperature": 21.2,
                            "avg_humidity": 44.1,
                        },
                        {
                            "group": 1,
                            "start_time": "2025-03-26T10:30:00",
                            "end_time": "2025-03-26T11:00:00",
                            "total": 30,
                            "avg_temperature": 21.5,
                            "avg_humidity": 43.8,
                        },
                    ],
                }
            }
        },
    }
}

# ==================== 传感器错误响应 ====================
R404_SENSOR_NOT_FOUND = {
    404: {
        "model": CommonOut[None],
        "description": "Sensor data not found",
        "content": {
            "application/json": {
                "example": {"code": 404, "msg": "Sensor data not found", "data": None}
            }
        },
    }
}

R404_DEVICE_NOT_FOUND_BY_SECRET = {
    404: {
        "model": CommonOut[None],
        "description": "Device not found by secret",
        "content": {
            "application/json": {
                "example": {"code": 404, "msg": "Device not found", "data": None}
            }
        },
    }
}


# ==================== 固件管理模型 ====================
class FirmwareOut(BaseModel):
    id: int
    device_type: str
    name: str
    version: str
    filepath: str

    class Config:
        from_attributes = True


class FirmwareItem(BaseModel):
    device_type: str = Field(
        ..., min_length=1, title="设备类型", description="固件适用的设备类型", examples=["ESP32-CAM"]
    )
    name: str = Field(
        ..., min_length=1, title="固件名称", description="固件名称", examples=["lark_firmware"]
    )
    version: str = Field(
        ..., min_length=1, title="固件版本", description="固件版本号", examples=["1.0.0"]
    )


class FirmwareSecret(BaseModel):
    secret: str = Field(
        ..., min_length=1, title="设备密钥", description="设备密钥", examples=["b1f9562544a348c98c57a66b32a92d32"]
    )


R404_FIRMWARE_NOT_FOUND = {
    404: {
        "model": CommonOut[None],
        "description": "Firmware not found",
        "content": {
            "application/json": {
                "example": {"code": 404, "msg": "Firmware not found", "data": None}
            }
        },
    }
}

R400_FIRMWARE_ALREADY_EXIST = {
    400: {
        "model": CommonOut[None],
        "description": "Firmware already exist.",
        "content": {
            "application/json": {
                "example": {"code": 400, "msg": "Firmware already exist.", "data": None}
            }
        },
    }
}
