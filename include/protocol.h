#ifndef	PROTOCOL_H
#define PROTOCOL_H

#include "sensor.h"

#define SERVER_PORT 8080
#define MAX_SATELLITES 20

// status
typedef enum {
	STATUS_NORMAL = 0,
	STATUS_WARNING,
	STATUS_CRITICAL
} SatStatus;

// summary
typedef struct {
	int id;		// satellite index
	char name[30];
	double alt;
	double speed;
	SatStatus status;
	char status_msg[50];
} SatelliteSummary;

// list
typedef struct {
	char time_str[30];
	int count;
	SatelliteSummary sum[MAX_SATELLITES];
} AllSatellitesPacket;

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

// define requesting type
typedef enum {
	REQ_ALL_SUMMARY = 0,
	REQ_DETAIL
} RequestType;

typedef struct {
	RequestType type;
	int target_id;
} ClientRequest;

#endif