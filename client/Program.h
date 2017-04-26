#pragma once

#define BUFFER_SIZE 1024

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <boost\asio.hpp>

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
	string username;

	asio::io_service service;
	asio::ip::tcp::socket socket;

	queue<shared_ptr<string>> messageQueue;
	asio::ip::tcp::endpoint endpoint;

	vector<shared_ptr<thread>> threads;
	mutex messageQueueMutex;


	void displayLoop();
	void inboundLoop();
	void writeLoop();

	void getUsername();
	bool isOwnMessage(shared_ptr<string> message);
};

