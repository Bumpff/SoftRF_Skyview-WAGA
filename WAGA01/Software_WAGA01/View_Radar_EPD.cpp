/*
 * View_Radar_EPD.cpp
 * Copyright (C) 2019-2021 Linar Yusupov
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

// this version follows
//   https://github.com/nbonniere/SoftRF/tree/master/software/firmware/source/SkyView
// this version follows Moshe MB07C
// This version is WAGA 01

#include "EPDHelper.h"

#include <Fonts/Picopixel.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

#include <TimeLib.h>
#include <math.h>
#define D2R (3.141593f/180.0f)
#define R2D (180.0f/3.141593f)
#include "TrafficHelper.h"
#include "BatteryHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"
//#include "ApproxMath.h" // -- removed by Moshe 7c
#include "TinyGPS++.h" // WAGA added 290225

#include "SkyView.h"



static navbox_t navbox1;
static navbox_t navbox2;
static navbox_t navbox3;
static navbox_t navbox4;

static int EPD_zoom = ZOOM_MEDIUM;

const String Nav_Names[3]= {"YBEV","YCUN","YNRG"}; // WAGA 070325
const double Nav_Waypoints[3][2] ={ 
  {-32.125,116.950},  // YBEV
  {-31.622,117.217},  // YCUN
  {-32.930,117.079}  // YNRG
}; // these waypoints are enumerated in SkyView.h

#define ICON_AIRPLANE

#if defined(ICON_AIRPLANE)
//#define ICON_AIRPLANE_POINTS 6
//const int16_t epd_Airplane[ICON_AIRPLANE_POINTS][2] = {{0,-4},{0,10},{-8,0},{8,0},{-3,8},{3,8}};
#define ICON_AIRPLANE_POINTS 12
const float epd_Airplane[ICON_AIRPLANE_POINTS][2] = {{0,-4},{0,10},{-8,0},{9,0},{-3,8},{4,8},{1,-4},{1,10},{-10,1},{11,1},{-2,9},{3,9}};
#else  //ICON_AIRPLANE
#define ICON_ARROW_POINTS 4
const float epd_Arrow[ICON_ARROW_POINTS][2] = {{-6,5},{0,-6},{6,5},{0,2}};
#endif //ICON_AIRPLANE
#define ICON_TARGETS_POINTS 5
const float epd_Target[ICON_TARGETS_POINTS][2] = {{4,4},{0,-6},{-4,4},{-5,-3},{0,2}};
#define ICON_LOOK_POINTS 10  // WAGA 270325  3x triangles and Rel_Vert print location
const float epd_Look[ICON_LOOK_POINTS][2] = {{0,0},{-10,15},{10,15},{0,17},{-10,32},{10,32},{0,34},{-10,49},{10,49},{0,9}};
#define DOTS_RING_POINTS 12  // WAGA 2020425
const float dot_ring[DOTS_RING_POINTS][2] = {{0,-1},{0.5,-0.866},{0.866,-0.5},{1,0},{0.866,0.5},{0.5,0.866},{0,1},{-0.5,0.866},{-0.866,0.5},{-1,0},{-0.866,-0.5},{-0.5,-0.866}}; // KW 020425 

#define MAX_DRAW_POINTS 12

// 2D rotation, this function does: Anti-clockwise for radar plots (X+ right, Y+ up) butclockwise for icons (X+ right, Y+ down).
// change tsin to -tsin to reverse direction
void EPD_2D_Rotate(float &tX, float &tY, float tCos, float tSin)
{
    float tTemp;
    tTemp = tX * tCos - tY * tSin;
    tY = tX * tSin + tY *  tCos;
    tX = tTemp;
}

static void EPD_Draw_NavBoxes() 
{
 // Serial.println("TIMER E4  - start draw_navboxes");// WAGA debug timings 140425
  int16_t  tbx, tby;
  uint16_t tbw, tbh;
  uint16_t top_navboxes_x, top_navboxes_y, top_navboxes_w, top_navboxes_h;
  uint16_t bottom_navboxes_x, bottom_navboxes_y, bottom_navboxes_w, bottom_navboxes_h;
  char info_line [TEXT_VIEW_LINE_LENGTH];
  int Traffic_count();
  uint8_t db=DB_OGN;
  
  char id_text [TEXT_VIEW_LINE_LENGTH]; 
  char id2_text [TEXT_VIEW_LINE_LENGTH]; 
 
  static char *u_dist, *u_alt,*u_spd;
  static float disp_dist, disp_alt, disp_spd;
  
 // WAGA 070425 --- set up units
  switch (settings->units)
    {
    case UNITS_IMPERIAL:
      u_dist = "nm";
      u_alt  = "f";
      u_spd  = "kt";
      disp_dist = 1000 *_GPS_MILES_PER_METER/_GPS_MPH_PER_KNOT; 
      disp_alt  = _GPS_FEET_PER_METER;
      disp_spd  = 1;  // TinyGPS gives speed in Kts - I think! 
      break;
    case UNITS_MIXED:
      u_dist = "km";
      u_alt  = "f";
      u_spd  = "kph";
      disp_dist = 1;
      disp_alt  = _GPS_FEET_PER_METER;
      disp_spd  = 3.6 * _GPS_MPS_PER_KNOT;
      break;
    case UNITS_METRIC:
    default:
      u_dist = "km";
      u_alt  = "m";
      u_spd  = "kph";
      disp_dist = 1;
      disp_alt  = 1;
      disp_spd  = 3.6 * _GPS_MPS_PER_KNOT;
      break;
    }

  switch (EPD_portland)
  {
  case SCREEN_PORTRAIT:

     top_navboxes_x = navbox1.x;
     top_navboxes_y = navbox1.y;
     top_navboxes_w = navbox1.width + navbox2.width;
     top_navboxes_h = maxof2(navbox1.height, navbox2.height);
      
      bottom_navboxes_x = navbox3.x;
      bottom_navboxes_y = navbox3.y;
      bottom_navboxes_w = navbox3.width + navbox4.width;
      bottom_navboxes_h = maxof2(navbox3.height, navbox4.height);
      break;

    case SCREEN_LANDSCAPE:
    
      top_navboxes_x = navbox1.x;
      top_navboxes_y = navbox1.y;
      top_navboxes_w = navbox1.width;
      top_navboxes_h = navbox1.height + navbox2.height;
      
      bottom_navboxes_x = navbox3.x;
      bottom_navboxes_y = navbox3.y;
      bottom_navboxes_w = navbox3.width;
      bottom_navboxes_h = navbox3.height+navbox4.height;
      break;
  }
// --- top nav boxes
  {
    display->fillRect(top_navboxes_x, top_navboxes_y,
                      top_navboxes_w, top_navboxes_h,
                      GxEPD_WHITE);

    display->drawRoundRect( navbox1.x + 1, navbox1.y + 1,
                            navbox1.width - 2, navbox1.height - 2,
                            4, GxEPD_BLACK);

    display->drawRoundRect( navbox2.x + 1, navbox2.y + 1,
                            navbox2.width - 2, navbox2.height - 2,
                            4, GxEPD_BLACK);

    display->setFont(&Picopixel);

    display->getTextBounds(navbox1.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor(navbox1.x + 5, navbox1.y + 5 + tbh);
    display->print(navbox1.title);
    
    display->getTextBounds(navbox2.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor(navbox2.x + 5, navbox2.y + 5 + tbh);
    display->print(navbox2.title);

  // NavBox1 - Threat distance/alarm_level is common to Radar and Nav views.  Threat_Assess() returns count of traffic
    if (Threat_Assess()>0){
        
      display->setCursor(navbox1.x + 55, navbox1.y + 5 + tbh);
      display->setFont(&Picopixel);
        switch (traffic[0].fop->Source)
        {
          case 0:
            display->print("FLARM");
            break;
          case 1:
            display->print("ADSB");
            break;
          case 6:
           display->print("MODE-S");
            break;
          default:
            /* TBD */
          break;
        }
            

      if(traffic[0].fop->alarm_level>0){
        display->setFont(&FreeMonoBold9pt7b);
        display->setCursor(navbox1.x + 5, navbox1.y + 32);
        snprintf(info_line, sizeof(info_line),"ALARM %d",(int8_t) traffic[0].fop->alarm_level);
        display->print(info_line);
      }
      else{
        display->setFont(&FreeSerifBold12pt7b);
        display->setCursor(navbox1.x + 10, navbox1.y + 32);
        navbox1.value = traffic[0].fop->distance;
        snprintf(info_line, sizeof(info_line), "%4.1f",(float)navbox1.value /1000 *disp_dist);
        display->print(info_line);

        display->setCursor(navbox1.x + 60, navbox1.y + 32); 
        display->setFont(&FreeMonoBold9pt7b);
        display->print(u_dist); 
      }
    }else{
      display->setFont(&FreeSerifBold12pt7b);
      display->setCursor(navbox1.x + 30, navbox1.y + 32);
      navbox1.value=0; // only zero when no traffic
      display->print(navbox1.value);
    }
    
    // NavBox2 ------
    switch(EPD_view_mode){
      case VIEW_MODE_RADAR:{
       
        display->setFont(&FreeSerifBold12pt7b);
        display->setCursor(navbox2.x + 30, navbox2.y + 30);
        navbox2.value= Traffic_Count();
        display->print(navbox2.value); 
       
        display->setCursor(navbox2.x + 50, navbox2.y + 5 + tbh);
        display->setFont(&Picopixel);
        //display->print(settings->voice);
        
        // WAGA show current Sound setting
        switch (settings->voice){
          case VOICE_ON:
            display->print("VOICE");
            break;
          case VOICE_BUZZER:
             display->print("BUZZER");
             break;
          default:
          display->print("OFF"); 
        }    
     
       break;
      }
      case VIEW_MODE_NAV:{
        ThisAircraft_NavData(); // function sets .value for navbox2, navbox3, navbox4
        display->setFont(&FreeSerifBold12pt7b);
        display->setCursor(navbox2.x + 20, navbox2.y + 30);
        snprintf(info_line, sizeof(info_line), "%03d",navbox2.value);
        display->print(info_line);
        
        display->setCursor(navbox2.x + 55, navbox2.y + 5 + tbh);  // WAGA waypoint
        display->setFont(&Picopixel);
        display->print(Nav_Names[settings->waypoint]);
        break;
      }
    }
    
  }
  //---- bottom nav boxes
  {
    display->fillRect(bottom_navboxes_x, bottom_navboxes_y,
                      bottom_navboxes_w, bottom_navboxes_h,
                      GxEPD_WHITE);

    display->drawRoundRect( navbox3.x + 1, navbox3.y + 1,
                            navbox3.width - 2, navbox3.height - 2,
                            4, GxEPD_BLACK);
    display->drawRoundRect( navbox4.x + 1, navbox4.y + 1,
                            navbox4.width - 2, navbox4.height - 2,
                            4, GxEPD_BLACK);

    display->setFont(&Picopixel);

    display->getTextBounds(navbox3.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor(navbox3.x + 5, navbox3.y + 5 + tbh);
    display->print(navbox3.title);

    display->getTextBounds(navbox4.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    display->setCursor(navbox4.x + 5, navbox4.y + 5 + tbh);
    display->print(navbox4.title);

    display->setFont(&FreeSerifBold12pt7b);

         
    switch(EPD_view_mode){ 
      case VIEW_MODE_RADAR:
      {
        if (Threat_Assess()==0){
          break;  // WAGA 070425 if there is no traffic, then can't set navboxes 3 and 4 values
        }
   
        navbox3.value = int(traffic[0].fop->RelativeVertical *disp_alt); // vertical distance
        display->setCursor(navbox3.x + 4, navbox3.y + 30); 
        snprintf(info_line, sizeof(info_line), "%+4d",navbox3.value);// distance to waypoint 
        display->print(info_line);
        
        display->setCursor(navbox3.x + 70, navbox3.y + 30);
        display->setFont(&FreeMonoBold9pt7b);
        display->print(u_alt);
        
        // --- lookup and display Comp ID
        display->setCursor(navbox4.x + 55, navbox4.y + 5 + tbh);
        display->setFont(&Picopixel);
        uint32_t id =0;
        if (traffic[0].fop->ID != 0){
          id = traffic[0].fop->ID; 
          snprintf(info_line, sizeof(info_line), "%06X", id);
        }else{
            snprintf(info_line, sizeof(info_line), "Nil traffic");
          }
        display->print(info_line);

        display->setCursor(navbox4.x + 15, navbox4.y + 30);
        display->setFont(&FreeSerifBold12pt7b);
        int found = SoC->DB_query(db,id,id_text, sizeof(id_text), id2_text, sizeof (id2_text));
        if (found==1){
          String tail = rightchr(id_text,5);
          display->print(tail);
        } else {
          display->print ("  -  ");
        }

        break;
      }
      case VIEW_MODE_NAV:
      
        display->setCursor(navbox3.x + 8, navbox3.y + 30);
        display->setFont(&FreeSerifBold12pt7b);
        snprintf(info_line, sizeof(info_line), "%4.1f",(float)navbox3.value *disp_dist);// distance to waypoint 
        display->print(info_line);

        display->setCursor(navbox3.x + 62, navbox3.y + 32);
        display->setFont(&FreeMonoBold9pt7b);
        display->print(u_dist);
        
        display->setCursor(navbox3.x + 55, navbox3.y + 5 + tbh);
        display->setFont(&Picopixel);
        // WAGA show current source setting
        switch (settings->connection){
          case CON_SERIAL:
            display->print("SERIAL");
            break;
          case CON_WIFI_UDP:
             display->print("WIFI");
             break;
          case CON_BLUETOOTH_SPP:
             display->print("BT_SPP");
             break;
          case CON_BLUETOOTH_LE:
          display->print("BT_LE");
          break; 
        }


        display->setCursor(navbox4.x + 15, navbox4.y + 30);
        display->setFont(&FreeSerifBold12pt7b);
        display->print((int)(navbox4.value * disp_spd)); 

        display->setCursor(navbox4.x + 53, navbox4.y + 32);
        display->setFont(&FreeMonoBold9pt7b);
        display->print(u_spd);



        break;
    }
  }
  //Serial.println("TIMER F - end of Draw_NavBoxes");  // WAGA debug timings 140425
}


void EPD_radar_Draw_Message(const char *msg1, const char *msg2)
{
  int16_t  tbx, tby;
  uint16_t tbw, tbh;
  uint16_t x, y;
  uint16_t radar_x, radar_y, radar_w;

  if (msg1 != NULL && strlen(msg1) != 0) {
    
    switch (EPD_portland)
    {
      case SCREEN_PORTRAIT:
       radar_x = 0;
        radar_y = (display->height() - display->width()) / 2;
        radar_w = display->width();
        break;
      case SCREEN_LANDSCAPE:
        radar_x = 0;
        radar_y =0;
        radar_w =display->height();
        break;
    }
    
    display->setFont(&FreeMonoBold18pt7b);

    {
      display->fillRect(radar_x, radar_y, radar_w, radar_w, GxEPD_WHITE);

      if (msg2 == NULL) {
        display->getTextBounds(msg1, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = (radar_w - tbw) / 2;
        y = radar_y + (radar_w + tbh) / 2;
        display->setCursor(x, y);
        display->print(msg1);
      } else {
        display->getTextBounds(msg1, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = (radar_w - tbw) / 2;
        y = radar_y + radar_w / 2 - tbh;
        display->setCursor(x, y);
        display->print(msg1);

        display->setFont(&FreeMonoBold9pt7b);
        display->getTextBounds(msg2, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = (radar_w - tbw) / 2;
        y = radar_y + radar_w / 2 + tbh;
        display->setCursor(x, y);
        display->print(msg2);
      }
    }
  }
}

static void EPD_Draw_Radar()

{
  //Serial.println(" TIMER E0 - start EPD_draw_radar"); // WAGA debug timings 140425
  int16_t  tbx, tby;
  uint16_t tbw, tbh;
  int16_t x;
  int16_t y;
  char cog_text[6];
  float range;      /* distance at edge of radar display */
  int16_t scale;

  uint16_t radar_x = 0;
  uint16_t radar_y = 0; 
  uint16_t radar_w = 0;
  
  /* divider is distance range */
  int32_t divider = 2000; // default 2000m 

  display->setFont(&FreeMono9pt7b);
  display->getTextBounds("N", 0, 0, &tbx, &tby, &tbw, &tbh);

  switch (EPD_portland){
    case SCREEN_PORTRAIT:
       radar_x = 0;
        radar_y = (display->height() - display->width()) / 2;
        radar_w = display->width();
        break;
    case SCREEN_LANDSCAPE:
        radar_x = 0;
        radar_y = 0;
        radar_w = display->height();
        break;
  }

   display->fillRect(radar_x, radar_y, radar_w, radar_w, GxEPD_WHITE);

  int16_t radar_center_x = radar_w / 2;
  int16_t radar_center_y = radar_y + radar_w / 2;
  int16_t radius = radar_w / 2 - 2;

  float epd_Points[MAX_DRAW_POINTS][2];

// set range depending on Zoom  and units
  if (settings->units == UNITS_METRIC || settings->units == UNITS_MIXED) {
    switch(EPD_zoom)
    {
    case ZOOM_LOWEST:
      range = 10000.0;  /* 20 KM */
      break;
    case ZOOM_LOW:
      range = 5000.0;   /* 10 KM */
      break;
    case ZOOM_HIGH:
      range = 1000.0;   /*  2 KM */
      break;
    case ZOOM_MEDIUM:
    default:
      range = 2000.0;   /*  4 KM */
      break;
    }
  } else {
    switch(EPD_zoom)
    {
    case ZOOM_LOWEST:
      range = 9260.0;   /* 10 NM */
      break;
    case ZOOM_LOW:
      range = 4630.0;   /*  5 NM */
      break;
    case ZOOM_HIGH:
      range = 926.0;    /*  1 NM */
      break;
    case ZOOM_MEDIUM:
    default:
      range = 1852.0;   /*  2 NM */
      break;
    }
  }
  
float radius_range = (float)radius / range;

  {
    float trSin = sin(D2R * (float)ThisAircraft.Track);
    float trCos = cos(D2R * (float)ThisAircraft.Track);
    //Serial.println("TIMER E1 - start container loop"); // WAGA debug timings 100425

// ---start loop reading container (not sorted for threat) to display all targets
    for (int i=0; i < MAX_TRACKING_OBJECTS; i++) {
      if (Container[i].ID && (now() - Container[i].timestamp) <= EPD_EXPIRATION_TIME) {

        float rel_x;
        float rel_y;
        float tgtSin;
        float tgtCos;

        bool isTeam = (Container[i].ID == settings->team) ;  // Not used by WAGA

        rel_x = Container[i].RelativeEast;
        if (fabs(rel_x) > range)  continue;  // only use targets within radar display range
        rel_y = Container[i].RelativeNorth;
        if (fabs(rel_y) > range)  continue;
        
	    	tgtSin = sin(D2R * (float)Container[i].Track);
        tgtCos = cos(D2R * (float)Container[i].Track);

        for (int i=0; i < ICON_TARGETS_POINTS; i++) {
          epd_Points[i][0] = epd_Target[i][0];
          epd_Points[i][1] = epd_Target[i][1];
          EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], tgtCos, tgtSin); // rotate icon clockwise
        }
#if 0
        Serial.print(F(" ID="));
        Serial.print((Container[i].ID >> 16) & 0xFF, HEX);
        Serial.print((Container[i].ID >>  8) & 0xFF, HEX);
        Serial.print((Container[i].ID      ) & 0xFF, HEX);
        Serial.println();

        Serial.print(F(" RelativeNorth=")); Serial.println(Container[i].RelativeNorth);
        Serial.print(F(" RelativeEast="));  Serial.println(Container[i].RelativeEast);
#endif
        switch (settings->orientation) {
          case DIRECTION_NORTH_UP:
            break;
          case DIRECTION_TRACK_UP:
            // rotate (anti-clockwise) relative to ThisAircraft.Track
            EPD_2D_Rotate(rel_x, rel_y, trCos, trSin);// rotate targets anti-clockwise
            for (int i=0; i < ICON_TARGETS_POINTS; i++)
               EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], trCos, -trSin);  // -trsin, so rotate icon anti-clockwise
            break;
          default:
            /* TBD */
            break;
        }

#if 0
      Serial.print(F("Debug "));
      Serial.print(trSin);
      Serial.print(F(", "));
      Serial.print(trCos);
      Serial.print(F(", "));
      Serial.print(rel_x);
      Serial.print(F(", "));
      Serial.print(rel_y);
      Serial.println();
      Serial.flush();
#endif
#if 0
      Serial.print(F("Debug "));
      Serial.print(tgtSin);
      Serial.print(F(", "));
      Serial.print(tgtCos);
      Serial.print(F(", "));
      Serial.print(epd_Points[1][0]);
      Serial.print(F(", "));
      Serial.print(epd_Points[1][1]);
      Serial.println();
      Serial.flush();
#endif

    	x = (int16_t) (rel_x * radius_range);
      y = (int16_t) (rel_y * radius_range);
      
      // ====== DRAWING ICONS ON RADAR = WAGA modified Source=6 is mode-s non- directional ==============
      if(Container[i].Source!=6){

        //WAGA debug
        /*Serial.print("--draw icons - i="); Serial.print(i);
          uint32_t id=0;
          char info_line [20];
          id = Container[i].ID; 
          snprintf(info_line, sizeof(info_line), " ID= %06X", id);  // WAGA debug
          Serial.println(info_line);
          */


        // -- draw a line to threat aircraft    
        if (Container[i].ID==traffic[0].fop->ID){   
          display->drawLine(radar_center_x, radar_center_y, radar_center_x + x, radar_center_y - y,GxEPD_BLACK);  
        }

        // draw target as a simple triangle
          display->fillTriangle(radar_center_x + x + (int16_t) round(epd_Points[0][0]),
                                radar_center_y - y + (int16_t) round(epd_Points[0][1]),
                                radar_center_x + x + (int16_t) round(epd_Points[1][0]),
                                radar_center_y - y + (int16_t) round(epd_Points[1][1]),
                                radar_center_x + x + (int16_t) round(epd_Points[2][0]),
                                radar_center_y - y + (int16_t) round(epd_Points[2][1]),
                                GxEPD_BLACK);
          // draw circle around tugs
          if (Container[i].AcftType==2){
            display->drawCircle(radar_center_x + x,
                                radar_center_y - y,
                                9,
                                GxEPD_BLACK);
          }
          // draw 2 circles if ADSB
          //Serial.println(Container[i].Source);
          if (Container[i].Source==1){
            display->drawCircle(radar_center_x + x,
                                radar_center_y - y,
                                10,
                                GxEPD_BLACK);
             display->drawCircle(radar_center_x + x,
                                radar_center_y - y,
                                11,
                                GxEPD_BLACK);                   
          }
          // ---Draw +/- by target icon to indicate above or below
          int16_t x3 = radar_center_x + x + (int16_t) round(epd_Points[3][0]);
          int16_t y3 = radar_center_y - y + (int16_t) round(epd_Points[3][1]);
          if (Container[i].RelativeVertical >   EPD_RADAR_V_THRESHOLD) {
          // draw a '+' next to target triangle
          display->drawLine(x3 - 2,
                            y3,
                            x3 + 2,
                            y3,
                            GxEPD_BLACK);
          display->drawLine(x3,
                            y3 + 2,
                            x3,
                            y3 - 2,
                            GxEPD_BLACK);
        } else if (Container[i].RelativeVertical < - EPD_RADAR_V_THRESHOLD) {
          // draw a '-' next to target triangle
          display->drawLine(x3 - 2,
                            y3,
                            x3 + 2,
                            y3,
                            GxEPD_BLACK);
        }
        }else {
          //WAGA draw a ring of dots for non-directional targets

          // WAGA debug
          Serial.print("--draw ring of dots - i="); Serial.print(i);
          uint32_t id=0;
          char info_line [20];
          id = Container[i].ID; 
          snprintf(info_line, sizeof(info_line), " ID= %06X", id);
          Serial.println(info_line);

          float ring_x=0;
          float ring_y=0;
          for (int A=0; A < DOTS_RING_POINTS; A++) {
          ring_x = radar_center_x + (y * dot_ring[A][0]);
          ring_y = radar_center_y + (y * dot_ring[A][1]);
          display->fillCircle(ring_x, ring_y, 1 ,GxEPD_BLACK);
          }
        } 
  
  
        // if Team match, draw a circle around target  // --WAGA 240325 not used
       /* if (isTeam) {
          /*
          display->drawCircle(radar_center_x + x,
                              radar_center_y - y,
                              7, GxEPD_BLACK);
          
          display->drawCircle(radar_center_x + x,
                              radar_center_y - y,
                              9, GxEPD_BLACK); 
          display->drawCircle(radar_center_x + x,
                              radar_center_y - y,
                              11, GxEPD_BLACK);   
            */                

        //}
      } // end of IF testing for Source =6

    }  //------end of loop going through container
  //Serial.println("TIMER E2 - end of container loop "); // WAGA debug timings 100425

    // draw range circles
    display->drawCircle(radar_center_x, radar_center_y, radius,   GxEPD_BLACK);
    //display->drawCircle(radar_center_x, radar_center_y, radius/2, GxEPD_BLACK);  // WAGA only single scale circle 

    // WAGA 260325 === FLARM ALARM LOOK-OUT POINTER =============
           
    if (Threat_Assess()!=0 && traffic[0].fop->alarm_level>0 && traffic[0].fop->Source!=6){

      float threatSin = sin(D2R * (float)traffic[0].fop->RelativeBearing);
      float threatCos = cos(D2R * (float)traffic[0].fop->RelativeBearing);

      for (int i=0; i < ICON_LOOK_POINTS; i++) {
          epd_Points[i][0] = epd_Look[i][0];
          epd_Points[i][1] = epd_Look[i][1]-radius;
          EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], threatCos, threatSin); // rotate Look points clockwise to relative bearing
      }

      int16_t x3 = radar_center_x + (int16_t) round(epd_Points[9][0]);
      int16_t y3 = radar_center_y + (int16_t) round(epd_Points[9][1]);

             
      // draw look-out indicator ------
      display->fillTriangle(radar_center_x + (int16_t) round(epd_Points[0][0]),
                             radar_center_y + (int16_t) round(epd_Points[0][1]),
                             radar_center_x + (int16_t) round(epd_Points[1][0]),
                             radar_center_y + (int16_t) round(epd_Points[1][1]),
                             radar_center_x + (int16_t) round(epd_Points[2][0]),
                             radar_center_y + (int16_t) round(epd_Points[2][1]),
                             GxEPD_BLACK);
        // ---Draw +/- in filled black triangle to indicate above or below
        if (traffic[0].fop->RelativeVertical>   EPD_RADAR_V_THRESHOLD) {
          // draw a '+' 
          display->drawLine(x3 - 3,
                            y3,
                            x3 + 3,
                            y3,
                            GxEPD_WHITE);
          display->drawLine(x3,
                            y3 + 3,
                            x3,
                            y3 - 3,
                            GxEPD_WHITE);
        } else if (traffic[0].fop->RelativeVertical < - EPD_RADAR_V_THRESHOLD) {
          // draw a '-' 
          display->drawLine(x3 - 3,
                            y3,
                            x3 + 3,
                            y3,
                            GxEPD_WHITE);
        }  
      // end of draw solid lookout triangle
      if (traffic[0].fop->alarm_level>1){
        display->drawTriangle(radar_center_x + (int16_t) round(epd_Points[3][0]),
                             radar_center_y + (int16_t) round(epd_Points[3][1]),
                             radar_center_x + (int16_t) round(epd_Points[4][0]),
                             radar_center_y + (int16_t) round(epd_Points[4][1]),
                             radar_center_x + (int16_t) round(epd_Points[5][0]),
                             radar_center_y + (int16_t) round(epd_Points[5][1]),
                             GxEPD_BLACK);
      }
      if (traffic[0].fop->alarm_level>2){
      display->drawTriangle(radar_center_x + (int16_t) round(epd_Points[6][0]),
                             radar_center_y + (int16_t) round(epd_Points[6][1]),
                             radar_center_x + (int16_t) round(epd_Points[7][0]),
                             radar_center_y + (int16_t) round(epd_Points[7][1]),
                             radar_center_x + (int16_t) round(epd_Points[8][0]),
                             radar_center_y + (int16_t) round(epd_Points[8][1]),
                             GxEPD_BLACK);
      }
    }else{
      //Serial.println(" -- no alarm to show"); 

    }
 //Serial.println("---Timer-check end of lookout "); // WAGA 100425

    // Print range on radar bottom left WAGA 050325
    display->setFont(&FreeMonoBold9pt7b);
    display->setCursor(radar_x+2,radar_y+radar_w-2);
       if (settings->units == UNITS_METRIC || settings->units == UNITS_MIXED) {
          display->print(EPD_zoom == ZOOM_LOWEST ? "10 km" :
                        EPD_zoom == ZOOM_LOW    ? "5 km" :
                        EPD_zoom == ZOOM_MEDIUM ? "2 km" :
                        EPD_zoom == ZOOM_HIGH   ? "1 km" : "");
        } else {
          display->print(EPD_zoom == ZOOM_LOWEST ? " 5 nm" :
                        EPD_zoom == ZOOM_LOW    ? "2.5nm" :
                        EPD_zoom == ZOOM_MEDIUM ? " 1 nm" :
                        EPD_zoom == ZOOM_HIGH   ? "0.5nm" : "");
        }

#if defined(ICON_AIRPLANE)
    /* draw ThisAircraft little airplane icon*/
    for (int i=0; i < ICON_AIRPLANE_POINTS; i++) {
    epd_Points[i][0] = epd_Airplane[i][0];
    epd_Points[i][1] = epd_Airplane[i][1];
    }
   
    switch (settings->orientation)
    {
    case DIRECTION_NORTH_UP:
      // draw icon airplane ThisAircraft.Track relative to North (clockwise rotation)
        for (int i=0; i < ICON_AIRPLANE_POINTS; i++) {
        EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], trCos, trSin);
      }
      break;
    case DIRECTION_TRACK_UP:
      break;
    default:
      /* TBD */
      break;
    }
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[0][0]),
                      radar_center_y + (int16_t) round(epd_Points[0][1]),
                      radar_center_x + (int16_t) round(epd_Points[1][0]),
                      radar_center_y + (int16_t) round(epd_Points[1][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[2][0]),
                      radar_center_y + (int16_t) round(epd_Points[2][1]),
                      radar_center_x + (int16_t) round(epd_Points[3][0]),
                      radar_center_y + (int16_t) round(epd_Points[3][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[4][0]),
                      radar_center_y + (int16_t) round(epd_Points[4][1]),
                      radar_center_x + (int16_t) round(epd_Points[5][0]),
                      radar_center_y + (int16_t) round(epd_Points[5][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[6][0]),
                      radar_center_y + (int16_t) round(epd_Points[6][1]),
                      radar_center_x + (int16_t) round(epd_Points[7][0]),
                      radar_center_y + (int16_t) round(epd_Points[7][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[8][0]),
                      radar_center_y + (int16_t) round(epd_Points[8][1]),
                      radar_center_x + (int16_t) round(epd_Points[9][0]),
                      radar_center_y + (int16_t) round(epd_Points[9][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[10][0]),
                      radar_center_y + (int16_t) round(epd_Points[10][1]),
                      radar_center_x + (int16_t) round(epd_Points[11][0]),
                      radar_center_y + (int16_t) round(epd_Points[11][1]),
                      GxEPD_BLACK);
#else  //ICON_AIRPLANE
    /* draw arrow tip */
    for (int i=0; i < ICON_ARROW_POINTS; i++) {
      epd_Points[i][0] = epd_Arrow[i][0];
      epd_Points[i][1] = epd_Arrow[i][1];
    }
    switch (settings->orientation) 
    {
    case DIRECTION_NORTH_UP:
    // rotate (anti-clockwise) targets relative to ThisAircraft.Track
    for (int i=0; i < ICON_ARROW_POINTS; i++) {
      EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], trCos, trSin);
    }
      break;
    case DIRECTION_TRACK_UP:
      break;
    default:
      /* TBD */
      break;
    }
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[0][0]),
                      radar_center_y + (int16_t) round(epd_Points[0][1]),
                      radar_center_x + (int16_t) round(epd_Points[1][0]),
                      radar_center_y + (int16_t) round(epd_Points[1][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[1][0]),
                      radar_center_y + (int16_t) round(epd_Points[1][1]),
                      radar_center_x + (int16_t) round(epd_Points[2][0]),
                      radar_center_y + (int16_t) round(epd_Points[2][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[2][0]),
                      radar_center_y + (int16_t) round(epd_Points[2][1]),
                      radar_center_x + (int16_t) round(epd_Points[3][0]),
                      radar_center_y + (int16_t) round(epd_Points[3][1]),
                      GxEPD_BLACK);
    display->drawLine(radar_center_x + (int16_t) round(epd_Points[3][0]),
                      radar_center_y + (int16_t) round(epd_Points[3][1]),
                      radar_center_x + (int16_t) round(epd_Points[0][0]),
                      radar_center_y + (int16_t) round(epd_Points[0][1]),
                      GxEPD_BLACK);
#endif //ICON_AIRPLANE

    switch (settings->orientation)
    {
    case DIRECTION_NORTH_UP:
      // draw N, W, E, S  -- WAGA only display N
      x = radar_x + (radar_w - tbw) / 2;
      y = radar_y + radar_w/2 - radius + (3 * tbh)/2;
      display->setCursor(x , y);
      display->print("N");
      break;
      /*----- not used by WAGA
      x = radar_x + radar_w / 2 - radius + tbw/2;
      y = radar_y + (radar_w + tbh) / 2;
      display->setCursor(x , y);
      display->print("W");
      x = radar_x + radar_w / 2 + radius - (3 * tbw)/2;
      y = radar_y + (radar_w + tbh) / 2;
      display->setCursor(x , y);
      display->print("E");
            x = radar_x + (radar_w - tbw) / 2;
      y = radar_y + radar_w/2 + radius - tbh/2;
      display->setCursor(x , y);
      display->print("S");
      break;
      */
    case DIRECTION_TRACK_UP:
      // draw L, R, B 
      /* -- not used by WAGA
      x = radar_x + radar_w / 2 - radius + tbw/2;
      y = radar_y + (radar_w + tbh) / 2;
      display->setCursor(x , y);
      display->print("L");
      x = radar_x + radar_w / 2 + radius - (3 * tbw)/2;
      y = radar_y + (radar_w + tbh) / 2;
      display->setCursor(x , y);
      display->print("R");
      x = radar_x + (radar_w - tbw) / 2;
      y = radar_y + radar_w/2 + radius - tbh/2;
      display->setCursor(x , y);
      display->print("B");
      */

      // draw aircraft heading top centre of radar square
      display->setFont(&FreeMonoBold9pt7b);
      snprintf(cog_text, sizeof(cog_text), "%03d", ThisAircraft.Track);
      display->getTextBounds(cog_text, 0, 0, &tbx, &tby, &tbw, &tbh);
      x = radar_x + (radar_w - tbw) / 2;
      y = radar_y + radar_w/2 - radius + (3 * tbh)/2;
      display->setCursor(x , y);
      display->print(cog_text);
      display->drawRoundRect(x-2, y-tbh-2, tbw+8, tbh+6, 4, GxEPD_BLACK);
      break;
    default:
      /* TBD */
      break;
    }
  }
  //Serial.println("TIMER E3 - end Draw_Radar ");  // WAGA debug timings 
}

void EPD_radar_setup()
{
  EPD_zoom = settings->zoom; 
  uint16_t radar_x = 0;
  uint16_t radar_y = 0;
  uint16_t radar_w = 0;
 
  pinMode(SOC_GPIO_PIN_BUZZ_T5S, OUTPUT);

  switch (EPD_portland){

    case SCREEN_PORTRAIT:
    {
      radar_x = 0;
      radar_y = (display->height() - display->width()) / 2;
      radar_w = display->width();

      navbox1.x = 0;
      navbox1.y = 0;
      navbox1.width  = display->width()/2;
      navbox1.height = (display->height() - display->width()) / 2;

      navbox2.x = navbox1.width;
      navbox2.y = 0;
      navbox2.width  = navbox1.width;
      navbox2.height = navbox1.height;

      navbox3.x = 0;
      navbox3.y = radar_y + radar_w;
      navbox3.width  = navbox1.width;
      navbox3.height = navbox1.height;  

      navbox4.x = navbox3.width;
      navbox4.y = navbox3.y;
      navbox4.width  = navbox1.width;
      navbox4.height = navbox1.height; 
      break;
    }
    case SCREEN_LANDSCAPE:
    {
      radar_x = 0;
      radar_y = 0;
      radar_w = display->height();

      navbox1.x = display->height();
      navbox1.y = 0;
      navbox1.width  = display->width() - display->height();
      navbox1.height = display->height()/4;

      navbox2.x = navbox1.x;
      navbox2.y = navbox1.height;
      navbox2.width  = navbox1.width;
      navbox2.height = navbox1.height;

      navbox3.x = navbox1.x;
      navbox3.y = navbox1.height *2;
      navbox3.width  = navbox1.width;
      navbox3.height = navbox1.height;  

      navbox4.x = navbox1.x;
      navbox4.y = navbox1.height *3;
      navbox4.width  = navbox1.width;
      navbox4.height = navbox1.height;  
      break;
    }
 
  }

 // set the Navbox titles and initial value and timestamp   
  switch(EPD_view_mode)
  {
    case VIEW_MODE_RADAR:
      memcpy(navbox1.title, NAVBOX1_TITLE, strlen(NAVBOX1_TITLE)); 
      navbox1.value      = 0;
      navbox1.timestamp  = millis();

      memcpy(navbox2.title, NAVBOX2_TITLE, strlen(NAVBOX2_TITLE));
      navbox2.value      = 0;  // was = PROTOCOL_NONE;
      navbox2.timestamp  = millis();

      //EPD_zoom = settings->zoom;
      memcpy(navbox3.title, NAVBOX3_TITLE, strlen(NAVBOX3_TITLE));
      navbox3.value      = 0;  // was = EPD_zoom;
      navbox3.timestamp  = millis();

      memcpy(navbox4.title, NAVBOX4_TITLE, strlen(NAVBOX4_TITLE));
      navbox4.value      = 0; // was = (int) (Battery_voltage() * 10.0);
      navbox4.timestamp  = millis();
      break;  
    case VIEW_MODE_NAV:
      memcpy(navbox1.title,NAVBOX1_TITLE_NAV,strlen(NAVBOX1_TITLE_NAV));
      navbox1.value = 0;
      navbox1.timestamp  = millis();

      memcpy(navbox2.title,NAVBOX2_TITLE_NAV,strlen(NAVBOX2_TITLE_NAV));
      navbox2.value = 0;
      navbox2.timestamp  = millis();

      memcpy(navbox3.title,NAVBOX3_TITLE_NAV,strlen(NAVBOX3_TITLE_NAV));
      navbox3.value = 0;
      navbox3.timestamp  = millis();

      memcpy(navbox4.title,NAVBOX4_TITLE_NAV,strlen(NAVBOX4_TITLE_NAV));
      navbox4.value = 0;
      navbox4.timestamp  = millis();
      break;
  }
}


void EPD_radar_loop()
{

  //Serial.println("TIMER E - start EPD_radar_loop");  //WAGA debug timings 160525

  char info_line [20];
  bool RadarNow = false;
  if ((Threat_Assess()!=0 && traffic[0].fop->alarm_level>0) || isTimeToDisplay()){
    RadarNow = true;
  }

  //Serial.print("EPD_is_ready = ");Serial.println(SoC->EPD_is_ready());  // WAGA debug timings 160425

    //if (isTimeToDisplay() && SoC->EPD_is_ready()) {   /// WAGA 150425 amended
    if (RadarNow && SoC->EPD_is_ready()) {

      //* -- WAGA testing
      if (Threat_Assess()!=0 && traffic[0].fop->alarm_level>0){
          //Serial.print("RadarNow due to traffic alarm >0   ID=");
          uint32_t id=0;
          char info_line [20];
          int ID_Alarm = traffic[0].fop->alarm_level;
          id = traffic[0].fop->ID; 
          snprintf(info_line, sizeof(info_line), " %06X  alarm= %d", id,ID_Alarm); 
          Serial.println(info_line);

        }else{
          //Serial.println("RadarNow due to isTimeToDisplay");
        }
      //* - end debug

    bool hasData = settings->protocol == PROTOCOL_NMEA  ? NMEA_isConnected()  :
                   settings->protocol == PROTOCOL_GDL90 ? GDL90_isConnected() :
                   false;

    if (hasData) {

      bool hasFix = settings->protocol == PROTOCOL_NMEA  ? isValidGNSSFix()   :
                    settings->protocol == PROTOCOL_GDL90 ? GDL90_hasOwnShip() :
                    false;

      if (hasFix) {
        EPD_Draw_Radar();
      } else  
        {
          Serial.println("no fix - so NO EPD_Draw_radar");  // WAGA debug timings 160425
          
          String datatype ="";
          switch(settings->protocol)
          {
          case 0:
            datatype = "data is UNK";
            break;
          case 1:
            datatype = "data is NMEA";
            break;
          case 2:
            datatype = "data is GDL90";
            break;
          default:
            datatype = "data is Other";
            break;
          }
        
        //snprintf(info_line,sizeof(info_line), "Protocol %d", settings->protocol);
        EPD_radar_Draw_Message(NO_FIX_TEXT, datatype.c_str());
      }
    } else {
      snprintf(info_line,sizeof(info_line), "Con/Baud %d / %d", settings->connection, settings->baudrate);
      EPD_radar_Draw_Message(NO_DATA_TEXT,info_line);
    }
        
      EPD_Draw_NavBoxes(); 

      SoC->EPD_update(EPD_UPDATE_FAST);

      EPDTimeMarker = millis();
    
  }
  
}

void ThisAircraft_NavData() // WAGA version
{
  static float Way_lat = Nav_Waypoints[settings->waypoint][0];  // from array of Waypoints
  static float Way_lng = Nav_Waypoints[settings->waypoint][1];
  double distanceKm = TinyGPSPlus::distanceBetween(ThisAircraft.latitude, ThisAircraft.longitude, Way_lat,Way_lng)/1000;  // WAGA
  double courseTo = TinyGPSPlus::courseTo(ThisAircraft.latitude, ThisAircraft.longitude, Way_lat,Way_lng);  // WAGA
        
  bool hasFix = settings->protocol == PROTOCOL_NMEA  ? isValidGNSSFix()   :
                settings->protocol == PROTOCOL_GDL90 ? GDL90_hasOwnShip() :
                false;
        
  if (hasFix) {
    navbox2.value = courseTo;
    navbox3.value = distanceKm;
    navbox4.value = ThisAircraft.GroundSpeed;  // kts 
  }else{
    navbox2.value = 0;
    navbox3.value = 0;
    navbox4.value = 0;
  }
  //Serial.print(F("Latitude")); Serial.println(ThisAircraft.latitude,4);
  //Serial.print(F("Longitude")); Serial.println(ThisAircraft.longitude,4);
}



void EPD_radar_zoom()
{
  if (EPD_zoom < ZOOM_HIGH) EPD_zoom++;
}

void EPD_radar_unzoom()
{
  if (EPD_zoom > ZOOM_LOWEST) EPD_zoom--;
}

// WAGA 030525 - return right x characters.  used in navbox4 view nav mode
String rightchr(String const& source, size_t const nbrchr) {
  Serial.print(">"); Serial.print(source);Serial.print("< source len=");Serial.println(source.length());
  if (nbrchr >= source.length()) { return source; }
  Serial.print("- returned >");Serial.print(source.substring(source.length() - nbrchr+1));Serial.print("<");
  return source.substring(source.length() - nbrchr+1);
} 
