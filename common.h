/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   common.h
 * Author: micky
 *
 * Created on February 21, 2020, 11:17 AM
 */

#ifndef COMMON_H
#define COMMON_H

#include <gst/gst.h>

#include "MyConfig.h"

#define MAX_CAM_DEVICE      8
#define MAX_BUFFER_SECS     30

extern  int             g_nCamCount;
extern  MyConfig        g_myConfig;

typedef struct _MqttConfig {
    gchar       server[128];
    gchar       username[128];
    gchar       password[128];
    gint        port;
//    gchar       topic[128];
}MqttConfig, *pMqttConfig;

typedef struct _CamDevice {
    gchar       cam_id[16];
    gchar       rtsp_url[256];
    gchar       trigger_on_topic[128];
    gchar       trigger_off_topic[128];
    guint       record_before_on;
    guint       record_after_off;
}CamDevice, pCamDevices;


extern MqttConfig   g_mqttConfig;
extern CamDevice    *g_pCamDevs;

extern GMainLoop    *g_main_loop;


#endif /* COMMON_H */

