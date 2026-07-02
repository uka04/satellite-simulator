#include <stdio.h>
#include "sensor.h"

int main() {
	int temp = get_temperature();

	printf("==== Satellite Control Simulator ====\n");
	printf("Temperature : %d C\n", temp);
	printf("Battery     : 98 %%\n");
	printf("Voltage     : 4.10 V\n");
	printf("Mode        : NORMAL\n");	
	return 0;
}
