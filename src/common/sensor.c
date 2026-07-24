#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ncurses.h>
#include "sensor.h"

int is_tle_file(const char *filename) {
	if (filename == NULL) return 0;

	size_t len = strlen(filename);
	// filename's length is longer then 4 (.tle)
	// file's address + len - 4 address
	if (len > 4 && strcmp(filename + len - 4, ".tle") == 0) {
		return 1;
	}
	return 0;
}

int read_tle_data(const char *file_path, SatelliteData *out_data) {
	FILE *fp = fopen(file_path, "r");
	if (fp == NULL) {
		return 0;
	}

	char line1[80];
	char line2[80];
	char line3[80];

	// line 2
	char NoradId_buf[10] = {0};
	char Class_buf[5] = {0};
	char CosparId_buf[15] = {0};
	char Epoch_Year_buf[5] = {0};
	char Epoch_Day_buf[15] = {0};
	char Decay_Rate1_buf[15] = {0};
	char Decay_Rate2_buf[10] = {0};
	char Bstar_buf[15] = {0};

	// line 3
	char Inclination_buf[15] = {0};
	char Raan_buf[15] = {0};
	char Eccentricity_buf[15] = {0};
	char Perigee_buf[15] = {0};
	char Mean_Anomaly_buf[15] = {0};
	char Motion_buf[15] = {0};
	char Revolution_Number_buf[10] = {0};
	
	if (fgets(line1, sizeof(line1), fp) != NULL &&
        fgets(line2, sizeof(line2), fp) != NULL &&
        fgets(line3, sizeof(line3), fp) != NULL) {

			// Satellite Name
            line1[strcspn(line1, "\r\n")] = 0;
			strncpy(out_data->name, line1, sizeof(out_data->name) -1);
			out_data->name[sizeof(out_data->name) -1] = '\0';
			
			// NORAD
			strncpy(NoradId_buf, &line2[2], 5);
			out_data->NoradId = atoi(NoradId_buf);

			// Classification
			strncpy(Class_buf, &line2[7], 1);
			strcpy(out_data->Classification, Class_buf);

			// CosparId
			strncpy(CosparId_buf, &line2[9], 8);
			strcpy(out_data->CosparId, CosparId_buf);

			// Epoch
			strncpy(Epoch_Year_buf, &line2[18], 2);
			out_data->Epoch_Year = atoi(Epoch_Year_buf);

			strncpy(Epoch_Day_buf, &line2[20], 12);
			out_data->Epoch_Day = atof(Epoch_Day_buf);

			// Decay_Rate
			strncpy(Decay_Rate1_buf, &line2[33], 10);
			out_data->Decay_Rate1 = atof(Decay_Rate1_buf);
			
			strncpy(Decay_Rate2_buf, &line2[44], 8);
			out_data->Decay_Rate2 = atof(Decay_Rate2_buf);

			// Bstar
			char bstar_mantissa[7] = {0};
			char bstar_exp[3] = {0};
			strncpy(bstar_mantissa, &line2[54], 6);
			strncpy(bstar_exp, &line2[59], 2);
			double mantissa = atof(bstar_mantissa) * 1e-5;
			double exponent = atof(bstar_exp);
			out_data->Bstar = mantissa * pow(10, exponent);

			// line 3
			// Inclination
			strncpy(Inclination_buf, &line3[8], 8);
			out_data->Inclination = atof(Inclination_buf);

			// Raan
			strncpy(Raan_buf, &line3[17], 8);
			out_data->Raan = atof(Raan_buf);

			// Eccentricity
			strncpy(Eccentricity_buf, &line3[26], 7);
			out_data->Eccentricity = atof(Eccentricity_buf) * 0.0000001;

			// Perigee
			strncpy(Perigee_buf, &line3[34], 8);
			out_data->Perigee = atof(Perigee_buf);

			// Mean_Anomaly
			strncpy(Mean_Anomaly_buf, &line3[43], 8);
			out_data->Mean_Anomaly = atof(Mean_Anomaly_buf);

			// Mean_Motion
			strncpy(Motion_buf, &line3[52], 11);
            out_data->Mean_Motion = atof(Motion_buf);

			// Revolution_Number
			strncpy(Revolution_Number_buf, &line3[63], 5);
            out_data->Revolution_Number = atoi(Revolution_Number_buf);

		fclose(fp);
		return 1;
        }

	fclose(fp);
	return 0;
}

void calculate_more_info(const SatelliteData *tle, SatelliteMoreInfo *out_info) {
	if (tle == NULL || out_info == NULL) return;

	out_info->Day_Distance_km = 42000 * tle->Mean_Motion;
	out_info->Period_min = 1440 / tle->Mean_Motion;

	time_t current_time = time(NULL);

	int full_year = (tle->Epoch_Year < 57) ? (2000 + tle->Epoch_Year) : (1900 + tle->Epoch_Year);

	struct tm epoch_base_tm = {0};		// epoch base 00:00:00
	epoch_base_tm.tm_year = full_year - 1900;
	epoch_base_tm.tm_mon = 0;
	epoch_base_tm.tm_mday = 1;
	epoch_base_tm.tm_hour = 0;
	epoch_base_tm.tm_min = 0;
	epoch_base_tm.tm_sec = 0;
	epoch_base_tm.tm_isdst = -1;

	// 2026 01 01 sec value
	time_t base_time_secs = mktime(&epoch_base_tm);

	// 2026 01 01 sec + epoch day sec, Epoch Day start +1day (1day = 86400sec)
	double tle_epoch_secs = base_time_secs + ((tle->Epoch_Day - 1.0) * 86400);

	// current sec -  tle sec
	double age_in_seconds = difftime(current_time, (time_t)tle_epoch_secs);

	// sec -> hour
	out_info->Data_Age_hours = age_in_seconds / 3600.0;
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
    fprintf(stream, "Eccentricity      : %.7f\n", tle->Eccentricity);
    fprintf(stream, "Perigee           : %.4f\n", tle->Perigee);
    fprintf(stream, "Mean_Anomaly      : %.4f\n", tle->Mean_Anomaly);
    fprintf(stream, "Mean_Motion       : %.8f orbits/day\n", tle->Mean_Motion);
    fprintf(stream, "Revolution_Number : %d\n\n", tle->Revolution_Number);

    if (sgp4_ok) {
        fprintf(stream, "-- SGP4 Real-time Position --\n");
        fprintf(stream, "Latitude      : %.4f deg\n", pos->lat);
        fprintf(stream, "Longtitude    : %.4f deg\n", pos->lon);
        fprintf(stream, "Altitude      : %.2f km\n", pos->alt);
        fprintf(stream, "Speed         : %.2f km/s\n\n", sqrt(pos->vx*pos->vx + pos->vy*pos->vy + pos->vz*pos->vz));
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

