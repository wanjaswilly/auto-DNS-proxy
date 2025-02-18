from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from datetime import datetime

db = SQLAlchemy()

# User model
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(50), unique=True, nullable=False)
    password = db.Column(db.String(200), nullable=False)
    
    def check_password(self, password):
        return check_password_hash(self.password, password)

    def __repr__(self):
        return f"<User {self.username}>"
    
class Resources(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    domain = db.Column(db.String(255), nullable=False, index=True)
    query_type = db.Column(db.String(50), nullable=False)  # A, AAAA, MX, etc.
    answer_section = db.Column(db.Text, nullable=True)  # Stores answer records
    authority_section = db.Column(db.Text, nullable=True)  # Name servers
    additional_section = db.Column(db.Text, nullable=True)  # Extra records
    query_time = db.Column(db.Float, nullable=False)  # Query time in ms
    dns_server = db.Column(db.String(100), nullable=False, default="8.8.8.8")
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)  # Time of query

# Proxy Settings Model
class ProxySettings(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    proxy_host = db.Column(db.String(255), nullable=False)  # IP or domain
    proxy_port = db.Column(db.Integer, nullable=False)  # Port number
    proxy_type = db.Column(db.String(50), nullable=False, default="HTTP")  # HTTP, HTTPS, SOCKS5
    requires_auth = db.Column(db.Boolean, default=False)  # Does it require authentication?
    username = db.Column(db.String(100), nullable=True)  # Username if required
    password = db.Column(db.String(100), nullable=True)  # Password if required
    created_at = db.Column(db.DateTime, default=datetime.utcnow)  # Timestamp of creation

    def __repr__(self):
        return f"<Proxy {self.proxy_host}:{self.proxy_port} ({self.proxy_type})>"

# Logs Model
class Logs(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    event_type = db.Column(db.String(50), nullable=False)  # INFO, ERROR, WARNING
    description = db.Column(db.Text, nullable=False)  # Details about the event
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)  # When it happened

    def __repr__(self):
        return f"<Log {self.event_type} - {self.timestamp}>"


