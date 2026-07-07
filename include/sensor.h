#ifndef SENSOR_H
#define SENSOR_H

typedef struct {
	char name[30];
	double mean_motion;
} SatelliteData;

int read_tle_data(const char *file_path, SatelliteData *out_data);

#endif
