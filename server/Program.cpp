#include "Program.h"


// Need to initialize the acceptor before the rest of the class actually runs. Run on port 420
CProgram::CProgram() : acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 420))
{
}


CProgram::~CProgram()
{
}

void CProgram::initialize()
{
	// Make space for 4 threads in the vector so that it doesn't need to resize 4 times when making new threads
	threads.reserve(4);

	// Add the 4 threads to the vector
	threads.push_back(make_shared<thread>(&CProgram::acceptorLoop, this));
	threads.push_back(make_shared<thread>(&CProgram::requestLoop, this));
	threads.push_back(make_shared<thread>(&CProgram::responseLoop, this));
	threads.push_back(make_shared<thread>(&CProgram::serverCommandsLoop, this));

	// Join each thread
	for (vector<shared_ptr<thread>>::iterator it = threads.begin(); it != threads.end(); ++it)
	{
		(*it)->join();
	}
}

void CProgram::update()
{
}

void CProgram::quit()
{
	cout << endl;
	system("pause");
}

bool CProgram::isRunning()
{
	return running;
}

void CProgram::acceptorLoop()
{
	cout << "Waiting for clients..." << endl;

	while (running)
	{
		// Make a new client socket
		shared_ptr<asio::ip::tcp::socket> clientSocket = make_shared<asio::ip::tcp::socket>(service);

		// When running gets switched to false, the thread keeps waiting
		// for a new client and will only exit the program once it gets a new client.
		// It's a feature, not a bug.

		// Wait for the socket to accept a connection
		acceptor.accept(*clientSocket);

		// Log that there was a connection
		cout << "New client joined! There are now ";

		// Add the client socket to the list of clients
		clientListMutex.lock();
		clientList.emplace_back(clientSocket);
		clientListMutex.unlock();

		cout << clientList.size() << " clients connected" << endl;
	}
}

void CProgram::requestLoop()
{
	while (running)
	{
		// Sleep for a couple milliseconds
		this_thread::sleep_for(chrono::milliseconds(sleepLengths::big));

		if (clientList.empty())
		{
			continue;
		}

		// Lock the client list
		clientListMutex.lock();

		// Loop through the list of client sockets
		for (list<shared_ptr<asio::ip::tcp::socket>>::iterator clientSocket = clientList.begin(); clientSocket != clientList.end(); ++clientSocket)
		{
			// Check if the client socket is available
			if (!(*clientSocket)->available())
			{
				continue;
			}

			shared_ptr<string> msg = make_shared<string>("");

			// Make a vector of characters to hold what the client sent since the buffer function can't take a string
			vector<char> bufferVector(BUFFER_SIZE);

			// Read from the client socket
			int bytesInBuffer = (*clientSocket)->read_some(asio::buffer(bufferVector));

			{
				int i = 0;
				// Convert the vector of characters into the message string
				for (vector<char>::const_iterator it = bufferVector.begin(); it != bufferVector.end() && i < bytesInBuffer; ++it, ++i)
				{
					*msg += *it;
				}
			}

			// Check if the client message contained "exit"
			if (msg->find("exit") != string::npos)
			{
				// Disconnect the client
				disconnectClient(*clientSocket);
				// Need to break instead of continue because the list of clients is now 1 shorter and if we continue we will try to go out of range of the list
				break;
			}
			
			// Map the client to the message they sent and add the map to the message queue
			shared_ptr<map<shared_ptr<asio::ip::tcp::socket>, shared_ptr<string>>> clientMap = make_shared<map<shared_ptr<asio::ip::tcp::socket>, shared_ptr<string>>>();
			clientMap->insert(pair<shared_ptr<asio::ip::tcp::socket>, shared_ptr<string>>(*clientSocket, msg));

			messageQueueMutex.lock();
			messageQueue.push(clientMap);
			messageQueueMutex.unlock();

			cout << "ChatLog: " << *msg << endl;
		}

		// Unlock the client list
		clientListMutex.unlock();
	}
}

void CProgram::responseLoop()
{
	while (running)
	{
		// Sleep for a couple milliseconds
		this_thread::sleep_for(chrono::milliseconds(sleepLengths::big));

		if (messageQueue.empty())
		{
			continue;
		}

		messageQueueMutex.lock();
		// Get the next message in the queue
		shared_ptr<map<shared_ptr<asio::ip::tcp::socket>, shared_ptr<string>>> message = messageQueue.front();
		// Remove the message from the queue
		messageQueue.pop();
		messageQueueMutex.unlock();

		clientListMutex.lock();
		// Loop through each of the clients and send each one the message
		for (list<shared_ptr<asio::ip::tcp::socket>>::const_iterator clientSocket = clientList.begin(); clientSocket != clientList.end(); ++clientSocket)
		{
			(*clientSocket)->write_some(asio::buffer(*(message->begin()->second)));
		}
		clientListMutex.unlock();
	}
}

void CProgram::serverCommandsLoop()
{
	while (running)
	{
		// Get the command from the command line
		string command;
		getline(cin, command);

		if (command == "stop")
		{
			// Lock the client list and loop through them
			clientListMutex.lock();
			for (list<shared_ptr<asio::ip::tcp::socket>>::iterator clientSocket = clientList.begin(); clientSocket != clientList.end(); ++clientSocket)
			{
				// Tell the clients the server is shutting down
				(*clientSocket)->write_some(asio::buffer("Server is shutting down"));
				// Shutdown the socket to the client
				(*clientSocket)->shutdown(asio::ip::tcp::socket::shutdown_both);
				// Close the socket
				(*clientSocket)->close();
			}
			// Clear the client list otherwise the other threads still try to loop through it and it throws an exception
			clientList.clear();
			clientListMutex.unlock();

			// Might not be needed but just to make sure the message gets sent to every client before the program quits
			this_thread::sleep_for(chrono::milliseconds(500));

			running = false;
		}
	}
}

void CProgram::disconnectClient(shared_ptr<asio::ip::tcp::socket> clientSocket)
{
	// Find the client in the client list
	list<shared_ptr<asio::ip::tcp::socket>>::iterator client = find(clientList.begin(), clientList.end(), clientSocket);

	// Shutdown the socket to the client
	clientSocket->shutdown(asio::ip::tcp::socket::shutdown_both);
	// Close the socket
	clientSocket->close();

	// Remove the client from the list
	clientList.erase(client);

	cout << "Client diconnected! There are now " << clientList.size() << " clients connected" << endl;
}
