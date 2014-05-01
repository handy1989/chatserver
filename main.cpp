#include <string>
#include <vector>
#include "chatserver.h"

using std::string;
using std::vector;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s port\n", argv[0]);
        return 1;
    }
	ChatServer *chat = new ChatServer(atoi(argv[1]));
	chat->run();
	return 0;
}
