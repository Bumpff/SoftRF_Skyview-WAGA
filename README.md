# SkyView WAGA Device (softRF)

This project began in Feb 25 as a development of the SoftRF Skyview EZ display. The starting point for this code was Moshe Braner's Fork revision M06b, which was based upon Linar Lyusupov revision 0.12.

The intent of the project is to develop a companion display device for the WAGA Flarm.  WAGA is the Western Australia Gliding Association.  The WAGA Flarm is a not for profit low cost genuine Atom PowerFlarm with ADSB using a bespoke PCB designed and assembled in Perth, Australia.  Approx 35 have been built for WAGA members for use in gliders and tugs.

NOTE:  The SkyView hardware is a Lilygo T5S which is an ESP32 with WaveShare 2.7" e-paper screen.  The e-paper screen is probably the limiting factor for this project.  Firstly , the screen datasheet says it's not suitable for use in strong sunlight - but its cheap to replace.  Then there is an issue regarding speed of screen update.  The code uses 'fast update' for the screen which I have measured as taking approx 850ms. However, the 'next screen' buffer is not available for writing to during a 'fast' update.  So, the worst case is an alarm received just after an update has initiated - the alarm cannot be passed to the screen for 850ms, and then it will take another 850ms to update and become visible.1.9Mb 

The code sketch size is nearing the 1.9MB limit of the 'Minimum SPIFSS' partition scheme.  90% of the 1,966,080 bytes are used by version WAGA01.  There is code not relevant to the Lilygo T5S board that could possibly be deleted.

The code has been primarily tested using PowerFlarm and a traffic simulator (details below). 

The original wifi_UDP and BT_LE taffic data connections appear to work from SoftRF transceivers.  Traffic data input with the USB socket as the connection does not work because it did not work in MB06B and Linar only fixed that in his V0.13.  
I have tested staisfactorily the bridge functionality with Wifi but I have not tested it with BT. SoftRF compatibility is not relevant to the WAGA project, so I wont be fixing any related issues.

<img width="1591" height="787" alt="image" src="https://github.com/user-attachments/assets/96e9679b-19e9-4656-8948-0cf1d281458e" />
<img width="389" height="273" alt="image" src="https://github.com/user-attachments/assets/e7a81ed4-129a-43e6-b59a-dd35df608053" />
<img width="519" height="357" alt="image" src="https://github.com/user-attachments/assets/df65c76a-74c3-4486-9349-69df7f21b67d" />



WAGA01 Additions and changes to MB06B include:
=============================================
1.  Landscape screen layout added to increase installation flexibility.  Use the WebGUI to select landscape or portrait.  (Does not do 180 rotation like Linar v0.13)
2.  NavBoxes in Radar_View amended to:  Threat, Traffic number, Vertical Distance and CompID (looked up from OGN database on SD card).
3.  Nav_View mode added.  Provides basic navigation data to hardcoded Waypoints for use by tugs on aero retrieves. Same radar display
  as RadarView.  NavBoxes are: Threat, Bearing to waypoint, Distance to waypoint and Ground Speed. Use WebGUI to select waypoint.
4.  There is no battery level indicator because this device will be hardwired to power and data.  Battery code remains present and unchanged, so could be easily reactivated.
5.  Additional fields added to NMEA parse: 'Notrack' and 'Source' (available from Flarm protocol 9 onwards). 'Source' distinguishes ADSB and mode-s traffic.
6.  Added concept of 'Threat' where threat is the current highest Alarm Level or closest traffic if no alarms.
7.  NavBox1 shows Threat distance or Alarm Level. In small text it shows source of threat info.
8.  NavBox 4 shows Comp ID of Threat (the Hex code is visbible in small font) derived from an OGN data file on the micro-SD card.  If no Comp ID, will show last 3 digits of ID hex code. 
9.  Alarm 'lookout relative bearing indicator' added.  When a Level 1 Alarm is detected, a black triangle is shown just inside the scale ring showing the pilot where
   to look for the Threat. If a Level 2 Alarm is detected, a triangle is added on the same arc as the black triangle but closer to the radar centre.
  If a Level 3 Alarm is detected, a further triangle is added.  Note: all triangles indicate the direction to look for the traffic, not the the traffic location on
  the radar.  Direction of lookout indicator is always relative to ThisAircraft heading, even when radar in North-Up mode.
10. Revised WebGUI option regarding Voice.  Now 'Sound' options are:  Off, Voice or Buzzer.  Note the SkyView WAGA has a buzzer installed using GPIO Pin 10.
11. Voice is Male voice for advisories "Traffic".  Female fast voice for alarms in ascending order are; "Alert", "Warning", Danger" with X 0'clock and above/below/level.
12. Buzzer on occurs immediately an alarm is detected. Buzzer off and repeat interval is triggered by loop events so as not to add delays to the loop. Buzzer may sound irregular (like Morse code).
13. Threat traffic has a line drawn to it from ThisAircraft.  Tugs have a single ring around the traffice icon. ADSB traffic, if Flarm has capability, has a double ring around the icon.
14. Non-directional traffic, if detected using Source field in $PFLAA sentences, is shown as a ring of dots. Voice states "Aware" instead of X o'clock.
15. The delay between screen updates is set as 2000ms (as per MB06B).  However, Flarm Alarms are processed as priority when received and thus screen display is improved to
    between 850-1700ms after alarm detection.  This delay is a characteristic of the e-paper screen.
16.  On startup:  If setting=Voice, the original voice 'post' jingle is suppressed.  If Setting=Buzzer, 2 buzzes are made.
17.  On startup, if 'No data' is available, message shows what connection and baud rate is set (enumeration values).  If there is 'No Fix', the message shows what type of data is set to be received (eg NMEA).
    This information previously was shown in the NavBoxes.

WAGA02 CHANGES - Not released
==============

WAGA03 CHANGES
==============
1.  Competition ID is shown beside each traffic icon.  If Comp ID not found on SD card, then last 3-digits of ID.  Aircraft with
    zero ground speed (PFLAA sentences) do not show Comp ID.
2.  Revised use of TextBoxes.
3.  Nav_View, intended to assist tug pilots doing paddock retrieves, now has RMI needle that always points to selected Waypoint.
4.  New Sound mode TONE added.  Plays a specific wave sound file for each of Traffic alert, Alarm1, Alarm2 and Alarm3.  Can be
    tones or a voice message.  Tones saved in Alerts directory on SD card. Detail in manual.  Use Audacity to create tones. 
5.  Alarm panel now obscures Radar Panel during alarms.  Has new vertical angle indicator.
6.  Bugs with Advisories Filters fixed.
7.  Updated manual.  


TRAFFIC SIMULATION TOOL
=======================
This is a tool written in MS Access for creating and sending traffic simulations on a PC and sending them to the SkyView by hardwire.  It's proven invaluable during testing.  It's a working tool, not a 'polished' programme!
Simulations using PLAU and PFLAA sentences can be created and saved by the developer as data tables.  Simulations are approximations intended for testing functionality and code changes.
GPS fix data can be inserted.  CheckSums are calculated and concated to the Sentance.
Then directly from MS Access (using USB->RS232_TTL conversion), at the push of a button, the sim is fed at the required input rate in a loop to the Skyview.
The PowerFlarm built in sims are in the database.
Captured NMEA from a Flarm in flight (eg suing XCSoar) can be replayed and/edited.
The tool can also be used to communicate with a PowerFlarm to read the settings and initiate the built-in sims.

<img width="1378" height="695" alt="image" src="https://github.com/user-attachments/assets/8251c0fd-fd4c-4248-b976-ecde56e822a3" />


<img width="1767" height="585" alt="image" src="https://github.com/user-attachments/assets/b9c09e6d-f26c-44f4-8e01-9f22a38ab2eb" />

Sim tool update 10 May 25. 
--------------------------
User interface allows setting of port number and baud rate. 
Custom Sims panel has text box to enter ThisAircraft track for sims where Fix is added. This saves editing the GPS sentence.

There are Utube videos showing some of the features here:

https://youtu.be/_5mlzl7RmtM

Sim tool update 10 May 26.
-------------------------
The tool can be used to read and change Flarm settings.  It works for CLassic Flarm (fewer settings available) and PowerFlarm.
Tool can read PowerFlam CARP Rx range data and display on built in graph.  CARP (Cotinuous Analysis of Radio Performance is explained in Flarm Technical Document FTD-065.

<img width="827" height="466" alt="image" src="https://github.com/user-attachments/assets/8f23d342-0706-4f20-ae11-3d90c73743bb" />


For discussions join the SoftRF Community.

For additional info see also Lyusupov's repository   https://github.com/lyusupov/SoftRF/tree/master/software/firmware/source

and 

Moshes repository:   https://github.com/moshe-braner/SoftRF


