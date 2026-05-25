import socket

def send_command(ip_address: str, port: int, command: str, timeout: float = 5.0) -> str:
    """
    Sends one command to the ESP8266 TCP server and returns its response.

    The firmware expects each command to end with newline '\n'.
    Example commands:
        GET_STATUS
        SHOW_TEXT Hello ALSO
        SHOW_NUMBER 123
        CLEAR
        INVERT
    """
    command = command.strip()
    message = command + "\n"

    try:
        with socket.create_connection((ip_address, port), timeout=timeout) as sock:
            sock.sendall(message.encode("utf-8"))
            response = sock.recv(1024).decode("utf-8", errors="replace").strip()
            return response

    except socket.timeout:
        return "ERR HOST_TIMEOUT"

    except ConnectionRefusedError:
        return "ERR CONNECTION_REFUSED"

    except OSError as error:
        return f"ERR HOST_SOCKET {error}"