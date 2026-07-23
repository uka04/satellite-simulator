#include <math.h>
#include "sensor.h"
#include "sgp4unit.h"
#include "sgp4ext.h"

extern "C" int get_satellite_position(const SatelliteData *tle, double minutes_past_epoch, SatellitePosition *out_pos) {
	elsetrec satrec;

	double deg2rad = M_PI / 180.0;	// radian
	char opsmode = 'i';
	double xpdotop = 1440.0 / (2.0 * M_PI); // 

	sgp4init(
		wgs72, opsmode, tle->NoradId, tle->Epoch_Day,
		tle->Bstar, tle->Eccentricity, tle->Perigee * deg2rad,
		tle->Inclination * deg2rad, tle->Mean_Anomaly * deg2rad, 
		tle->Mean_Motion / xpdotop, tle->Raan * deg2rad, 
		satrec
	);

	double ro[3];
	double vo[3];

	// find orbit
	bool result = sgp4(wgs72, satrec, minutes_past_epoch, ro, vo);
	// if failed to calculate or satellite crash
	if (!result) return 0;

	out_pos->x = ro[0]; out_pos->y = ro[1]; out_pos->z = ro[2];
	out_pos->vx = vo[0]; out_pos->vy = vo[1]; out_pos->vz = vo[2];

	double r = sqrt(ro[0]*ro[0] + ro[1]*ro[1] + ro[2]*ro[2]);
	out_pos->alt = r - 6378.137;

	out_pos->lat = asin(ro[2] / r) * (180.0 / M_PI);
	out_pos->lon = atan2(ro[1], ro[0]) * (180.0 / M_PI);

	return 1;
}
