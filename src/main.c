#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include "sensor.h"

#define MAX_SATELLITES 20
#define PATH_LENGTH 300

void get_current_time_str(char *buffer, int max_size) {
	time_t raw_time = time(NULL);
	struct tm *time_info = localtime(&raw_time);

	strftime(buffer, max_size, "%Y-%m-%d %H:%M:%S", time_info);
}

int main() {
	char file_list[MAX_SATELLITES][PATH_LENGTH];
	int file_count = 0;

	DIR *dir = opendir("data");
	if (dir == NULL) {
		printf("can't open the diretory");
		return 1;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL && file_count < MAX_SATELLITES) {
		if (entry->d_type == DT_REG && is_tle_file(entry->d_name)) {
			snprintf(file_list[file_count], PATH_LENGTH, "data/%s", entry->d_name);
			file_count++;
		}
	}
	closedir(dir);

	if (file_count == 0) {
		printf("No file in 'data' folder");
		return 1;
	}

	// select a satellite & formatting
	printf("==== List of Satellite ====\n");
	for (int i = 0; i < file_count; i++) {
		char display_name[PATH_LENGTH];

		// read from +5
		if (strncmp(file_list[i], "data/", 5) == 0) {
			snprintf(display_name, sizeof(display_name), "%s", file_list[i] + 5);
		} else {
			snprintf(display_name, sizeof(display_name), "%s", file_list[i]);
		}
		char *ext = strrchr(display_name, '.');
		if (ext != NULL && strcmp(ext, ".tle") == 0) {
			*ext = '\0';
		}

		printf("[%d] %s\n", i + 1, display_name);
	}
	printf("===========================\n");
	printf("Select Satellite. (1-%d): ", file_count);

	int choice;
	if (scanf("%d", &choice) != 1) {
		printf("Select a valid number\n");
		return 1;
	}
	
	if (choice < 1 || choice > file_count) {
		printf("Wrong number.\n");
		return 1;
	}

	int selected_index = choice - 1;

	FILE *log_file = fopen("logs/satellite.log", "a");
	if (log_file == NULL) {
		printf("can't open the log file");
		return 1;
	}

	char time_str[30];

	get_current_time_str(time_str, sizeof(time_str));	
	
	SatelliteData my_satellite;
	SatelliteMoreInfo my_info;

	if (read_tle_data(file_list[selected_index], &my_satellite)) {
		calculate_more_info(&my_satellite, &my_info);
		
        printf("[%s]\n", time_str);
        printf("==== Satellite Control Simulator ====\n");
        printf("Satellite Name : %s\n\n", my_satellite.name);

		// line 2
		printf("-- Line 2 Info --\n");
		printf("Norad Id       : %d\n", my_satellite.NoradId);
		printf("Classification : %s\n", my_satellite.Classification);
		printf("CosparId       : %s\n", my_satellite.CosparId);
		printf("Epoch_Year     : %d\n", my_satellite.Epoch_Year);
		printf("Epoch_Day      : %f\n", my_satellite.Epoch_Day);
		printf("Decay Rate1    : %f\n", my_satellite.Decay_Rate1);
		printf("Decay Rate2    : %f\n", my_satellite.Decay_Rate2);
		printf("Bstar          : %.8f\n\n", my_satellite.Bstar);

		// line 3
		printf("-- Line 3 Info --\n");
		printf("Inclination       : %.4f\n", my_satellite.Inclination);
		printf("Raan              : %.4f\n", my_satellite.Raan);
		printf("Eccentricity      : %.7f\n", my_satellite.Eccentricity);
		printf("Perigee           : %.4f\n", my_satellite.Perigee);
		printf("Mean_Anomaly      : %.4f\n", my_satellite.Mean_Anomaly);
        printf("Mean_Motion       : %.8f orbits/day\n", my_satellite.Mean_Motion);
		printf("Revolution_Number : %d\n\n", my_satellite.Revolution_Number);

		// SGP4 Info
		double minutes_past_epoch = my_info.Data_Age_hours * 60.0;

		SatellitePosition pos;
		// result of calculation
		int sgp4_ok = get_satellite_position(&my_satellite, minutes_past_epoch, &pos);

		if (sgp4_ok) {
			printf("-- SGP4 Real-time Position --\n");
			printf("Latitude      : %.4f deg\n", pos.lat);
			printf("Longtitude    : %.4f deg\n", pos.lon);
			printf("Altitude      : %.2f km\n", pos.alt);
			printf("Speed         : %.2f km/s\n\n",sqrt(pos.vx*pos.vx + pos.vy*pos.vy + pos.vz*pos.vz));
		} else {
			printf("SGP4 Calculation Failed.\n\n");
		}

		// more Info
		printf("-- More Info --\n");
		printf("Day_Distance_km   : Around %f\n", my_info.Day_Distance_km);
		printf("Period_min        : Around %f\n", my_info.Period_min);
		printf("Data Age          : %.2f hours ago\n", my_info.Data_Age_hours);
        
        fprintf(log_file, "[%s]\n", time_str);
        fprintf(log_file, "==== Satellite Control Simulator ====\n");
        fprintf(log_file, "Satellite Name : %s\n\n", my_satellite.name);

		// line 2
		fprintf(log_file, "-- Line 2 Info --\n");
		fprintf(log_file, "Norad Id       : %d\n", my_satellite.NoradId);
		fprintf(log_file, "Classification : %s\n", my_satellite.Classification);
		fprintf(log_file, "CosparId       : %s\n", my_satellite.CosparId);
		fprintf(log_file, "Epoch_Year     : %d\n", my_satellite.Epoch_Year);
		fprintf(log_file, "Epoch_Day      : %f\n", my_satellite.Epoch_Day);
		fprintf(log_file, "Decay Rate1    : %f\n", my_satellite.Decay_Rate1);
		fprintf(log_file, "Decay Rate2    : %f\n", my_satellite.Decay_Rate2);
		fprintf(log_file, "Bstar          : %.8f\n\n", my_satellite.Bstar);

		// line 3
		fprintf(log_file, "-- Line 3 Info --\n");
		fprintf(log_file, "Inclination       : %.4f\n", my_satellite.Inclination);
		fprintf(log_file, "Raan              : %.4f\n", my_satellite.Raan);
		fprintf(log_file, "Eccentricity      : %.7f\n", my_satellite.Eccentricity);
		fprintf(log_file, "Perigee           : %.4f\n", my_satellite.Perigee);
		fprintf(log_file, "Mean_Anomaly      : %.4f\n", my_satellite.Mean_Anomaly);
        fprintf(log_file, "Mean_motion       : %.8f orbits/day\n", my_satellite.Mean_Motion);
		fprintf(log_file, "Revolution_Number : %d\n\n", my_satellite.Revolution_Number);

		// SGP4 Info
		if (sgp4_ok) {
			fprintf(log_file, "-- SGP4 Real-time Position --\n");
			fprintf(log_file, "Latitude      : %.4f deg\n", pos.lat);
			fprintf(log_file, "Longtitude    : %.4f deg\n", pos.lon);
			fprintf(log_file, "Altitude      : %.2f km\n", pos.alt);
			fprintf(log_file, "Speed         : %.2f km/s\n\n",sqrt(pos.vx*pos.vx + pos.vy*pos.vy + pos.vz*pos.vz));
		} else {
			fprintf(log_file, "SGP4 Calculation Failed.\n\n");
		}

		// more Info
		fprintf(log_file, "-- More Info --\n");
		fprintf(log_file, "Day_Distance_km   : Around %f\n", my_info.Day_Distance_km);
		fprintf(log_file, "Period_min        : Around %f\n", my_info.Period_min);
		fprintf(log_file, "Data Age          : %.2f hours ago\n", my_info.Data_Age_hours);
        fprintf(log_file, "------------------------------------\n");
    } else {
        printf("Error: Failed to read satellite data.\n");
    }

	fclose(log_file);

	return 0;
}
