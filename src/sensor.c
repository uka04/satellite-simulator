#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"

int read_tle_data(const char *file_path, SatelliteData *out_data) {
	FILE *fp = fopen(file_path, "r");
	if (fp == NULL) {
		return 0;
	}

	char line1[80];
	char line2[80];
	char line3[80];
	char motion_buf[12];
	
	if (fgets(line1, sizeof(line1), fp) != NULL &&
            fgets(line2, sizeof(line2), fp) != NULL &&
            fgets(line3, sizeof(line3), fp) != NULL) {

                line1[strcspn(line1, "\r\n")] = 0;
		strncpy(out_data->name, line1, sizeof(out_data->name) -1);
		out_data->name[sizeof(out_data->name) -1] = '\0';
		
                strncpy(motion_buf, &line3[52], 11);
                motion_buf[11] = '\0';
                out_data->mean_motion = atof(motion_buf);

		fclose(fp);
		return 1;
        }
	
	fclose(fp);
	return 0;
}


