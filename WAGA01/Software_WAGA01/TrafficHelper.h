/*
 * TrafficHelper.h
 * Copyright (C) 2019-2022 Linar Yusupov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRAFFICHELPER_H
#define TRAFFICHELPER_H

#include "SoCHelper.h"

extern "C" {
#include <gdl90.h>
}

typedef struct traffic_struct {

    time_t    timestamp;
    uint8_t   packet_type;   // 1=PFLAA, 2=PFLAU

/* -------------------------------+------------------------------ */
/*            FLARM FTD-12        |      GDL90 equivalent         */
/* -------------------------------+------------------------------ */
    int8_t    alarm_level;        // trafficAlertStatus
                                  //
    int8_t    IDType;             // addressType                // PFLAA only
    uint32_t  ID;                 // address
    uint16_t  Track;              // trackOrHeading             // PFLAA only
    int16_t   TurnRate;                                         // PFLAA only
    uint16_t  GroundSpeed;        // horizontalVelocity         // PFLAA only
    float     ClimbRate;          // verticalVelocity           // PFLAA only
    int8_t    AcftType;           // emitterCategory            // PFLAA only
    int8_t    NoTrack;             //                            // PFLAA only - protocol 9 added WAGA 280325
    int8_t    Source;              //                            // PFLAA only - protocol 9 added WAGA 280325

    uint8_t   alert_level;        // minimum alarm level to generate voice warning

/*            Legacy              */
    float     RelativeNorth;                                   // PFLAA only
    float     RelativeEast;                                    // PFLAA only
    float     RelativeVertical;
    int       RelativeBearing;                                 // PFLAU only

    float     distance;
    float     adj_dist;

/*            GDL90      */
    float     latitude;
    float     longitude;
    float     altitude;
    uint8_t   callsign [GDL90_TRAFFICREPORT_MSG_CALLSIGN_SIZE];

    uint8_t   alert;              // bitmap of issued voice/tone/ble/... alerts
} traffic_t;

typedef struct traffic_by_dist_struct {
  traffic_t *fop;
  float     distance;
  char      CompID[3]; // WAGA 010425
 } traffic_by_dist_t;

#define ALARM_ZONE_NONE         10000  // metres
#define ALARM_ZONE_CLOSE        6000

#define VERTICAL_SLOPE          5  /* slope effect for alerts */

/* alarm levels are defined in NMEAHelper.h */

#define ENTRY_EXPIRATION_TIME   5 /* seconds */
#define TRAFFIC_VECTOR_UPDATE_INTERVAL 2 /* WAGA 170425 2 secs is probably optimum.  EPD partial udate takes approx 0.7 secs 
                                          so 1.3 secs (66% of loop) available for Flarm Alarm interupt */
#define TRAFFIC_UPDATE_INTERVAL_MS (TRAFFIC_VECTOR_UPDATE_INTERVAL * 1000) 
#define isTimeToUpdateTraffic() (millis() - UpdateTrafficTimeMarker > \
                                  TRAFFIC_UPDATE_INTERVAL_MS)

#define isTimeToVoice()         (millis() - Traffic_Voice_TimeMarker > 2000)
#define VOICE_EXPIRATION_TIME   5 /* seconds */

#define TRAFFIC_ALERT_VOICE     1    // bit within the .alert field

#define ALARM_1_REPEAT          600  // WAGA 180425
#define ALARM_2_REPEAT          400
#define ALARM_3_REPEAT          200
#define BUZZ_DURATION           100

// wordings for voice warnings and notifications:
#define WARNING_WORD1 "alert"// WAGA 030425  was traffic
#define WARNING_WORD2 "warning"  // WAGA 030425 added, did not exist
#define WARNING_WORD3 "danger"
// words available: alert, traffic, warning, danger, near, aware, notice
#define ADVISORY_WORD "traffic"

void Traffic_setup        (void);
void Traffic_loop         (void);
void Traffic_Add          (void);
void Traffic_Update       (traffic_t *);
void Traffic_ClearExpired (void);
int  Traffic_Count        (void);
void Buzzer               (void);

int  traffic_cmp_by_distance(const void *, const void *);
int  traffic_cmp_by_alarm(const void *, const void *); // WAGA 260325
int Threat_Assess         (void);  // WAGA 180325
String rightchr           (String const&, size_t const); //WAGA 030525

extern traffic_t ThisAircraft, Container[MAX_TRACKING_OBJECTS], fo, EmptyFO;
extern traffic_by_dist_t traffic[MAX_TRACKING_OBJECTS];


#endif /* TRAFFICHELPER_H */
