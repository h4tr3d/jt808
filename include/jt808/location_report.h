// MIT License
//
// Copyright (c) 2020 Yuming Meng
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// @File    :  location_report.h
// @Version :  1.0
// @Time    :  2020/07/02 09:56:33
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#ifndef JT808_LOCATION_REPORT_H_
#define JT808_LOCATION_REPORT_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

namespace libjt808 {

// Alarm bits
union AlarmBit {
    struct {
        // Emergency alarm triggered after pressing the alarm switch.
        uint32_t sos                       : 1;
        // Overspeed alarm.
        uint32_t overspeed                 : 1;
        // Fatigue driving.
        uint32_t fatigue                   : 1;
        // Early warning.
        uint32_t early_warning             : 1;
        // GNSS module failure.
        uint32_t gnss_fault                : 1;
        // GNSS antenna not connected or cut.
        uint32_t gnss_antenna_cut          : 1;
        // GNSS antenna short circuit.
        uint32_t gnss_antenna_shortcircuit : 1;
        // Terminal main power undervoltage.
        uint32_t power_low                 : 1;
        // Terminal main power outage.
        uint32_t power_cut                 : 1;
        // Terminal LCD or display failure.
        uint32_t lcd_fault                 : 1;
        // TTS module failure.
        uint32_t tts_fault                 : 1;
        // Camera failure.
        uint32_t camera_fault              : 1;
        // OBD fault code.
        uint32_t obd_fault_code            : 1;
        // Reserved 5 bits.
        uint32_t retain1                   : 5;
        // Cumulative driving overtime for the day.
        uint32_t day_drive_overtime        : 1;
        // Overtime parking.
        uint32_t stop_driving_overtime     : 1;
        // Entering/exiting area.
        uint32_t in_out_area               : 1;
        // Entering/exiting route.
        uint32_t in_out_road               : 1;
        // Insufficient/excessive driving time on road section.
        uint32_t road_drive_time           : 1;
        // Route deviation alarm.
        uint32_t road_deviate              : 1;
        // Vehicle VSS failure.
        uint32_t vss_fault                 : 1;
        // Abnormal vehicle oil level.
        uint32_t oil_fault                 : 1;
        // Vehicle theft (through vehicle anti-theft device).
        uint32_t car_alarm                 : 1;
        // Illegal vehicle ignition.
        uint32_t car_acc_alarm             : 1;
        // Illegal vehicle displacement.
        uint32_t car_move                  : 1;
        // Collision/rollover alarm.
        uint32_t collision                 : 1;
        // Reserved 2 bits.
        uint32_t retain2                   : 2;
    } bit;

    uint32_t value;
};

// Status bits
union StatusBit {
    struct {
        // ACC switch, 0: ACC off; 1: ACC on.
        uint32_t acc          : 1;
        // Positioning flag, 0: not positioned; 1: positioned.
        uint32_t positioning  : 1;
        // Latitude hemisphere, 0: north latitude; 1: south latitude.
        uint32_t sn_latitude  : 1;
        // Longitude hemisphere, 0: east longitude; 1: west longitude.
        uint32_t ew_longitude : 1;
        // 0: operating status; 1: out of service status.
        uint32_t operation    : 1;
        // 0: longitude and latitude not encrypted by the security plugin; 1: longitude and latitude encrypted by the
        // security plugin.
        uint32_t gps_encrypt  : 1;
        // Reserved 2 bits.
        uint32_t retain1      : 2;
        // 00: empty; 01: half load; 10: reserved; 11: full load.
        uint32_t trip_status  : 2;
        // 0: vehicle oil circuit normal; 1: vehicle oil circuit disconnected.
        uint32_t oil_cut      : 1;
        // 0: vehicle circuit normal; 1: vehicle circuit disconnected.
        uint32_t circuit_cut  : 1;
        // 0: door unlocked; 1: door locked.
        uint32_t door_lock    : 1;
        // 0: door 1 closed; 1: door 1 open; (front door).
        uint32_t door1_status : 1;
        // 0: door 2 closed; 1: door 2 open; (middle door).
        uint32_t door2_status : 1;
        // 0: door 3 closed; 1: door 3 open; (rear door).
        uint32_t door3_status : 1;
        // 0: door 4 closed; 1: door 4 open; (driver's door).
        uint32_t door4_status : 1;
        // 0: door 5 closed; 1: door 5 open; (custom).
        uint32_t door5_status : 1;
        // 0: GPS satellite not used for positioning; 1: GPS satellite used for positioning.
        uint32_t gps_en       : 1;
        // 0: Beidou satellite not used for positioning; 1: Beidou satellite used for positioning.
        uint32_t beidou_en    : 1;
        // 0: GLONASS satellite not used for positioning; 1: GLONASS satellite used for positioning.
        uint32_t glonass_en   : 1;
        // 0: Galileo satellite not used for positioning; 1: Galileo satellite used for positioning.
        uint32_t galileo_en   : 1;
        // Reserved 10 bits.
        uint32_t retain2      : 10;
    } bit;

    uint32_t value;
};

// Location basic information data.
struct LocationBasicInformation {
    union AlarmBit  alarm;
    union StatusBit status;
    // Latitude (latitude value in degrees multiplied by 10^6, accurate to one millionth of a degree)
    uint32_t latitude;
    // Longitude (longitude value in degrees multiplied by 10^6, accurate to one millionth of a degree)
    uint32_t longitude;
    // Altitude, unit: meter (m)
    uint16_t altitude;
    // Speed 1/10km/h
    uint16_t speed;
    // Bearing 0-359, true north is 0, clockwise
    uint16_t bearing;
    // Time, "YYMMDDhhmmss" (GMT+8 time, all times in this standard use this time zone).
    std::string time;
};

// Extended vehicle signal status bits
union ExtendedVehicleSignalBit {
    struct {
        // Low beam signal
        uint32_t near_lamp       : 1;
        // High beam signal
        uint32_t farl_amp        : 1;
        // Right turn signal
        uint32_t right_turn_lamp : 1;
        // Left turn signal
        uint32_t left_turn_lamp  : 1;
        // Brake signal
        uint32_t breaking        : 1;
        // Reverse signal
        uint32_t reversing       : 1;
        // Fog lamp signal
        uint32_t fog_lamp        : 1;
        // Outline lamp
        uint32_t outline_lamp    : 1;
        // Horn signal
        uint32_t horn            : 1;
        // Air conditioner status
        uint32_t air_conditioner : 1;
        // Neutral signal
        uint32_t neutral         : 1;
        // Retarder working
        uint32_t retarder        : 1;
        // ABS working
        uint32_t abs             : 1;
        // Heater working
        uint32_t heater          : 1;
        // Clutch status
        uint32_t clutch          : 1;
        // Reserved 17 bits.
        uint32_t retain          : 17;
    } bit;

    uint32_t value;
};

// Location information report additional item ID.
enum LocationExtensionId {
    // Mileage, 1/10km, corresponding to the mileage reading on the vehicle, DWORD
    kMileage = 0x01,
    // Oil mass, 1/10L, corresponding to the oil mass reading on the vehicle, WORD
    kOilMass = 0x02,
    // Speed obtained by the tachograph function, 1/10km/h, WORD
    kTachographSpeed = 0x03,
    // ID of the alarm event that requires manual confirmation, starting from 1, WORD
    kAlarmCount = 0x04,
    // Overspeed alarm additional information, BYTE or BYTE+DWORD
    kOverSpeedAlarm = 0x11,
    // Access area/route alarm additional information, BYTE+DWORD+BYTE
    kAccessAreaAlarm = 0x12,
    // Insufficient/excessive driving time alarm additional information, DWORD+WORD+BYTE
    kDrivingTimeAlarm = 0x13,
    // Extended vehicle signal status bits, DWORD
    kVehicleSignalStatus = 0x25,
    // IO status bits, WORD
    kIoStatus = 0x2A,
    // Analog quantity, DWORD
    kAnalogQuantity = 0x2B,
    // Wireless communication network signal strength, BYTE
    kNetworkQuantity = 0x30,
    // Number of GNSS positioning satellites, BYTE
    kGnssSatellites = 0x31,
    // Length of subsequent custom information, BYTE
    kCustomInformationLength = 0xE0,
    // Positioning solution status, BYTE
    kPositioningStatus = 0xEE
};

// Definition of location information additional items storage: key: itemid, value: itemvalue.
using LocationExtensions = std::map<uint8_t, std::vector<uint8_t>>;

// Overspeed alarm additional information location type, BYTE.
enum kOverSpeedAlarmLocationType {
    // No specific location.
    kOverSpeedAlarmNoSpecificLocation = 0x0,
    // Circular area.
    kOverSpeedAlarmCircularArea,
    // Rectangular area.
    kOverSpeedAlarmRectangleArea,
    // Polygonal area.
    kOverSpeedAlarmPolygonArea,
    // Road section.
    kOverSpeedAlarmRoadSection
};

// Access area/route alarm additional information message body location type, BYTE.
enum kAccessAreaAlarmLocationType {
    // Circular area.
    kAccessAreaAlarmCircularArea = 0x0,
    // Rectangular area.
    kAccessAreaAlarmRectangleArea,
    // Polygonal area.
    kAccessAreaAlarmPolygonArea,
    // Route.
    kOverSpeedAlarmRoute
};

// Access area/route alarm additional information message body direction type, BYTE.
enum kAccessAreaAlarmDirectionType {
    // Entering the area.
    kAccessAreaAlarmInArea = 0x0,
    // Leaving the area.
    kAccessAreaAlarmOutArea
};

// IO status bits
union IoStatusBit {
    struct {
        // Deep dormancy state
        uint16_t deep_dormancy : 1;
        // Dormancy state
        uint16_t dormancy      : 1;
        // Reserved 14 bits.
        uint16_t retain        : 14;
    } bit;

    uint16_t value;
};

// Temporary location tracking control information.
struct LocationTrackingControl {
    // Time interval.
    uint16_t interval;
    // Valid time in seconds (s).
    uint32_t tracking_time;
};

// Set overspeed alarm additional information message body.
int SetOverSpeedAlarmBody(uint8_t const& location_type, uint32_t const& area_route_id, std::vector<uint8_t>* out);

// Get overspeed alarm additional information message body.
int GetOverSpeedAlarmBody(std::vector<uint8_t> const& out, uint8_t* location_type, uint32_t* area_route_id);

// Set access area/route alarm additional information message body.
int SetAccessAreaAlarmBody(uint8_t const& location_type, uint32_t const& area_route_id, uint8_t const& direction,
                           std::vector<uint8_t>* out);

// Get access area/route alarm additional information message body.
int GetAccessAreaAlarmBody(std::vector<uint8_t> const& out, uint8_t* location_type, uint32_t* area_route_id,
                           uint8_t* direction);

} // namespace libjt808

#endif // JT808_LOCATION_REPORT_H_
