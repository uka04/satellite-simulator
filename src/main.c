#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>
#include <ncurses.h>
#include "sensor.h"
#include "protocol.h"

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

void display_satellite_info_ncurses(const char *time_str, const SatelliteData *tle, 
const SatelliteMoreInfo *info, const SatellitePosition *pos, int sgp4_ok) {
	
	mvprintw(0, 0, "[%s]", time_str);
    mvprintw(1, 0, "==== Satellite Control Simulator ====");
    mvprintw(2, 0, "Satellite Name : %s", tle->name);

	// line 2
	mvprintw(4, 0, "-- Line 2 Info --");
	mvprintw(5, 0, "Norad Id       : %d", tle->NoradId);
	mvprintw(6, 0, "Classification : %s", tle->Classification);
	mvprintw(7, 0, "CosparId       : %s", tle->CosparId);
	mvprintw(8, 0, "Epoch_Year     : %d", tle->Epoch_Year);
	mvprintw(9, 0, "Epoch_Day      : %f", tle->Epoch_Day);
	mvprintw(10, 0, "Decay Rate1    : %f", tle->Decay_Rate1);
	mvprintw(11, 0, "Decay Rate2    : %f", tle->Decay_Rate2);
	mvprintw(12, 0, "Bstar          : %.8f", tle->Bstar);

	// line 3
	mvprintw(14, 0, "-- Line 3 Info --");
	mvprintw(15, 0, "Inclination       : %.4f", tle->Inclination);
	mvprintw(16, 0, "Raan              : %.4f", tle->Raan);
	mvprintw(17, 0,"Eccentricity      : %.7f", tle->Eccentricity);
	mvprintw(18, 0, "Perigee           : %.4f", tle->Perigee);
	mvprintw(19, 0, "Mean_Anomaly      : %.4f", tle->Mean_Anomaly);
    mvprintw(20, 0, "Mean_Motion       : %.8f orbits/day", tle->Mean_Motion);
	mvprintw(21, 0, "Revolution_Number : %d", tle->Revolution_Number);

	if (sgp4_ok) {
		mvprintw(23, 0, "-- SGP4 Real-time Position --");
		mvprintw(24, 0, "Latitude      : %.4f deg", pos->lat);
		mvprintw(25, 0, "Longtitude    : %.4f deg", pos->lon);
		mvprintw(26, 0, "Altitude      : %.2f km", pos->alt);
		mvprintw(27, 0, "Speed         : %.2f km/s",sqrt(pos->vx*pos->vx + pos->vy*pos->vy + pos->vz*pos->vz));
	} else {
		mvprintw(23, 0, "SGP4 Calculation Failed.");
	}

	// more Info
	mvprintw(29, 0, "-- More Info --");
	mvprintw(30, 0, "Day_Distance_km   : Around %f", info->Day_Distance_km);
	mvprintw(31, 0, "Period_min        : Around %f", info->Period_min);
	mvprintw(32, 0, "Data Age          : %.2f hours ago", info->Data_Age_hours);
}

void run_simulator(const char *file_path) {
	SatelliteData my_satellite;
	SatelliteMoreInfo my_info;
	SatellitePosition pos;
	int sgp4_ok = 0;
	char time_str[30];

	double current_speed = 0.0;
	double prev_speed = 0.0;

	if (!read_tle_data(file_path, &my_satellite)) {
		printf("Error: Failed to read satellite data.\n");
		return;
	}

	calculate_more_info(&my_satellite, &my_info);
	update_satellite(&my_satellite, &my_info, &pos, &sgp4_ok);

	get_current_time_str(time_str, sizeof(time_str));
		
	FILE *log_file = fopen("logs/satellite.log", "a");
	if (log_file != NULL) {
		print_satellite_info(log_file, time_str, &my_satellite, &my_info, &pos, sgp4_ok);
		fclose(log_file);
	} else {
		printf("Can't open the log file.\n");
	}
	
	// init ncurses
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	timeout(1000);
	start_color();

	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);

	while(1) {
		clear();			// clear the screen

		get_current_time_str(time_str, sizeof(time_str));	// current time
		my_info.Data_Age_hours += (1.0 / 3600.0);			// current time + time
		update_satellite(&my_satellite, &my_info, &pos, &sgp4_ok);	// re calculation

		display_satellite_info_ncurses(time_str, &my_satellite, &my_info, &pos, sgp4_ok);

		// calculate speed
        if (sgp4_ok) {
            current_speed = sqrt(pos.vx*pos.vx + pos.vy*pos.vy + pos.vz*pos.vz);
            
            check_event_system(&pos, &my_info, current_speed, prev_speed, 34);
            
            prev_speed = current_speed;
        }
			
		mvprintw(42, 0, "[Press 'q' to exit]");
		refresh();

		int ch = getch();
		if (ch == 'q' || ch == 'Q') {
			break;
		}
	}

	// end ncurses
	endwin();
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
