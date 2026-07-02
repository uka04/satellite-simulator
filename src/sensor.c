#include <stdlib.h>
#include <time.h>

int get_temperature() {
	srand(time(NULL));

	int temp = (rand() % 15) + 21;

	return temp;
}
