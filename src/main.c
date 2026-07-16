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

void update_satellite(const SatelliteData *tle, const SatelliteMoreInfo *info,
							SatellitePosition *out_pos, int *out_sgp4_ok) {
		// SGP4 Info
		double minutes_past_epoch = info->Data_Age_hours * 60.0;
		// result of calculation
		*out_sgp4_ok = get_satellite_position(tle, minutes_past_epoch, out_pos);
}

void print_satellite_info(FILE *stream, const char *time_str, const SatelliteData *tle, 
const SatelliteMoreInfo *info, const SatellitePosition *pos, int sgp4_ok) {
	if (stream == NULL) return;
	fprintf(stream, "[%s]\n", time_str);
    fprintf(stream, "==== Satellite Control Simulator ====\n");
    fprintf(stream, "Satellite Name : %s\n\n", tle->name);

	// line 2
	fprintf(stream, "-- Line 2 Info --\n");
	fprintf(stream, "Norad Id       : %d\n", tle->NoradId);
	fprintf(stream, "Classification : %s\n", tle->Classification);
	fprintf(stream, "CosparId       : %s\n", tle->CosparId);
	fprintf(stream, "Epoch_Year     : %d\n", tle->Epoch_Year);
	fprintf(stream, "Epoch_Day      : %f\n", tle->Epoch_Day);
	fprintf(stream, "Decay Rate1    : %f\n", tle->Decay_Rate1);
	fprintf(stream, "Decay Rate2    : %f\n", tle->Decay_Rate2);
	fprintf(stream, "Bstar          : %.8f\n\n", tle->Bstar);

	// line 3
	fprintf(stream, "-- Line 3 Info --\n");
	fprintf(stream, "Inclination       : %.4f\n", tle->Inclination);
	fprintf(stream, "Raan              : %.4f\n", tle->Raan);
	fprintf(stream,"Eccentricity      : %.7f\n", tle->Eccentricity);
	fprintf(stream, "Perigee           : %.4f\n", tle->Perigee);
	fprintf(stream, "Mean_Anomaly      : %.4f\n", tle->Mean_Anomaly);
    fprintf(stream, "Mean_Motion       : %.8f orbits/day\n", tle->Mean_Motion);
	fprintf(stream, "Revolution_Number : %d\n\n", tle->Revolution_Number);

	if (sgp4_ok) {
		fprintf(stream, "-- SGP4 Real-time Position --\n");
		fprintf(stream, "Latitude      : %.4f deg\n", pos->lat);
		fprintf(stream, "Longtitude    : %.4f deg\n", pos->lon);
		fprintf(stream, "Altitude      : %.2f km\n", pos->alt);
		fprintf(stream, "Speed         : %.2f km/s\n\n",sqrt(pos->vx*pos->vx + pos->vy*pos->vy + pos->vz*pos->vz));
	} else {
		fprintf(stream, "SGP4 Calculation Failed.\n\n");
	}

	// more Info
	fprintf(stream, "-- More Info --\n");
	fprintf(stream, "Day_Distance_km   : Around %f\n", info->Day_Distance_km);
	fprintf(stream, "Period_min        : Around %f\n", info->Period_min);
	fprintf(stream, "Data Age          : %.2f hours ago\n", info->Data_Age_hours);

	if (stream != stdout) {
		fprintf(stream, "------------------------------------\n");
	}
}

void run_simulator(const char *file_path) {
	SatelliteData my_satellite;
	SatelliteMoreInfo my_info;
	SatellitePosition pos;
	int sgp4_ok = 0;
	char time_str[30];

	if (!read_tle_data(file_path, &my_satellite)) {
		printf("Error: Failed to read satellite data.\n");
		return;
	}

	calculate_more_info(&my_satellite, &my_info);
	update_satellite(&my_satellite, &my_info, &pos, &sgp4_ok);

	get_current_time_str(time_str, sizeof(time_str));
		
	print_satellite_info(stdout, time_str, &my_satellite, &my_info, &pos, sgp4_ok);

	FILE *log_file = fopen("logs/satellite.log", "a");
	if (log_file != NULL) {
		print_satellite_info(log_file, time_str, &my_satellite, &my_info, &pos, sgp4_ok);
		fclose(log_file);
	} else {
		printf("Can't open the log file.\n");
	}
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
	
	run_simulator(file_list[choice - 1]);

	return 0;
}
