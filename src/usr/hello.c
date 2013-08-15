#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    int i = 0;
		for (i = fork(); i > 0; i = fork()) {
	    printf(1, "Hello, world! %d\n", i);
		}
    exit();
}
