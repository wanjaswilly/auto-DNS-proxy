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
        return check_password_hash(self.password_hash, password)

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
