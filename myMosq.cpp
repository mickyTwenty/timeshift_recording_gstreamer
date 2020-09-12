/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <mosquittopp.h>

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "myMosq.h"
#include "myGst.h"

using namespace std;

myMosq::myMosq() : mosquittopp("unique")
{
    mosqpp::lib_init();        // Mandatory initialization for mosquitto library
    this->keepalive = 60;    // Basic configuration setup for myMosq class
    memmove(this->id, "unique", 128);
    this->port = g_mqttConfig.port;
    
    username_pw_set(g_mqttConfig.username, g_mqttConfig.password);
    connect_async(g_mqttConfig.server,     // non blocking connection to broker request
    port,
    keepalive);
    
    subscribe(NULL, "#", 2);
    
    loop_start();            // Start thread managing connection / publish / subscribe
};


myMosq::~myMosq() {
    loop_stop();            // Kill the thread
    mosqpp::lib_cleanup();    // Mosquitto library cleanup
 }

void myMosq::on_disconnect(int rc) {
    g_print("MQTT: disconnection(%d)\n\n", rc);
}

void myMosq::on_connect(int rc)
 {
    if ( rc == 0 ) {
        g_print("MQTT: connected with server\n\n");
    } else {
        g_print("MQTT: impossible to connect with server(%d)\n\n", rc);
    }
 }

void myMosq::on_message(const mosquitto_message* message)
{
    gchar mqtt_topic[256];
    gchar mqtt_msg[1024];
    
    memmove(mqtt_topic, message->topic, 256);
    memmove(mqtt_msg, message->payload, 1024);
    g_print("MQTT: received message from: topic='%s' msg: '%s'\n", mqtt_topic, mqtt_msg);
    
    if ( g_ascii_strcasecmp(mqtt_msg, "on") == 0 ) {
        for ( int i = 0; i < g_nCamCount; i++ ) {
            if ( g_ascii_strcasecmp(mqtt_topic, g_pCamDevs[i].trigger_on_topic) == 0 ) {
                if ( g_cam_rec[i].rec_state == REC_STATE_STOPPED ) {
                    g_thread_new("start_recording", (GThreadFunc)start_recording_cb, &g_cam_rec[i]);
                }
            }
        }
    } else if ( g_ascii_strcasecmp(mqtt_msg, "off") == 0 ) {
        for ( int i = 0; i < g_nCamCount; i++ ) {
            if ( g_ascii_strcasecmp(mqtt_topic, g_pCamDevs[i].trigger_off_topic) == 0 ) {
                if ( g_cam_rec[i].rec_state == REC_STATE_RECORDING ) {
//                    g_thread_new("start_recording", (GThreadFunc)start_recording_cb, &g_cam_rec[i]);
                    g_timeout_add_seconds(g_pCamDevs[i].record_after_off, stop_recording_cb, &g_cam_rec[i]);
                    
                }
            }
        }
    }
    
//    g_thread_new("start_recording", (GThreadFunc)start_recording_cb, &g_cam_rtsp);
}