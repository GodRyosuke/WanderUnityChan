#include "Game.hpp"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
int main(int argc, char** argv)
{
	Game game;
	if (!game.Initialize()) {
		printf("error: Failed to Initialize Game\n");
		return -1;
	}

	game.RunLoop();
	game.Shutdown();
	return 0;
}