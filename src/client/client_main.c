#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include "sensor.h"
#include "protocol.h"

void dispaly_ui(const SatellitePacket *pkt) {
	mvprintw(0, 0, "[%s]", time_str);
    mvprintw(1, 0, "==== Satellite Control Simulator ====");
    mvprintw(2, 0, "Satellite Name : %s", pkt->tle.name);

	// line 2
	mvprintw(4, 0, "-- Line 2 Info --");
	mvprintw(5, 0, "Norad Id       : %d", pkt->tle.NoradId);
	mvprintw(6, 0, "Classification : %s", pkt->tle.Classification);
	mvprintw(7, 0, "CosparId       : %s", pkt->tle.CosparId);
	mvprintw(8, 0, "Epoch_Year     : %d", pkt->tle.Epoch_Year);
	mvprintw(9, 0, "Epoch_Day      : %f", pkt->tle.Epoch_Day);
	mvprintw(10, 0, "Decay Rate1    : %f", pkt->tle.Decay_Rate1);
	mvprintw(11, 0, "Decay Rate2    : %f", pkt->tle.Decay_Rate2);
	mvprintw(12, 0, "Bstar          : %.8f", pkt->tle.Bstar);

	// line 3
	mvprintw(14, 0, "-- Line 3 Info --");
	mvprintw(15, 0, "Inclination       : %.4f", pkt->tle.Inclination);
	mvprintw(16, 0, "Raan              : %.4f", pkt->tle.Raan);
	mvprintw(17, 0,"Eccentricity      : %.7f", pkt->tle.Eccentricity);
	mvprintw(18, 0, "Perigee           : %.4f", pkt->tle.Perigee);
	mvprintw(19, 0, "Mean_Anomaly      : %.4f", pkt->tle.Mean_Anomaly);
    mvprintw(20, 0, "Mean_Motion       : %.8f orbits/day", pkt->tle.Mean_Motion);
	mvprintw(21, 0, "Revolution_Number : %d", pkt->tle.Revolution_Number);

	if (sgp4_ok) {
		mvprintw(23, 0, "-- SGP4 Real-time Position --");
		mvprintw(24, 0, "Latitude      : %.4f deg", pkt->pos.lat);
		mvprintw(25, 0, "Longtitude    : %.4f deg", pkt->pos.lon);
		mvprintw(26, 0, "Altitude      : %.2f km", pkt->pos.alt);
		mvprintw(27, 0, "Speed         : %.2f km/s",pkt->current_speed);
	} else {
		mvprintw(23, 0, "SGP4 Calculation Failed.");
	}

	// more Info
	mvprintw(29, 0, "-- More Info --");
	mvprintw(30, 0, "Day_Distance_km   : Around %f", pkt->info.Day_Distance_km);
	mvprintw(31, 0, "Period_min        : Around %f", pkt->info.Period_min);
	mvprintw(32, 0, "Data Age          : %.2f hours ago", pkt->info.Data_Age_hours);
}

void check_event_system(const SatellitePacket *pkt, int start_y) {
	mvprintw(start_y, 0, "[Event System Log] --");
	int event_triggered = 0;
	int cur_y = start_y + 1;
	
	// altitude
	if (pkt->pos.alt < 150.0) {
		attron(COLOR_PAIR(1) | A_BOLD);
		mvprintw(cur_y++, 0, "[CRITICAL] REENTRY RISK: Satellite collaspsing! (%.2f km)!\033[0m\n", pos->alt);
		attroff(COLOR_PAIR(1) | A_BOLD);
		event_triggered = 1;
	} else if (pkt->pos.alt < 350.0) {
		attron(COLOR_PAIR(2) | A_BOLD);
		mvprintw(cur_y++, 0, "[WARNING] High atmospheric drag. Low altitude. (%.2f km).\033[0m\n", pos->alt);
		attroff(COLOR_PAIR(2) | A_BOLD);
		event_triggered = 1;
	}

	// speed 
	if (pkt->delta_v > 0.005) {
		attron(COLOR_PAIR(2) | A_BOLD);
		mvprintw(cur_y++, 0, "[ALERT] ORBIT CHANGED: Speed maneuver detected! (Delta V: %.4f km/s)", fabs(current_speed - prev_speed));
        attroff(COLOR_PAIR(2) | A_BOLD);
		event_triggered = 1;
	}

	// old epoch
	if (pkt->info.Data_Age_hours > 72.0) {
		attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(cur_y++, 0, "[WARNING] UPDATE TLE: Satellite data is old (%.1f hours ago). Update required.", info->Data_Age_hours);
        attroff(COLOR_PAIR(2) | A_BOLD);
		event_triggered = 1;
    }

	// in polar
	if (fabs(pkt->pos.lat) > 80.0) {
		attron(COLOR_PAIR(3) | A_BOLD);
		mvprintw(cur_y++, 0, "[INFO] POLAR PASS: Passing over the polar region (Lat: %.2f deg)", pos->lat);
		attroff(COLOR_PAIR(3) | A_BOLD);
		event_triggered = 1;
	}

	// no event
	if (!event_triggered) {
        mvprintw(cur_y++, 0, "[INFO] No special events.");
    }
	mvprintw(cur_y, 0, "-------------------");
}

int main() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;	//ipv4
    serv_addr.sin_port = htons(SERVER_PORT); // destination port setting
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

	printf("Connecting to Satellite Server. \n");
	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Connection Failed! Make sure server is running. \n");
		return 1;
	}

	// init ncurses
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);

	SatellitePacket packet;

	while (1) {
		int bytes = recv(sock_fd, &packet, sizeof(SatellitePacket), 0);
        if (bytes <= 0) break; // server shutdown

        clear();
        display_ui(&packet);
        check_event_system(&packet, 34);
        mvprintw(42, 0, "[Connected to Server - Press Ctrl+C to Exit]");
        refresh();
	}

	endwin();
	close(sock_fd);
	return 0;
}