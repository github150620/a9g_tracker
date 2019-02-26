// Copyright © 2019 - WU PENG. All Rights Reserved.

#include "gps_filter.h"

const float PI = 3.141592653;
const float R = 6371; // 6371km
const float MAX_SPEED = 240; // 240km/h

bool GPS_IsInChina(float latitude, float longitude) {
    if (latitude>3.86 && latitude<53.55 && longitude>73.66 && longitude<135.05) {
        return true;
    } else {
        return false;
    }
}

bool GPS_IsPossible(int timestamp, float latitude, float longitude) {
    bool ok = false;
    static int lastTs = 0;
    static float lastLat = 0.0;
    static float lastLng = 0.0;

    float maxDegrees = 360*(MAX_SPEED/3600)/(2*PI*R); // The degrees per 1 second at max speed.

    float dt = timestamp - lastTs;
    if (dt == 0) {
        maxDegrees = 2*maxDegrees;
    } else {
        maxDegrees = (1.0+1.0/dt)*maxDegrees;
    }

    if (latitude-lastLat<maxDegrees && latitude-lastLat>-1.0*maxDegrees && longitude-lastLng<maxDegrees && longitude-lastLng>-1.0*maxDegrees) {
        ok = true;
    } else {
        ok = false;
    }
    lastTs = timestamp;
    lastLat = latitude;
    lastLng = longitude;
    return ok;
}