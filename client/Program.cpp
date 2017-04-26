#include "Program.h"



CProgram::CProgram() : socket(service), endpoint(asio::ip::address::from_string("127.0.0.1"), 420)
{
}


CProgram::~CProgram()
{
}

void CProgram::initialize()
{
	// Make space for the 3 threads that will be created
	threads.reserve(3);

	// Prompt the user for their username
	getUsername();

	bool connected = false;

	do
	{
		// Try to connect to the server. If it couldn't connect, wait 5 seconds and try again
		try {
			socket.connect(endpoint);
			connected = true;
		}
		catch (const boost::system::system_error & e)
		{
			cout << "Could not connect to the server. Trying again in 5 seconds..." << endl;
			this_thread::sleep_for(chrono::seconds(5));
		}
	} while (!connected);

	// Clear the screen and welcome the user
	system("cls");

	cout << "Welcome to Chris's chat server for the people who don't care about security" << endl
		<< "Type \"exit\" to quit" << endl << endl;

	// Make threads
	threads.push_back(make_shared<thread>(&CProgram::displayLoop, this));
	threads.push_back(make_shared<thread>(&CProgram::inboundLoop, this));
	threads.push_back(make_shared<thread>(&CProgram::writeLoop, this));

	// Join all threads
	for (vector<shared_ptr<thread>>::iterator thread = threads.begin(); thread != threads.end(); ++thread)
	{
		(*thread)->join();
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

void CProgram::displayLoop()
{
	while (running)
	{
		this_thread::sleep_for(chrono::milliseconds(250));

		// Wait until there is a message in the queue
		if (messageQueue.empty())
		{
			continue;
		}

		// Lock the message queue, get the message from the front of the queue, and then remove the message from the queue
		messageQueueMutex.lock();
		shared_ptr<string> message = messageQueue.front();
		messageQueue.pop();
		messageQueueMutex.unlock();

		// Make sure it's not the user's own message because they can already see that on the screen since they typed it
		if (!isOwnMessage(message))
		{
			// Output the message
			cout << *message << endl;

			// Check for the shutdown message
			if ((*message).find("Server is shutting down") != string::npos)
			{
				// Sleep for a second so the user can see the message that the server is being shut down
				this_thread::sleep_for(chrono::seconds(1));

				// This will will make the client wait until the user enters another message to send
				// before it quits the program because it's waiting on getline. It's a feature, not a bug.
				running = false;
			}
		}
	}
}

void CProgram::inboundLoop()
{
	while (running)
	{
		this_thread::sleep_for(chrono::milliseconds(250));

		// Cherck to see if there is something in the socket
		if (!socket.available())
		{
			continue;
		}

		shared_ptr<string> message = make_shared<string>("");

		// Make a vector of characters. This needs BUFFER_SIZE because the asio::buffer won't dynamically add characters to the buffer
		vector<char> bufferVector(BUFFER_SIZE);

		// Store the number of bytes that were read into a variable for use when converting the vector to a string
		int bytesInBuffer = socket.read_some(asio::buffer(bufferVector));

		// Convert the vector of characters into the message string
		{
			int i = 0;
			for (vector<char>::const_iterator it = bufferVector.begin(); it != bufferVector.end() && i < bytesInBuffer; ++it, ++i)
			{
				*message += *it;
			}
		}

		// Add the message to the queue
		messageQueueMutex.lock();
		messageQueue.push(message);
		messageQueueMutex.unlock();
	}
}

void CProgram::writeLoop()
{
	while (running)
	{
		string userInput;

		getline(cin, userInput);

		// Prepend the user's username to the message they typed
		string message = username + userInput;

		// If they didn't type anything, skip this message
		if (userInput.empty())
		{
			continue;
		}

		// Send the message to the server
		socket.write_some(asio::buffer(message));

		// If the message contained "exit" then quit
		if (userInput.find("exit") != string::npos)
		{
			// Sleep for half a second otherwise it doesn't actually send the message to the server to quit
			this_thread::sleep_for(chrono::milliseconds(500));
			running = false;
		}

		userInput.clear();
	}
}

void CProgram::getUsername()
{
	cout << "Please enter a username: ";
	getline(cin, username);
	username += ": ";

	system("cls");
}

bool CProgram::isOwnMessage(shared_ptr<string> message)
{
	// If the username is the very first thing it sees, it's the user's own message
	return message->find(username) == 0;
}
