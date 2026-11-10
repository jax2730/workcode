import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent

class Config:
    # -----------------------------
    # 基础配置
    # -----------------------------
    SECRET_KEY = os.environ.get("SECRET_KEY", "dev-secret-key")

    # -----------------------------
    # 数据库配置
    # -----------------------------
    # 默认使用 SQLite
    SQLALCHEMY_DATABASE_URI = os.environ.get(
        "DATABASE_URL",
        f"sqlite:///{BASE_DIR / 'data' / 'app.db'}"
    )
    SQLALCHEMY_TRACK_MODIFICATIONS = False

    # -----------------------------
    # 异步任务系统（你的 city_process 用）
    # -----------------------------
    TASK_DATA_DIR = BASE_DIR / "task_data"
    os.makedirs(TASK_DATA_DIR, exist_ok=True)

    # 子进程脚本路径
    CITY_PROCESS_SCRIPT = str(BASE_DIR / "city_process" / "query_city.py")

    # -----------------------------
    # 日志配置
    # -----------------------------
    LOG_LEVEL = os.environ.get("LOG_LEVEL", "INFO")


class DevelopmentConfig(Config):
    DEBUG = True
    ENV = "development"


class ProductionConfig(Config):
    DEBUG = False
    ENV = "production"
    # 生产环境必须设置 SECRET_KEY
    SECRET_KEY = os.environ.get("SECRET_KEY")


config = {
    "dev": DevelopmentConfig,
    "prod": ProductionConfig,
    "default": DevelopmentConfig
}