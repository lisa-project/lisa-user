#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "climain.h"
#include "shared.h"

int main(int argc, char **argv) {
	priv = CLI_MAX_ENABLE;
	climain();
	return 0;
}
