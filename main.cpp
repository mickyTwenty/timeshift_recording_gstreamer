/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: micky
 *
 * Created on February 20, 2020, 5:40 AM
 */


#include <iostream>
#include <string.h>
#include <unistd.h>

#include <gst/gst.h>

#include "common.h"
#include "myMosq.h"
#include "myGst.h"

using namespace std;

GMainLoop   *g_main_loop = NULL;
MyConfig    *g_pConfig = NULL;
RecordApp   g_cam_rtsp;
RecordApp   *g_cam_rec = NULL;

/*
 * 
 */
int main(int argc, char** argv) {
    
    gchar *config_file = NULL;
    
    if ( argc > 1 )
        config_file = argv[1];
    else {
        g_print("Specify config file path\n");
        return -1;
    }
    
    if ( argc == 3 ) {
        g_strOutDir = argv[2];
    } else {
        g_print("Specify mp4 output directory\n");
        return -1;
    }
    
    g_pConfig = new MyConfig();
    
    if ( g_pConfig->load_config(config_file) ) {
        g_print("loading config file error!\n");
        return -1;
    }
    
    GError *err;
    if ( !gst_init_check(&argc, &argv, &err) ) {
        g_print("gst_init_check() error msg: %s;\n", err->message);
        return -1;
    }
        
    g_cam_rec = new RecordApp[g_nCamCount];
        
    // Pipeline for test(videotestsrc)
//    g_cam_rtsp.pipeline =
//        gst_parse_launch ("videotestsrc ! video/x-raw, width=352, height=288, format=I420 " // VIDEO_CAPS        //"video/x-raw,width=1920,height=1080,format=I420"
//            " ! clockoverlay ! x264enc tune=zerolatency bitrate=8000 ! tee name=vtee "
//            "vtee. ! avdec_h264 ! videoconvert ! videoscale ! autovideosink "
//      "vtee. ! queue name=vrecq1 ! avdec_h264 ! videoconvert ! videoscale ! autovideosink "
//            "vtee. ! queue name=vrecq0 ! mp4mux name=mux0 ! filesink async=false name=filesink0 "
//            "videotestsrc horizontal-speed=1  ! video/x-raw, width=352, height=288, format=I420 "
//            " ! clockoverlay ! x264enc tune=zerolatency bitrate=8000 ! tee name=vtee1 "
//            "vtee1. ! avdec_h264 ! videoconvert ! videoscale ! autovideosink "
//            "vtee1. ! queue name=vrecq1 ! mp4mux name=mux1 ! filesink async=false name=filesink1 "
//           , NULL);

    gchar *pipeline_parser = g_strdup_printf(" ");
    for ( int i = 0; i < g_nCamCount; i++ ) { 
        gchar *bin = g_strdup_printf(" rtspsrc location=%s ! decodebin ! x264enc tune=zerolatency bitrate=8000 ! queue name=vrecq%d ! mp4mux name=mux%d ! filesink async=false name=filesink%d ", g_pCamDevs[i].rtsp_url, i, i, i);
        pipeline_parser = g_strconcat(pipeline_parser, bin, NULL);
    }
    
    g_cam_rtsp.pipeline = gst_parse_launch(pipeline_parser, NULL);
    
    for ( int i = 0; i < g_nCamCount; i++ ) {
        g_cam_rec[i].id = g_strdup_printf("cam%02d", i);
        g_cam_rec[i].vrecq = gst_bin_get_by_name (GST_BIN (g_cam_rtsp.pipeline), g_strdup_printf("vrecq%d", i));
        g_object_set (g_cam_rec[i].vrecq, "max-size-time", (guint64) (g_pCamDevs[i].record_before_on + 5) * GST_SECOND,
            "max-size-bytes", 0, "max-size-buffers", 0, "leaky", 2, NULL);

        g_cam_rec[i].vrecq_src = gst_element_get_static_pad (g_cam_rec[i].vrecq, "src");
        g_cam_rec[i].vrecq_src_probe_id = gst_pad_add_probe (g_cam_rec[i].vrecq_src,
        (GstPadProbeType)(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER), block_probe_cb,
        NULL, NULL);

        g_cam_rec[i].chunk_count = 0;
        g_cam_rec[i].rec_state = REC_STATE_STOPPED;
        g_cam_rec[i].filesink = gst_bin_get_by_name (GST_BIN (g_cam_rtsp.pipeline), g_strdup_printf("filesink%d", i));
        g_cam_rec[i].muxer = gst_bin_get_by_name (GST_BIN (g_cam_rtsp.pipeline), g_strdup_printf("mux%d", i));
        app_update_filesink_location (&g_cam_rec[i]);
    }

    
    gst_element_set_state (g_cam_rtsp.pipeline, GST_STATE_PLAYING);
    
    g_main_loop = g_main_loop_new (NULL, FALSE);
    gst_bus_add_watch (GST_ELEMENT_BUS (g_cam_rtsp.pipeline), bus_cb, g_cam_rec);

//    g_timeout_add_seconds (20, start_recording_cb, &g_cam_rtsp);

    g_main_loop_run (g_main_loop);

    gst_element_set_state (g_cam_rtsp.pipeline, GST_STATE_NULL);
    gst_object_unref (g_cam_rtsp.pipeline);
//    g_free(pipeline_parser);
    return 0;

}

