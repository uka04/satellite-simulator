#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>		// network communication
#include "sensor.h"
#include "protocol.h"

#define MAX_SATELLITES 20
#define PATH_LENGTH 300

void get_current_time_str(char *buffer, int max_size) {
	time_t raw_time = time(NULL);
	struct tm *time_info = localtime(&raw_time);
	strftime(buffer, max_size, "%Y-%m-%d %H:%M:%S", time_info);
}

void run_server(const char *file_path) {
	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(sever_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	server_addr.sin_family = AF_INET	// ipv4
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(SERVER_PORT);

	bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(server_fd, 1);	// open

	printf("[Server] Waiting for Clients to connect on port %d\n", SERVER_PORT);
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
	printf("[Server] Client Connected!\n");

	SatellitePacket packet;
	memset(&packet, 0, sizeof(packet));

	if (!read_tle_data(file_path, &packet.tle)) {
		printf("Error: Failed to read TLE data.\n");
		close(client_fd);
		continue;
	}

	calculate_more_info(&packet.tle, &packet.info);
	double prev_speed = 0.0;

	while (1) {
		get_current_time_str(time_str, sizeof(time_str));	// current time
		packet.info.Data_Age_hours += (1.0 / 3600.0);			// current time + time
		update_satellite(&packet.tle, &packet.info, &packet.pos, &packet.sgp4_ok);	// re calculation

		if (packet.sgp4_ok) {
			packet.current_speed = sqrt(packet.pos.vx*packet.pos.vx + packet.pos.vy*packet.pos.vy + packetpos..vz*packet.pos.vz);
            packet.delta_v = (prev_speed > 0.0) ? fabs(packet.current_speed - prev_speed) : 0.0;
            prev_speed = packet.current_speed;
		}

		FILE *log_file = fopen("logs/satellite.log", "a");
		if (log_file) {
			print_satellite_info(log_file, packet.time_str, &packet.tle, &packet.info, &packet.pos, packet.sgp4_ok);
			fclose(log_file);
		}

		if (send(client_fd, &packet, sizeof(SatellitePacket), 0) <= 0) {
			printf("[Server] Client disconnected.\n");
			break;
		}

		sleep(1);
	}

	close(client_fd);
	close(server_fd);
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

	run_server(file_list[choice - 1]);
	return 0;
}