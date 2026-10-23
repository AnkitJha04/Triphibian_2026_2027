@echo off
cd /d "%~dp0"
python -m pip install -r requirements-web.txt
python bridge.py
