from __future__ import annotations

import os
import sys
import threading
from pathlib import Path

os.environ.setdefault("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu-sandbox")

from PySide6.QtCore import QUrl, Qt
from PySide6.QtGui import QColor, QPalette
from PySide6.QtWidgets import QApplication, QMainWindow, QMessageBox
from PySide6.QtWebEngineCore import QWebEngineSettings
from PySide6.QtWebEngineWidgets import QWebEngineView

from bridge import make_server, controller


class CommandDeck(QMainWindow):
    def __init__(self, server):
        super().__init__()
        self.server = server
        self.setWindowTitle("RIFTWALKER · Triphibious Command Deck")
        self.resize(1560, 980)
        self.setMinimumSize(1050, 700)
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor("#060b11"))
        palette.setColor(QPalette.ColorRole.WindowText, QColor("#e9f5f7"))
        self.setPalette(palette)
        self.view = QWebEngineView(self)
        settings = self.view.settings()
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalStorageEnabled, False)
        settings.setAttribute(QWebEngineSettings.WebAttribute.WebGLEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.Accelerated2dCanvasEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.ScrollAnimatorEnabled, True)
        self.setCentralWidget(self.view)
        host, port = server.server_address[:2]
        self.view.load(QUrl(f"http://{host}:{port}/"))

    def closeEvent(self, event):
        # Stop ESC output before closing if the controller still answers.
        try:
            status = controller.status()
            if status.get("connected") and (status.get("telemetry") or {}).get("armed", False):
                controller.command("S")
                import time
                time.sleep(0.12)
            controller.disconnect()
        except Exception:
            pass
        try:
            threading.Thread(target=self.server.shutdown, daemon=True).start()
        except Exception:
            pass
        super().closeEvent(event)


def main() -> int:
    QApplication.setAttribute(Qt.ApplicationAttribute.AA_ShareOpenGLContexts, True)
    app = QApplication(sys.argv)
    app.setApplicationName("Riftwalker Triphibious Command Deck")
    app.setStyle("Fusion")
    server = make_server("127.0.0.1", 0)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    window = CommandDeck(server)
    window.show()
    code = app.exec()
    controller.disconnect()
    server.shutdown()
    server.server_close()
    return code


if __name__ == "__main__":
    raise SystemExit(main())
