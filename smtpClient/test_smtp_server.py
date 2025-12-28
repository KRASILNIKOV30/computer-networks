#!/usr/bin/env python3
import socket
import sys
import time

def smtp_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind(('127.0.0.1', 1025))
        server_socket.listen(1)
        print('SMTP сервер запущен на 127.0.0.1:1025')

        client_socket, addr = server_socket.accept()
        print(f'Подключение от {addr}')

        client_file = client_socket.makefile('rw', buffering=1, encoding='utf-8', newline='\r\n')

        time.sleep(0.1)
        client_file.write('220 localhost SMTP server ready\r\n')
        client_file.flush()

        while True:
            command_line = client_file.readline()
            if not command_line:
                break

            command = command_line.strip()
            print(f'Команда: {command}')

            if command.startswith('EHLO') or command.startswith('HELO'):
                client_file.write('250 Hello client\r\n')
                client_file.flush()
            elif command.startswith('MAIL FROM'):
                client_file.write('250 OK\r\n')
                client_file.flush()
            elif command.startswith('RCPT TO'):
                client_file.write('250 OK\r\n')
                client_file.flush()
            elif command.startswith('DATA'):
                client_file.write('354 Start mail input\r\n')
                client_file.flush()

                while True:
                    line = client_file.readline()
                    if not line:
                        break
                    print(f'Данные: {line.strip()}')
                    if line.strip() == '.':
                        break

                client_file.write('250 OK\r\n')
                client_file.flush()
            elif command.startswith('QUIT'):
                client_file.write('221 Bye\r\n')
                client_file.flush()
                break
            elif command.startswith('STARTTLS'):
                client_file.write('502 Command not implemented\r\n')
                client_file.flush()
            else:
                client_file.write('500 Unknown command\r\n')
                client_file.flush()

        client_socket.close()
        print('SMTP сессия завершена')
    except Exception as e:
        print(f'Ошибка сервера: {e}')
    finally:
        server_socket.close()

if __name__ == '__main__':
    smtp_server()