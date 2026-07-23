#ifndef	PROTOCOL_H
#define PROTOCOL_H

#define PORT 8080

// network communication packet
typedef struct {
	char time_str[30];
	SatelliteData tle;
	SatelliteMoreInfo info;
	SatellitePosition pos;
	int sgp4_ok;

	double current_speed;
	double delta_v;
} SatellitePacket;

#endif