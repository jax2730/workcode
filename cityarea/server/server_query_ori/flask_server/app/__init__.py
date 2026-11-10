from flask import Flask
from config import config
def create_app():
    app = Flask(__name__)
    app.config.from_object('config.Config')

    from . import city_process
    app.register_blueprint(city_process.bp)

    return app