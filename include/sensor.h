#include <math.h>
#include <stdio.h>
#ifndef SENSOR_H
#define SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct elsetrec;

typedef struct {
	char name[30];

	int NoradId;
	char Classification[2];
	char CosparId[10];
	int Epoch_Year;
	double Epoch_Day;
	double Decay_Rate1;
	double Decay_Rate2;
	double Bstar;

	double Inclination;
	double Raan;
	double Eccentricity;
	double Perigee;
	double Mean_Anomaly;
	double Mean_Motion;
	int Revolution_Number;
} SatelliteData;

typedef struct {
	double x, y, z;
	double vx, vy, vz;
	double lat, lon, alt;
} SatellitePosition;

typedef struct {
	double Day_Distance_km;
	double Period_min;
	double Data_Age_hours;
} SatelliteMoreInfo;

int read_tle_data(const char *file_path, SatelliteData *out_data);
void calculate_more_info(const SatelliteData *tle, SatelliteMoreInfo *out_info);
int is_tle_file(const char *filename);
int get_satellite_position(const SatelliteData *tle, double minutes_past_epoch, SatellitePosition *out_pos);


void update_satellite(const SatelliteData *tle, const SatelliteMoreInfo *info,
							SatellitePosition *out_pos, int *out_sgp4_ok);
void print_satellite_info(FILE *stream, const char *time_str, const SatelliteData *tle, 
							const SatelliteMoreInfo *info, const SatellitePosition *pos, int sgp4_ok);

void display_satellite_info_ncurses(const char *time_str, const SatelliteData *tle, 
const SatelliteMoreInfo *info, const SatellitePosition *pos, int sgp4_ok);

void check_event_system(const SatellitePosition *pos, const SatelliteMoreInfo *info, 
double current_speed, double prev_speed, int start_y);

void run_simulator(const char *file_path);

#ifdef __cplusplus
}
#endif

#endif
