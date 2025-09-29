#include "Starter.h"

int main(int argc, char** argv)

{
	auto app = GUI::CreateApplication({ argc, argv });

	app->Run();

	delete app;
}