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
#include <sstream>
#include <nlohmann/json.hpp>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996) 
std::vector<SOCKET> Connects;
unsigned int conS = 0;

void client(int);
void Logout(const std::string&);

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
	for (size_t i = 0; i < p.size(); ++i) {
		port = port * 10 + (p[i] - '0');
	}

	SOCKADDR_IN addr{};
	addr.sin_addr.s_addr = inet_addr(IP);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	SOCKET sockListen = socket(AF_INET, SOCK_STREAM, NULL);
	if (sockListen == INVALID_SOCKET) {
		std::cout << "Error at socket.\n";
		Logout("Error at socket.");
		WSACleanup();
		return 2;
	}
	if (bind(sockListen, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		std::cout << "Bind failed with error.\n";
		Logout("Bind failed with error.");
		closesocket(sockListen);
		WSACleanup();
		return 3;
	}
	if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << ("Listen failed with error\n");
		closesocket(sockListen);
		WSACleanup();
		return 4;
	}



	User add;
	std::string clr;
	for (size_t i = 0; i < jsData["User"].size(); ++i) {
		add.name = jsData["User"][i]["Username"];
		add.password = jsData["User"][i]["Password"];
		add.color = 0;
		clr = jsData["User"][i]["Color"];
		for (size_t i = 0; i < clr.size(); ++i) {
			add.color = add.color * 10 + (clr[i] - '0');
		}
		users.push_back(add);
	}
	config.close();

	SOCKET Connect;
	int addrSize = sizeof(addr);
	for (size_t i = 0; i < users.size(); ++i) {
		Connect = accept(sockListen, (SOCKADDR*)&addr, &addrSize);
		if (Connect == INVALID_SOCKET) {
			Logout("Accept failed.");
			continue;
		}
		if (Connect == 0) {
			std::cout << "Error connection\n";
		}
		else {
			Connects.push_back(Connect);
			++conS;
			std::cout << "Connection!\n" << conS << "|\n";

			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)client, (LPVOID)(i), NULL, NULL);
		}
	}

	closesocket(sockListen);
	shutdown(Connect, SD_SEND);
	closesocket(Connect);
	WSACleanup();
	return 0;
}

void client(int index) {
	char msg[64]{ 0 };
	char password[64]{ 0 };
	char name[64]{ 0 };
	bool find = false;
	WORD color;
	while (!find) {
		recv(Connects[index], name, 64, 0);
		recv(Connects[index], password, 64, 0);
		for (size_t i = 0; i < users.size(); ++i) {
			if (name == users[i].name && password == users[i].password) {
				find = true;
				Logout(std::string(name) + " logged in chat.");
				std::cout << name << " logged in chat!\n";
				color = users[i].color + 32;
				break;
			}
		}
		if (!find) {
			Logout(std::string(name) + " failed login in chat.");
			std::cout << name << " failed login in chat!\n";
		}
	}

	while (true) {
		recv(Connects[index], msg, 64, 0);
		if (std::string(msg) == "Exit.") {
			Logout(std::string(name) + " exit chat.");
			std::cout << std::string(name) << " exit chat.\n";

			for (unsigned int i = 0; i < conS; ++i) {
				if (i != index) {
					send(Connects[i], ("'\t" + std::string(name) + std::string(" exit chat!\n")).c_str(), 65, 0);
				}
			}

			shutdown(Connects[index], SD_SEND);
			closesocket(Connects[index]);
			Connects[index] = INVALID_SOCKET;
			break;
		}
		else {
			if (strlen(msg) == 0) {
				for (unsigned int i = 0; i < conS; ++i) {
					if (i != index) {
						send(Connects[i], (char(color) + std::string(name) + ": join in chat.").c_str(), 65, 0);
					}
				}
				continue;
			}
			Logout(std::string(name) + ": " + std::string(msg));
			for (unsigned int i = 0; i < conS; ++i) {
				if (i != index) {
					send(Connects[i], (char(color) + std::string(name) + ": " + std::string(msg)).c_str(), 65, 0);
				}
			}
		}
	}
}

void Logout(const std::string& logMessage) {
	std::ofstream logFile("log.txt", std::ios_base::app);
	if (logFile.is_open()) {
		auto current_time = std::chrono::system_clock::now();
		auto time_stamp = std::chrono::system_clock::to_time_t(current_time);
		std::stringstream time_stream;
		time_stream << std::put_time(std::localtime(&time_stamp), "%Y-%m-%d %X");
		logFile << "[" << time_stream.str() << "]" << logMessage << std::endl;
		logFile.close();
	}
}
