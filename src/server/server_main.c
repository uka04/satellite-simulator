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

void evaluate_status(const SatellitePacket *pkt, SatelliteSummary *sum) {
	sum->status = STATUS_NORMAL;
    snprintf(sum->status_msg, sizeof(sum->status_msg), "NORMAL");

    // Critical 
    if (pkt->pos.alt < 150.0) {
        sum->status = STATUS_CRITICAL;
        snprintf(sum->status_msg, sizeof(sum->status_msg), "CRITICAL");
        return;
    }

    // Warning 
    if (pkt->pos.alt < 350.0 || pkt->delta_v > 0.005 || pkt->info.Data_Age_hours > 72.0) {
        sum->status = STATUS_WARNING;
        snprintf(sum->status_msg, sizeof(sum->status_msg), "WARNING");
    }
}

int main() {
	SatellitePacket sat_list[MAX_SATELLITES];
	double prev_speeds[MAX_SATELLITES] = {0.0};
	int sat_count = 0;

	DIR *dir = opendir("data");
	if (dir == NULL) {
		printf("can't open the diretory");
		return 1;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL && sat_count < MAX_SATELLITES) {
		if (entry->d_type == DT_REG && is_tle_file(entry->d_name)) {
			char file_path[PATH_LENGTH];
			snprintf(file_path, sizeof(file_path), "data/%s", entry->d_name);

			memset(&sat_list[sat_count], 0, sizeof(SatellitePacket)); //init SatellitePacket
			if (read_tle_data(file_path, &sat_list[sat_count].tle)) {
				calculate_more_info(&sat_list[sat_count].tle, &sat_list[sat_count].info);
				sat_count++;
			}
		}
	}
	closedir(dir);

	if (sat_count == 0) {
		printf("No valid TLE files found in 'data/'\n");
		return 1;
	}

	// socket communication
	printf("==== List of Satellite ====\n");

	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	server_addr.sin_family = AF_INET;	// ipv4
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(SERVER_PORT);

	bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(server_fd, 1);	// open

	printf("[Server] Waiting for Clients to connect on port %d\n", SERVER_PORT);
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
	printf("[Server] Client Connected!\n");

	struct timeval tv = {0, 100000};
	setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	ClientRequest req = { REQ_ALL_SUMMARY, 0 }; // reauesting all List, default

	while (1) {
		char current_time[30];
		get_current_time_str(current_time, sizeof(current_time));	// current time

		for (int i = 0; i < sat_count; i++) {
			strcpy(sat_list[i].time_str, current_time);
			sat_list[i].info.Data_Age_hours += (1.0 / 3600.0);		// current time + time
			update_satellite(&sat_list[i].tle, &sat_list[i].info, &sat_list[i].pos, &sat_list[i].sgp4_ok);	// re calculation
			
			if (sat_list[i].sgp4_ok) {
				sat_list[i].current_speed = sqrt(sat_list[i].pos.vx * sat_list[i].pos.vx + 
												 sat_list[i].pos.vy * sat_list[i].pos.vy + 
												 sat_list[i].pos.vz * sat_list[i].pos.vz);
				sat_list[i].delta_v = (prev_speeds[i] > 0.0) ? fabs(sat_list[i].current_speed - prev_speeds[i]) : 0.0;
				prev_speeds[i] = sat_list[i].current_speed;
			}
		}

		// read client's request
		ClientRequest new_req;
		if (recv(client_fd, &new_req, sizeof(ClientRequest), 0) > 0) {
			req = new_req;	// update new request
		}

		// response to client request
		if (req.type == REQ_ALL_SUMMARY) {
			AllSatellitesPacket all_pkt;
			strcpy(all_pkt.time_str, current_time);
			all_pkt.count = sat_count;

			for (int i = 0; i < sat_count; i++) {
                all_pkt.sum[i].id = i;
                strncpy(all_pkt.sum[i].name, sat_list[i].tle.name, sizeof(all_pkt.sum[i].name));
                all_pkt.sum[i].alt = sat_list[i].pos.alt;
                all_pkt.sum[i].speed = sat_list[i].current_speed;
                evaluate_status(&sat_list[i], &all_pkt.sum[i]);
            }

			if (send(client_fd, &all_pkt, sizeof(AllSatellitesPacket), 0) <= 0) break;

		} else if (req.type == REQ_DETAIL) {
			int target = req.target_id;
			if (target < 0 || target >= sat_count) target = 0;
			if (send(client_fd, &sat_list[target], sizeof(SatellitePacket), 0) <= 0) break;
		}

		sleep(1);
	}

	close(client_fd);
	close(server_fd);
	return 0;
}