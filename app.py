import subprocess
from flask import Flask, render_template, redirect, url_for, request, session
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from models import db, User, Resources
from utilities import utilities


app = Flask(__name__)

# app config
app.config["SQLALCHEMY_DATABASE_URI"] = "sqlite:///database.db"
app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

# initialize app
db.init_app(app)

# Routes
@app.route('/')
def home():
    if 'user' in session:
        return render_template('dashboard.html')
    return render_template('login.html')

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        user = User.query.filter_by(username=username).first()
        if user and user.check_password(password):
            session['user'] = username
            return redirect(url_for('home'))
        return render_template('login.html', error = 'Invalid credentials')
    
    return render_template('login.html')

@app.route('/logout')
def logout():
    session.pop('user', None)
    return redirect(url_for('login'))

@app.route('/dashboard')
def dashboard():
    if 'user' in session:
        return render_template('dashboard.html', resources = Resources.query.all())
    return redirect(url_for('login'))

@app.route('/resources')
def resources():
    resources = get_network_resources()
    
    return render_template('resources.html')

@app.route('/register')
def register():
    username = request.form['username']
    password = request.form['password']
    """Registers a new user with a hashed password."""
    if User.query.filter_by(username=username).first():
        return {"error": "Username already exists!"}

    hashed_password = generate_password_hash(password)

    new_user = User(username=username, password_hash=hashed_password)
    db.session.add(new_user)
    db.session.commit()

    return {"message": "User registered successfully!"}
 


if __name__ == '__main__':
    app.run(debug=True)
