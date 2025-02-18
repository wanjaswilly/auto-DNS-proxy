import subprocess
from flask import Flask, render_template, redirect, url_for, request, session
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from models import db, User, Resources, Logs, ProxySettings
from utilities import utilities


app = Flask(__name__)

# app config
app.config["SQLALCHEMY_DATABASE_URI"] = "sqlite:///database.db"
app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

app.secret_key = "worrynotforthefutureisnow"

# initialize app
db.init_app(app)

# Routes
@app.route('/')
def home():
    if 'user' in session:
        
        users_count = User.query.count()
        resources_count = Resources.query.count()
        return render_template('dashboard.html', users_count=users_count, resources_count=resources_count)
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
    return render_template('resources.html', resources=Resources.query.all())

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == "POST":
        username = request.form['username']
        password = request.form['password']
        """Registers a new user with a hashed password."""
        if User.query.filter_by(username=username).first():
            return {"error": "Username already exists!"}

        hashed_password = generate_password_hash(password)

        new_user = User(username=username, password=hashed_password)
        db.session.add(new_user)
        db.session.commit()

        return render_template('login.html', message= "User registered successfully!")
    
    return render_template('register.html')
 
@app.route('/users')
def users():
     return render_template('users.html', users=User.query.all())
 
@app.route('/logs')
def logs():
    return render_template('logs.html', logs=Logs.query.all())

@app.route('/proxy')
def proxy():
    return render_template('proxy.html', proxy=ProxySettings.query.all())


if __name__ == '__main__':
    app.run(debug=True)
