

import SessionServer;
#include <iostream>


int main(int argc, char** argv)
{
	SessionServer* server;
	std::cout << argc << std::endl;
	for (size_t i = 0; i < argc; i++)
	{
		std::cout << argv[i] << std::endl;
	}
	
	
	return 0;
}