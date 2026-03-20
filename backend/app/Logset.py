import logging
from logging.handlers import TimedRotatingFileHandler, RotatingFileHandler
import os
import asyncio
from concurrent.futures import ThreadPoolExecutor

isDeploy = os.environ.get("FASTAPI_DEPLOY", None)
if isDeploy:
    log_path = "/home/mint/log/lark/fastapi"
else:
    log_path = "../log"

# 确保日志目录存在
os.makedirs(log_path, exist_ok=True)

# 创建格式化器
formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")

# app.log 仅记录 INFO 和 WARNING
info_handler = TimedRotatingFileHandler(
    log_path + "/app.log",
    when="midnight",
    interval=1,
    backupCount=7,
)
info_handler.setLevel(logging.INFO)
info_handler.setFormatter(formatter)

# 添加过滤器：只允许 INFO 和 WARNING 通过
class InfoWarningFilter(logging.Filter):
    def filter(self, record):
        return record.levelno in (logging.INFO, logging.WARNING)
info_handler.addFilter(InfoWarningFilter())


# error.log 仅记录 ERROR 及以上
error_handler = RotatingFileHandler(
    log_path + "/error.log",
    maxBytes=2*1024*1024,  # 2MB
    backupCount=10
)
error_handler.setLevel(logging.ERROR)
error_handler.setFormatter(formatter)


# 配置请求日志处理器
request_handler = TimedRotatingFileHandler(
    log_path + "/requests.log",
    when="midnight",
    interval=1,
    backupCount=7,
)
request_handler.setLevel(logging.INFO)
request_handler.setFormatter(formatter)


# 配置控制台处理器（仅测试环境）
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)  # 记录所有级别
console_handler.setFormatter(formatter)

# 配置日志记录器
logger = logging.getLogger("myapp")
if isDeploy:
    logger.setLevel(logging.INFO)         # 生产环境只记录 INFO 及以上
else:
    logger.setLevel(logging.DEBUG)        # 测试环境记录 DEBUG 及以上
logger.addHandler(info_handler)
logger.addHandler(error_handler)
if not isDeploy:
    logger.addHandler(console_handler)   # 测试环境加控制台

# 配置请求日志记录器
request_logger = logging.getLogger("request")
request_logger.setLevel(logging.INFO)
request_logger.addHandler(request_handler)

# 创建线程池用于日志处理
log_executor = ThreadPoolExecutor(max_workers=1)


# 通用异步日志记录函数
async def async_log(logger_obj, level, message, *args, **kwargs):
    def _log():
        getattr(logger_obj, level)(message, *args, **kwargs)
    loop = asyncio.get_event_loop()
    await loop.run_in_executor(log_executor, _log)
