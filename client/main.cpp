#include "Program.h"

int main()
{
	{
		CProgram program;

		program.initialize();
		while (program.isRunning())
		{
			program.update();
		}
		program.quit();
	}

	return 0;
}