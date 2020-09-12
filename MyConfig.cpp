/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MyConfig.cpp
 * Author: micky
 * 
 * Created on February 21, 2020, 12:15 PM
 */

#include "MyConfig.h"
#include "common.h"

#include <glib-object.h>
#include <json-glib/json-glib.h>

MqttConfig  g_mqttConfig;
int         g_nCamCount = 0;
CamDevice   *g_pCamDevs = new CamDevice[MAX_CAM_DEVICE];

MyConfig::MyConfig()
{
    
}

MyConfig::~MyConfig() {
    
}

int MyConfig::load_config(gchar *file)
{
    
    if ( file == NULL || !g_file_test(file, G_FILE_TEST_EXISTS) ) {
        return EXIT_FAILURE;
    }    
    
    JsonParser *parser = json_parser_new();
    JsonNode *root;
    GError *error = NULL;
    
    json_parser_load_from_file(parser, file, &error);
    if ( error ) {
        g_print ("Config: unable to parse `%s': %s\n", file, error->message);
        g_error_free (error);
        g_object_unref (parser);
        return EXIT_FAILURE;
    }
    
    root = json_parser_get_root (parser);
    
 
    if ( json_node_get_node_type(root) != JSON_NODE_OBJECT ) {
        g_print("Config: unexpected element type %s", json_node_type_name(root));
        g_object_unref (parser);
        return EXIT_FAILURE;
    }
   
    JsonObject *root_obj = json_node_get_object(root);
    
    g_print("Loading MQTT settings...\n\n");
    JsonNode *mqtt_node = json_object_get_member(root_obj, "mqtt");
    JsonObject *mqtt_obj = json_node_get_object(mqtt_node);
    
    memmove(g_mqttConfig.server, json_object_get_string_member(mqtt_obj, "mqtt_address"), 128);
    memmove(g_mqttConfig.username, json_object_get_string_member(mqtt_obj, "mqtt_username"), 128);
    memmove(g_mqttConfig.password, json_object_get_string_member(mqtt_obj, "mqtt_password"), 128);
    g_mqttConfig.port = json_object_get_int_member(mqtt_obj, "port");
//    memmove(g_mqttConfig.topic, json_object_get_string_member(mqtt_obj, "mqtt_topic"), 128);
    
    g_print("Config: loading ip camera settings...\n");
    JsonNode *cam_array_node = json_object_get_member(root_obj, "camera");
    JsonArray *cam_array = json_node_get_array(cam_array_node);
    

    g_nCamCount = json_array_get_length(cam_array);
    for ( guint i = 0; i < g_nCamCount; i++ ) {
        JsonNode *cam_node = json_array_get_element(cam_array, i);
        JsonObject *cam_obj = json_node_get_object(cam_node);
        
        //memmove(g_pCamDevs[i].cam_id, g_strdup_printf("cam%02d", i));
        memmove(g_pCamDevs[i].rtsp_url, json_object_get_string_member(cam_obj, "rtsp_url"), 256);
        memmove(g_pCamDevs[i].trigger_on_topic, json_object_get_string_member(cam_obj, "mqtt_trigger_on_topic"), 128);
        memmove(g_pCamDevs[i].trigger_off_topic, json_object_get_string_member(cam_obj, "mqtt_trigger_off_topic"), 128);
        g_pCamDevs[i].record_before_on = json_object_get_int_member(cam_obj, "record_before_on_seconds");
        g_pCamDevs[i].record_after_off = json_object_get_int_member(cam_obj, "record_after_off_seconds");
        g_print("Camera(%d): %s\ton_topic:'%s'\toff_topic:'%s'\n\trecord_before_on_seconds:%d\trecord_after_off_seconds:%d\n", i + 1, 
                g_pCamDevs[i].rtsp_url,
                g_pCamDevs[i].trigger_on_topic,
                g_pCamDevs[i].trigger_off_topic,
                g_pCamDevs[i].record_before_on,
                g_pCamDevs[i].record_after_off);
    }
    
    g_print("Config: total %d camera(s) rtsp loaded...\n\n", g_nCamCount);

    g_object_unref (parser);
    return EXIT_SUCCESS;
}

