#pragma once

#define BUFFER_SIZE 1024

#include <atomic>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <boost\asio.hpp>
#include <boost\asio\ip\tcp.hpp>

using namespace std;
namespace asio = boost::asio;

class CProgram
{
public:
	CProgram();
	~CProgram();

	void initialize();
	void update();
	void quit();
	bool isRunning();

private:
	atomic_bool running = true;

	vector<shared_ptr<thread>> threads;

	asio::io_service service;
	asio::ip::tcp::acceptor acceptor;

	mutex clientListMutex;
	mutex messageQueueMutex;

	list<shared_ptr<asio::ip::tcp::socket>> clientList;
	// A queue that holds a pointer to a map of pointers to sockets and strings
	queue<shared_ptr<map<shared_ptr<asio::ip::tcp::socket>, shared_ptr<string>>>> messageQueue;

	enum sleepLengths { small = 100, big = 200 };


	void acceptorLoop();
	void requestLoop();
	void responseLoop();
	void serverCommandsLoop();

	void disconnectClient(shared_ptr<asio::ip::tcp::socket> clientSocket);
};

