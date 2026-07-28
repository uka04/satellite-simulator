#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include "sensor.h"
#include "protocol.h"

void display_summary_ui(const AllSatellitesPacket *pkt) { 
	mvprintw(0, 0, "[%s] == Multi-Satellite Control Dashboard ==", pkt->time_str);
	mvprintw(2, 0, "ID  %-20s %-12s %-12s %-15s", "Satellite Name", "Alt(km)", "Speed(km/s)", "Status");
    mvprintw(3, 0, "------------------------------------------------------------------");

	for (int i = 0; i < pkt->count; i++) {
		int row = 4 + i;
		mvprintw(row, 0, "[%d] %-20s %-12.2f %-12.2f ", i + 1, pkt->sum[i].name, pkt->sum[i].alt, pkt->sum[i].speed);

		if (pkt->sum[i].status == STATUS_CRITICAL) {
			attron(COLOR_PAIR(1) | A_BOLD);
			printw("%-15s", pkt->sum[i].status_msg);
            attroff(COLOR_PAIR(1) | A_BOLD);
		} else if (pkt->sum[i].status == STATUS_WARNING) {
            attron(COLOR_PAIR(2) | A_BOLD);
            printw("%-15s", pkt->sum[i].status_msg);
            attroff(COLOR_PAIR(2) | A_BOLD);
        } else {
            attron(COLOR_PAIR(3));
            printw("%-15s", pkt->sum[i].status_msg);
            attroff(COLOR_PAIR(3));
        }
	}

	mvprintw(pkt->count + 6, 0, "-> Select Satellite Number [1-%d] to view Detail (or 'q' to Quit): ", pkt->count);
}

void check_event_system(const SatellitePacket *pkt, int start_y) {
	mvprintw(start_y, 0, "[Event System Log]");
	int event_triggered = 0;
	int cur_y = start_y + 1;
	
	// altitude
	if (pkt->pos.alt < 150.0) {
		attron(COLOR_PAIR(1) | A_BOLD);
		mvprintw(cur_y++, 0, "[CRITICAL] REENTRY RISK: Satellite collaspsing! (%.2f km)!", pkt->pos.alt);
		attroff(COLOR_PAIR(1) | A_BOLD);
		event_triggered = 1;
	} else if (pkt->pos.alt < 350.0) {
		attron(COLOR_PAIR(2) | A_BOLD);
		mvprintw(cur_y++, 0, "[WARNING] High atmospheric drag. Low altitude. (%.2f km).", pkt->pos.alt);
		attroff(COLOR_PAIR(2) | A_BOLD);
		event_triggered = 1;
	}

	// speed 
	if (pkt->delta_v > 0.005) {
		attron(COLOR_PAIR(2) | A_BOLD);
		mvprintw(cur_y++, 0, "[ALERT] ORBIT CHANGED: Speed maneuver detected! (Delta V: %.4f km/s)", pkt->delta_v);
        attroff(COLOR_PAIR(2) | A_BOLD);
		event_triggered = 1;
	}

	// old epoch
	if (pkt->info.Data_Age_hours > 72.0) {
		attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(cur_y++, 0, "[WARNING] UPDATE TLE: Satellite data is old (%.1f hours ago). Update required.", pkt->info.Data_Age_hours);
        attroff(COLOR_PAIR(2) | A_BOLD);
		event_triggered = 1;
    }

	// in polar
	if (fabs(pkt->pos.lat) > 80.0) {
		attron(COLOR_PAIR(3) | A_BOLD);
		mvprintw(cur_y++, 0, "[INFO] POLAR PASS: Passing over the polar region (Lat: %.2f deg)", pkt->pos.lat);
		attroff(COLOR_PAIR(3) | A_BOLD);
		event_triggered = 1;
	}

	// no event
	if (!event_triggered) {
        mvprintw(cur_y++, 0, "[INFO] No special events.");
    }
	mvprintw(cur_y, 0, "-------------------");
}

void display_detail_ui(const SatellitePacket *pkt) {
	mvprintw(0, 0, "[%s]", pkt->time_str);
    mvprintw(1, 0, "==== Satellite Control Simulator ====");
    mvprintw(2, 0, "Satellite Name : %s", pkt->tle.name);

    // Line 2 Info
    mvprintw(4, 0, "-- Line 2 Info --");
    mvprintw(5, 0, "Norad Id       : %d", pkt->tle.NoradId);
    mvprintw(6, 0, "Classification : %s", pkt->tle.Classification);
    mvprintw(7, 0, "CosparId       : %s", pkt->tle.CosparId);
    mvprintw(8, 0, "Epoch_Year     : %d", pkt->tle.Epoch_Year);
    mvprintw(9, 0, "Epoch_Day      : %f", pkt->tle.Epoch_Day);
    mvprintw(10, 0, "Decay Rate1    : %f", pkt->tle.Decay_Rate1);
    mvprintw(11, 0, "Decay Rate2    : %f", pkt->tle.Decay_Rate2);
    mvprintw(12, 0, "Bstar          : %.8f", pkt->tle.Bstar);

    // Line 3 Info
    mvprintw(14, 0, "-- Line 3 Info --");
    mvprintw(15, 0, "Inclination       : %.4f", pkt->tle.Inclination);
    mvprintw(16, 0, "Raan              : %.4f", pkt->tle.Raan);
    mvprintw(17, 0, "Eccentricity      : %.7f", pkt->tle.Eccentricity);
    mvprintw(18, 0, "Perigee           : %.4f", pkt->tle.Perigee);
    mvprintw(19, 0, "Mean_Anomaly      : %.4f", pkt->tle.Mean_Anomaly);
    mvprintw(20, 0, "Mean_Motion       : %.8f orbits/day", pkt->tle.Mean_Motion);
    mvprintw(21, 0, "Revolution_Number : %d", pkt->tle.Revolution_Number);

    if (pkt->sgp4_ok) {
        mvprintw(23, 0, "-- SGP4 Real-time Position --");
        mvprintw(24, 0, "Latitude      : %.4f deg", pkt->pos.lat);
        mvprintw(25, 0, "Longtitude    : %.4f deg", pkt->pos.lon);
        mvprintw(26, 0, "Altitude      : %.2f km", pkt->pos.alt);
        mvprintw(27, 0, "Speed         : %.2f km/s", pkt->current_speed);
    } else {
        mvprintw(23, 0, "SGP4 Calculation Failed.");
    }

    // More Info
    mvprintw(29, 0, "-- More Info --");
    mvprintw(30, 0, "Day_Distance_km   : Around %f", pkt->info.Day_Distance_km);
    mvprintw(31, 0, "Period_min        : Around %f", pkt->info.Period_min);
    mvprintw(32, 0, "Data Age          : %.2f hours ago", pkt->info.Data_Age_hours);

    check_event_system(pkt, 34);

    mvprintw(43, 0, "-> Press 'b' to return to Main Dashboard | Press 'q' to Quit");
}

int main() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;	//ipv4
    serv_addr.sin_port = htons(SERVER_PORT); // destination port setting
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

	printf("Connecting to Satellite Server. \n");
	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Connection Failed!\n");
		return 1;
	}

	// init ncurses
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);	// non-blocking, key down
	curs_set(0);
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);

	ClientRequest req = { REQ_ALL_SUMMARY, 0 };
	send(sock_fd, &req, sizeof(ClientRequest), 0);

	AllSatellitesPacket last_all_pkt;
	SatellitePacket last_detail_pkt;
	int has_all_data = 0;
	int has_detail_data = 0;

	while (1) {
		
		int ch = getch();
		if (ch == 'q' || ch == 'Q') break;

		int request_changed = 0;

		// request detail
		if (ch >= '1' && ch <= '9') {
			req.type = REQ_DETAIL;
			req.target_id = ch - '1';
			send(sock_fd, &req, sizeof(ClientRequest), 0);
			request_changed = 1;
		} 

		else if (ch == 'b' || ch == 'B') {
			req.type = REQ_ALL_SUMMARY;
			send(sock_fd, &req, sizeof(ClientRequest), 0);
			request_changed = 1;
		}

		if (req.type == REQ_ALL_SUMMARY) {
			AllSatellitesPacket all_pkt;
			if (recv(sock_fd, &all_pkt, sizeof(AllSatellitesPacket), MSG_DONTWAIT) > 0) {
				last_all_pkt = all_pkt;
				has_all_data = 1;
				
				clear();
				display_summary_ui(&last_all_pkt);
				refresh();
			}
		} else {
			SatellitePacket detail_pkt;
			if (recv(sock_fd, &detail_pkt, sizeof(SatellitePacket), MSG_DONTWAIT) > 0) {
				last_detail_pkt = detail_pkt;
				has_detail_data = 1;

				clear();
				display_detail_ui(&last_detail_pkt);
				refresh();
			}
		}

		if (request_changed) {
			clear();
			if (req.type == REQ_ALL_SUMMARY && has_all_data) {
				display_summary_ui(&last_all_pkt);
			} else if (req.type == REQ_DETAIL && has_detail_data) {
				display_detail_ui(&last_detail_pkt);
			}
			refresh();
		}

		usleep(20000); // 0.02초 대기 (CPU 점유율 방지 및 빠른 키 입력 반응)
	}

	endwin();
	close(sock_fd);
	return 0;
}