from __future__ import annotations

import os
import socket
import sys
import threading
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

os.environ.setdefault("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu-sandbox")

from PySide6.QtCore import QUrl, Qt, Slot
from PySide6.QtGui import QColor, QPalette
from PySide6.QtWidgets import QApplication, QMainWindow, QMessageBox

try:
    from PySide6.QtWebEngineCore import QWebEnginePage, QWebEngineSettings
    from PySide6.QtWebEngineWidgets import QWebEngineView
except ImportError:
    print("Qt WebEngine is missing. Install with: python -m pip install PySide6 PySide6-Addons")
    raise


APP_DIR = Path(__file__).resolve().parent
ROOT = APP_DIR.parent
DECK_NAME = "TRIPHIBIOUS_Command_Deck_FINAL_FIXED.html"


def find_deck() -> Path:
    candidates = [
        ROOT / DECK_NAME,
        APP_DIR / DECK_NAME,
        APP_DIR / "index.html",
    ]
    for path in candidates:
        if path.is_file() and path.stat().st_size > 100_000:
            return path
    raise FileNotFoundError(
        f"Could not find {DECK_NAME}. Place it next to the Application folder."
    )


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


class QuietHandler(SimpleHTTPRequestHandler):
    extensions_map = {
        **SimpleHTTPRequestHandler.extensions_map,
        ".html": "text/html; charset=utf-8",
        ".js": "text/javascript; charset=utf-8",
        ".json": "application/json; charset=utf-8",
    }

    def end_headers(self):
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

    def log_message(self, format, *args):
        return


class DeckPage(QWebEnginePage):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.console_errors: list[str] = []

    def javaScriptConsoleMessage(self, level, message, lineNumber, sourceID):
        text = f"{sourceID}:{lineNumber} {message}"
        if level in (
            QWebEnginePage.JavaScriptConsoleMessageLevel.ErrorMessageLevel,
            QWebEnginePage.JavaScriptConsoleMessageLevel.WarningMessageLevel,
        ):
            self.console_errors.append(text)
            print(f"[deck] {text}", file=sys.stderr)


class CommandDeck(QMainWindow):
    def __init__(self, deck: Path, server: ThreadingHTTPServer):
        super().__init__()
        self.deck = deck
        self.server = server
        self.setWindowTitle("RIFTWALKER // TRIPHIBIOUS COMMAND DECK")
        self.resize(1540, 960)
        self.setMinimumSize(1100, 700)

        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor("#02070c"))
        palette.setColor(QPalette.ColorRole.WindowText, QColor("#e8f6fa"))
        self.setPalette(palette)

        self.view = QWebEngineView(self)
        self.page = DeckPage(self.view)
        self.view.setPage(self.page)
        settings = self.view.settings()
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessFileUrls, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalStorageEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.WebGLEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.Accelerated2dCanvasEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.ScrollAnimatorEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.ErrorPageEnabled, True)
        self.setCentralWidget(self.view)

        host, port = server.server_address[:2]
        url = QUrl(f"http://{host}:{port}/{deck.name}")
        self.view.loadFinished.connect(self._on_loaded)
        self.view.load(url)

    @Slot(bool)
    def _on_loaded(self, ok: bool):
        if not ok:
            QMessageBox.critical(
                self,
                "Command Deck",
                f"Failed to load {self.deck.name}. Check that the HTML file is intact.",
            )
            return
        self.page.runJavaScript(
            "document.title || 'RIFTWALKER // TRIPHIBIOUS COMMAND DECK'"
        )

    def closeEvent(self, event):
        try:
            self.server.shutdown()
        except Exception:
            pass
        super().closeEvent(event)


def start_server(root: Path) -> ThreadingHTTPServer:
    handler = partial(QuietHandler, directory=str(root))
    server = ThreadingHTTPServer(("127.0.0.1", free_port()), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server


def main() -> int:
    QApplication.setAttribute(Qt.ApplicationAttribute.AA_ShareOpenGLContexts, True)
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )
    try:
        deck = find_deck()
    except FileNotFoundError as exc:
        app = QApplication(sys.argv)
        QMessageBox.critical(None, "Command Deck", str(exc))
        return 1

    server = start_server(deck.parent)
    app = QApplication(sys.argv)
    app.setApplicationName("Riftwalker Triphibious Command Deck")
    app.setStyle("Fusion")
    print(f"Loading Command Deck: {deck}")
    window = CommandDeck(deck, server)
    window.show()
    code = app.exec()
    try:
        server.shutdown()
    except Exception:
        pass
    return code


if __name__ == "__main__":
    sys.exit(main())
