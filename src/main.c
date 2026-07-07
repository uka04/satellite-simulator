#include <stdio.h>
#include <time.h>
#include "sensor.h"

void get_current_time_str(char *buffer, int max_size) {
	time_t raw_time = time(NULL);
	struct tm *time_info = localtime(&raw_time);

	strftime(buffer, max_size, "%Y-%m-%d %H:%M:%S", time_info);
}

int main() {

	FILE *log_file = fopen("logs/satellite.log", "a");
	if (log_file == NULL) {
		printf("can't open the log file");
		return 1;
	}
	
	char line1[80];
	char line2[80];
	char line3[80];
	char time_str[30];

	get_current_time_str(time_str, sizeof(time_str));	
	
	SatelliteData my_satellite;

	if (read_tle_data("data/iss.tle", &my_satellite)) {
        printf("[%s]\n", time_str);
        printf("==== Satellite Control Simulator ====\n");
        printf("Satellite Name : %s\n", my_satellite.name);
        printf("mean_motion    : %.2f orbits/day\n", my_satellite.mean_motion);
        
        fprintf(log_file, "[%s]\n", time_str);
        fprintf(log_file, "==== Satellite Control Simulator ====\n");
        fprintf(log_file, "Satellite Name : %s\n", my_satellite.name);
        fprintf(log_file, "mean_motion    : %.2f orbits/day\n", my_satellite.mean_motion);
        fprintf(log_file, "------------------------------------\n");
    } else {
        printf("Error: Failed to read satellite data.\n");
    }

	fclose(log_file);

	return 0;
}
