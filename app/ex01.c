#include <ubinos.h>

#if (INCLUDE__APP__ex01 == 1)

#include <stdio.h>

int appmain(int argc, char * argv[]) {
	for (int i = 0;; i++) {
		printf("hello world (%d)\r\n", i);
		bsp_busywaitms(1000);
	}

	return 0;
}

#endif /* (INCLUDE__APP__ex01 == 1) */

