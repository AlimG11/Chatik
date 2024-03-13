#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "Ws2_32.lib")

SOCKET ConnectSock;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
const WORD conclr = 7;

void client();

struct User {
	std::string name;
	std::string password;
	WORD color;
};
std::vector<User> users;

int main(int argc, char* argv[]) {

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup failed!\n";
		return 1;
	}
	std::ifstream config(argv[1]);
	if (!config) {
		std::cout << "Couldnt open config file!\n";
		return -1;
	}
	nlohmann::json jsData;
	config >> jsData;

	std::string k = jsData["IP"];
	const char* IP = k.c_str();
	std::string p = jsData["Port"];
	u_short port = 0;
	for (int i = 0; i < p.size(); ++i) {
		port = port * 10 + (p[i] - '0');
	}

	SOCKADDR_IN addr{};
	int addrSize = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr(IP);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;


	User add;
	std::string clr;
	for (int i = 0; i < jsData["User"].size(); ++i) {
		add.name = jsData["User"][i]["Username"];
		add.password = jsData["User"][i]["Password"];
		add.color = 0;
		clr = jsData["User"][i]["Color"];
		for (int i = 0; i < clr.size(); ++i) {
			add.color = add.color * 10 + (clr[i] - '0');
		}
		users.push_back(add);
	}
	config.close();

	ConnectSock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(ConnectSock, (SOCKADDR*)&addr, addrSize) != 0) {
		std::cout << "Error: Failed to connect to the server\n";
		closesocket(ConnectSock);
		return 1;
	}
	else {
		std::cout << "Connect!\n";
	}
	char name[64]{ 0 };
	char password[64]{ 0 };
	bool find = false;
	while (true) {
		std::cout << "Enter you username: ";
		std::cin >> name;
		std::cout << "Enter you password: ";
		std::cin >> password;
		std::system("cls");
		std::cout << "Enter you username: " << name << "\nEnter you password: ";
		for (int i = 0; i < strlen(password); ++i) {
			std::cout << '*';
		}
		std::cout << '\n';

		send(ConnectSock, name, 64, 0);
		Sleep(100);
		send(ConnectSock, password, 64, 0);

		for (size_t i = 0; i < users.size(); ++i) {
			if (name == users[i].name && password == users[i].password) {
				find = true;
				std::cout << "You are logged in chat!\n";
				break;
			}
		}
		if (!find) {
			std::system("cls");
			std::cout << "Incorrect username or password!\n";
		}
		else {
			break;
		}
	}

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)client, NULL, NULL, NULL);

	char msg[64]{ 0 };
	while (true) {
		do {
			std::cin.getline(msg, 64);
			find = false;
			if (std::string(msg) == "Exit.") {
				send(ConnectSock, msg, 64, 0);
				std::cout << "You are exit the chat.\n";
				closesocket(ConnectSock);
				WSACleanup();
				return 0;
			}

			if (strlen(msg) != 0) {
				for (int i = 0; i < strlen(msg); ++i) {
					if ((unsigned char)msg[i] < 32) {
						find = true;
						std::cout << "Invalid input message: char >= 32!\n";
						break;
					}
				}

				if (!find && msg[strlen(msg) - 1] != '.' && msg[strlen(msg) - 1] != '!' && msg[strlen(msg) - 1] != '?') {
					find = true;
					std::cout << "The message must end in the following characters: '.' or '!' or '?'\n";
				}
			}
		} while (find);

		send(ConnectSock, msg, 64, 0);
	}

	shutdown(ConnectSock, SD_SEND);
	closesocket(ConnectSock);
	WSACleanup();
	return 0;
}

void client() {
	std::string msg;
	char tmp[65]{ 0 };
	WORD color;
	//бесконечный цикл, который смотрит сообщение от сервера и выводит его на экран
	while (true) {
		msg = "";
		recv(ConnectSock, tmp, 65, 0);
		Sleep(200);
		//std::cout << tmp << '\n';
		if (strlen(tmp) > 1) {
			if (tmp[1] == '\t') {
				for (int i = 0; i < strlen(tmp) - 1; ++i) {
					msg += tmp[i + 2];
				}

				color = (unsigned char)tmp[0] - 32;

				SetConsoleTextAttribute(hConsole, color);
				std::cout << msg << '\n';
				SetConsoleTextAttribute(hConsole, conclr);
				break;
			}
			for (int i = 0; i < strlen(tmp) - 1; ++i) {
				msg += tmp[i + 1];
			}

			color = (unsigned char)tmp[0] - 32;
			//std::cout << tmp << "|\n" << (int)tmp[1] << "|\n" << (int)tmp[0] << "|\nColor: " << color << "|\n";
			SetConsoleTextAttribute(hConsole, color);
			std::cout << msg << '\n';
			SetConsoleTextAttribute(hConsole, conclr);
		}
	}
}
