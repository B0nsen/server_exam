#include <winsock2.h>
#include <iostream>
#include <queue>
#include <string>
using namespace std;

#define MAX_CLIENTS 30
#define DEFAULT_BUFLEN 4096

#pragma comment(lib, "ws2_32.lib") // Winsock library
#pragma warning(disable:4996) // отключаем предупреждение _WINSOCK_DEPRECATED_NO_WARNINGS

SOCKET server_socket;

std::string menu[10] = {"hamburger", "coca-cola", "french-fries"};

int times[10] = { 5, 1 ,7 };

int price[10] = { 15, 5, 10 };

queue<string> history;

queue<int> OrderTimer;

queue<SOCKET> SocketQueue;

DWORD WINAPI CookingThread(LPVOID lp)
{
	while (true)
	{
		if (history.size() != 0)
		{
			Sleep(OrderTimer.front() * 1000);
			send(SocketQueue.front(), history.front().c_str(), strlen(history.front().c_str()), 0);
			OrderTimer.pop();
			history.pop();
			SocketQueue.pop();
		}
	}
	
	
}

int main() {
	system("title Server");
	puts("Start server... DONE.");
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed. Error Code: %d", WSAGetLastError());
		return 1;
	}

	// create a socket
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Could not create socket: %d", WSAGetLastError());
		return 2;
	}
	// puts("Create socket... DONE.");

	// prepare the sockaddr_in structure
	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);

	// bind socket
	if (bind(server_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed with error code: %d", WSAGetLastError());
		return 3;
	}

	// puts("Bind socket... DONE.");

	// слушать входящие соединения
	listen(server_socket, MAX_CLIENTS);

	// принять и входящее соединение
	puts("Server is waiting for incoming connections...\nPlease, start one or more client-side app.");

	// размер нашего приемного буфера, это длина строки
	// набор дескрипторов сокетов
	// fd means "file descriptors"
	fd_set readfds; // https://docs.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-fd_set
	SOCKET client_socket[MAX_CLIENTS] = {};
	HANDLE h = CreateThread(NULL, 0, CookingThread, NULL, 0, NULL);
	while (true) {
		// очистить сокет fdset
		FD_ZERO(&readfds);

		// добавить главный сокет в fdset
		FD_SET(server_socket, &readfds);

		// добавить дочерние сокеты в fdset
		for (int i = 0; i < MAX_CLIENTS; i++) 
		{
			SOCKET s = client_socket[i];
			if (s > 0) {
				FD_SET(s, &readfds);
			}
		}

		// дождитесь активности на любом из сокетов, тайм-аут равен NULL, поэтому ждите бесконечно
		if (select(0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR) {
			printf("select function call failed with error code : %d", WSAGetLastError());
			return 4;
		}

		// если что-то произошло на мастер-сокете, то это входящее соединение
		SOCKET new_socket; // новый клиентский сокет
		sockaddr_in address;
		int addrlen = sizeof(sockaddr_in);
		if (FD_ISSET(server_socket, &readfds)) {
			if ((new_socket = accept(server_socket, (sockaddr*)&address, &addrlen)) < 0) {
				perror("accept function error");
				return 5;
			}

			// информировать серверную сторону о номере сокета - используется в командах отправки и получения
			printf("New connection, socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

			// добавить новый сокет в массив сокетов
			char str[DEFAULT_BUFLEN];
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (client_socket[i] == 0) {
					client_socket[i] = new_socket;
					sprintf_s(str, DEFAULT_BUFLEN, "Welcome to macdonalds here is our menu\0");
					send(client_socket[i], str, strlen(str), 0);
					for (int j = 0; j < menu->size(); j++)
					{
						char str[DEFAULT_BUFLEN];
						sprintf_s(str, DEFAULT_BUFLEN, "%s %d$", menu[j].c_str(), price[j]);
						send(client_socket[i], str, strlen(str), 0);
					}
					printf("Adding to list of sockets at index %d\n", i);
					break;
				}
			}

		}

		// если какой-то из клиентских сокетов отправляет что-то
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			SOCKET s = client_socket[i];
			// если клиент присутствует в сокетах чтения
			if (FD_ISSET(s, &readfds))
			{
				// получить реквизиты клиента
				getpeername(s, (sockaddr*)&address, (int*)&addrlen);

				// проверьте, было ли оно на закрытие, а также прочитайте входящее сообщение
				// recv не помещает нулевой терминатор в конец строки (в то время как printf %s предполагает, что он есть)

				char client_message[DEFAULT_BUFLEN];

				int client_message_length = recv(s, client_message, DEFAULT_BUFLEN, 0);
				client_message[client_message_length] = '\0';

				char* word = new char[64];
				int j = 0;
				int timer = 0;
				int cost = 0;
				std::string order;
				for (int i = 0; i < strlen(client_message); i++)
				{
					client_message[i] = tolower(client_message[i]);
				}

				for (int i = 0; i < strlen(client_message); i++)
				{
					if (client_message[i] != ' ')
					{
						word[j] = client_message[i];
						j++;
					}
					else
					{
						for (int c = 0; c < menu->size(); c++)
						{
							word[j] = '\0';
							if (!strcmp(menu[c].c_str(), word))
							{
								timer += times[c];
								cost += price[c];
								order += word;
								order += ' ';
							}
							
						}
						j = 0;
						delete[] word;
						word = new char[64];
					}
				}
				
				OrderTimer.push(timer);
				char str[DEFAULT_BUFLEN];
				sprintf_s(str, "You order for %s was recieved. Its going to take %d seconds.", order.c_str(), timer);
				SocketQueue.push(client_socket[i]);
				send(s, str, strlen(str), 0);

				string check_exit = client_message;
				if (check_exit == "no")
				{
					client_socket[i] = 0;
				}

				//Sleep(timer * 1000);
				
				sprintf_s(str, "You order for %s has been made. Its going to cost %d$. Anything else?", order.c_str(), cost);
				history.push(str);

				//send(s, str, strlen(str), 0);
				}

			}
		}
	WSACleanup();
	TerminateThread(h, 0);
	CloseHandle(h);
}