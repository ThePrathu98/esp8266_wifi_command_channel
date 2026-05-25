import sys
from datetime import datetime

from PyQt6.QtWidgets import (
    QApplication,
    QWidget,
    QLabel,
    QLineEdit,
    QPushButton,
    QTextEdit,
    QVBoxLayout,
    QHBoxLayout,
    QGridLayout,
)

from esp8266_client import send_command

class Esp8266Gui(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Project B - ESP8266 Wi-Fi Command GUI")
        self.setMinimumWidth(650)

        self.ip_input = QLineEdit("192.168.1.105")
        self.port_input = QLineEdit("5005")

        self.text_input = QLineEdit("Hello ALSO")
        self.number_input = QLineEdit("123")

        self.status_label = QLabel("Status: Not tested")

        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)

        self.test_button = QPushButton("Test Connection / GET_STATUS")
        self.show_text_button = QPushButton("SHOW_TEXT")
        self.show_number_button = QPushButton("SHOW_NUMBER")
        self.clear_button = QPushButton("CLEAR")
        self.invert_button = QPushButton("INVERT TEXT AND BACKGROUND COLORS")

        self.test_button.clicked.connect(self.send_get_status)
        self.show_text_button.clicked.connect(self.send_show_text)
        self.show_number_button.clicked.connect(self.send_show_number)
        self.clear_button.clicked.connect(lambda: self.send_and_log("CLEAR"))
        self.invert_button.clicked.connect(self.send_invert_display)

        self.build_layout()

    def build_layout(self):
        main_layout = QVBoxLayout()

        connection_layout = QGridLayout()
        connection_layout.addWidget(QLabel("ESP8266 IP Address:"), 0, 0)
        connection_layout.addWidget(self.ip_input, 0, 1)
        connection_layout.addWidget(QLabel("Port:"), 0, 2)
        connection_layout.addWidget(self.port_input, 0, 3)
        connection_layout.addWidget(self.test_button, 0, 4)

        main_layout.addLayout(connection_layout)
        main_layout.addWidget(self.status_label)

        text_layout = QHBoxLayout()
        text_layout.addWidget(QLabel("Text:"))
        text_layout.addWidget(self.text_input)
        text_layout.addWidget(self.show_text_button)

        number_layout = QHBoxLayout()
        number_layout.addWidget(QLabel("Number:"))
        number_layout.addWidget(self.number_input)
        number_layout.addWidget(self.show_number_button)

        button_layout = QHBoxLayout()
        button_layout.addWidget(self.clear_button)
        button_layout.addWidget(self.invert_button)

        main_layout.addLayout(text_layout)
        main_layout.addLayout(number_layout)
        main_layout.addLayout(button_layout)

        main_layout.addWidget(QLabel("Response Log:"))
        main_layout.addWidget(self.log_output)

        self.setLayout(main_layout)

    def get_connection_info(self):
        ip_address = self.ip_input.text().strip()
        port = int(self.port_input.text().strip())
        return ip_address, port

    def append_log(self, command: str, response: str):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_output.append(f"[{timestamp}] {command} -> {response}")

    def send_and_log(self, command: str):
        try:
            ip_address, port = self.get_connection_info()
        except ValueError:
            self.status_label.setText("Status: Invalid port number")
            self.append_log(command, "ERR INVALID_PORT")
            return

        response = send_command(ip_address, port, command)
        self.append_log(command, response)

        if response.startswith("OK"):
            self.status_label.setText("Status: Connected / Command OK")
        else:
            self.status_label.setText("Status: Command failed or connection issue")

    def send_get_status(self):
        self.send_and_log("GET_STATUS")

    def send_show_text(self):
        text = self.text_input.text().strip()

        if len(text) == 0:
            self.append_log("SHOW_TEXT", "ERR GUI_EMPTY_TEXT")
            self.status_label.setText("Status: Text input is empty")
            return

        self.send_and_log(f"SHOW_TEXT {text}")

    def send_show_number(self):
        number_text = self.number_input.text().strip()

        if len(number_text) == 0:
            self.append_log("SHOW_NUMBER", "ERR GUI_EMPTY_NUMBER")
            self.status_label.setText("Status: Number input is empty")
            return

        self.send_and_log(f"SHOW_NUMBER {number_text}")

    def send_invert_display(self):
        self.send_and_log("INVERT")


if __name__ == "__main__":
    app = QApplication(sys.argv)

    window = Esp8266Gui()
    window.show()

    sys.exit(app.exec())