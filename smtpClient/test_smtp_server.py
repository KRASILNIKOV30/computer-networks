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
        
        # Приветствие с небольшой задержкой
        time.sleep(0.1)
        client_socket.send(b'220 localhost SMTP server ready\r\n')
        
        # Обрабатываем команды
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
                
            command = data.decode().strip()
            print(f'Команда: {command}')
            
            if command.startswith('HELO'):
                client_socket.send(b'250 Hello client\r\n')
            elif command.startswith('MAIL FROM'):
                client_socket.send(b'250 OK\r\n')
            elif command.startswith('RCPT TO'):
                client_socket.send(b'250 OK\r\n')
            elif command.startswith('DATA'):
                client_socket.send(b'354 Start mail input\r\n')
                
                # Читаем данные до точки
                while True:
                    line_data = client_socket.recv(1024)
                    if not line_data:
                        break
                    line = line_data.decode().strip()
                    print(f'Данные: {line}')
                    if line == '.':
                        break
                
                client_socket.send(b'250 OK\r\n')
            elif command.startswith('QUIT'):
                client_socket.send(b'221 Bye\r\n')
                break
            else:
                client_socket.send(b'500 Unknown command\r\n')
        
        client_socket.close()
        print('SMTP сессия завершена')
    except Exception as e:
        print(f'Ошибка сервера: {e}')
    finally:
        server_socket.close()

if __name__ == '__main__':
    smtp_server()