/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   myGst.h
 * Author: micky
 *
 * Created on February 26, 2020, 8:39 AM
 */

#ifndef MYGST_H
#define MYGST_H

#include "common.h"

enum REC_STATE {
    REC_STATE_STOPPED,
    REC_STATE_RECORDING,
    REC_STATE_STOPPING
};
  
typedef struct
{
    gchar           *id;
    GstElement      *pipeline, *vrecq;
    GstElement      *filesink;
    GstElement      *muxer;
    GstPad          *vrecq_src;
    gulong          vrecq_src_probe_id;
    guint           buffer_count;
    guint           chunk_count;
    guint           rec_state;
    guint           before_sec;
    guint           after_sec;
} RecordApp;

extern RecordApp    g_cam_rtsp;
extern RecordApp    *g_cam_rec;

static gchar        *g_strOutDir = NULL;

static gboolean     start_recording_cb(gpointer user_data);

static void         app_update_filesink_location(RecordApp *app)
{
    gchar *fn;

    if ( app->rec_state == REC_STATE_STOPPED || app->rec_state == REC_STATE_STOPPING ) {
        fn = g_strdup_printf ("%s/%s_temp.mp4", g_strOutDir, app->id);
    } else if ( app->rec_state == REC_STATE_RECORDING ) {
        app->chunk_count++;
        gchar *timestamp = g_date_time_format(g_date_time_new_now_local (), "%Y%m%d%H%M%S");
        fn = g_strdup_printf ("%s/%s_%s.mp4", g_strOutDir, app->id, timestamp);
        g_print ("GST: setting file location to '%s'\n", fn);
    }
    
    
    gst_element_set_state (app->filesink, GST_STATE_NULL);
    gst_element_set_state (app->muxer, GST_STATE_NULL);
    g_object_set (app->filesink, "location", fn, NULL);
    gst_element_set_state (app->filesink, GST_STATE_PLAYING);
    gst_element_set_state (app->muxer, GST_STATE_PLAYING);
    
    g_free (fn);
}

static gboolean     bus_cb(GstBus *bus, GstMessage *msg, gpointer user_data);

static GstPadProbeReturn probe_drop_one_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    RecordApp *app = (RecordApp*)user_data;
    GstBuffer *buf = (GstBuffer*)info->data;
    
    if (app->buffer_count++ == 0) {
//        g_print ("Drop one buffer with ts %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (GST_BUFFER_PTS (info->data)));
        return GST_PAD_PROBE_DROP;
    } else {
        gboolean is_keyframe;

        is_keyframe = !GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT);
//        g_print ("Buffer with ts %" GST_TIME_FORMAT " (keyframe=%d)\n", GST_TIME_ARGS (GST_BUFFER_PTS (buf)), is_keyframe);

        if (is_keyframe) {
//            g_print ("Letting buffer through and removing drop probe\n");
            return GST_PAD_PROBE_REMOVE;
        } else {
//            g_print ("Dropping buffer, wait for a keyframe.\n");
            return GST_PAD_PROBE_DROP;
        }
    }
}

static GstPadProbeReturn block_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    //g_print ("\nGST: Queue output pad blocked!\n");
    g_assert ((info->type & GST_PAD_PROBE_TYPE_BUFFER) == GST_PAD_PROBE_TYPE_BUFFER);
    /* FIXME: this doesn't work: gst_ buffer_replace ((GstBuffer **) &info->data, NULL); */
    return GST_PAD_PROBE_OK;
}

static gpointer push_eos_thread(gpointer user_data)
{
    RecordApp *app = (RecordApp*)user_data;
    GstPad *peer;

    peer = gst_pad_get_peer (app->vrecq_src);
//    g_print ("GST: pushing EOS event on pad %s:%s\n", GST_DEBUG_PAD_NAME (peer));

    /* tell pipeline to forward EOS message from filesink immediately and not
     * hold it back until it also got an EOS message from the video sink */
    g_object_set (g_cam_rtsp.pipeline, "message-forward", TRUE, NULL);

    gst_pad_send_event (peer, gst_event_new_eos ());
    gst_object_unref (peer);

    return NULL;
}

static gboolean stop_recording_cb(gpointer user_data)
{
    RecordApp *app = (RecordApp*)user_data;

    g_print ("GST: stop recording. cam_id: %s\n", app->id);

    app->vrecq_src_probe_id = gst_pad_add_probe (app->vrecq_src,
        (GstPadProbeType)(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER), block_probe_cb,
        NULL, NULL);
    
    app->rec_state = REC_STATE_STOPPING;
    
//    g_object_set (app->vrecq, "max-size-time", (guint64) MAX_BUFFER_SECS * GST_SECOND,
//      "max-size-bytes", 0, "max-size-buffers", 0, "leaky", 2, NULL);

    g_thread_new ("eos-push-thread", push_eos_thread, app);

    return FALSE;                 /* don't call us again */
}

static gboolean bus_cb(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    RecordApp *app = (RecordApp*)user_data;

    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
            g_printerr ("Exit!\n");
            g_main_loop_quit (g_main_loop);
            return FALSE;
        case GST_MESSAGE_ELEMENT:{
        const GstStructure *s = gst_message_get_structure (msg);

            if (gst_structure_has_name (s, "GstBinForwarded")) {
                
                for ( int i = 0; i < g_nCamCount; i++ ) {
                    if ( app[i].rec_state == REC_STATE_STOPPING ) {
                        GstMessage *forward_msg = NULL;

                        gst_structure_get (s, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);
                        if (GST_MESSAGE_TYPE (forward_msg) == GST_MESSAGE_EOS) {
                            //g_print ("GST: EOS from element %s\n",
                            //    GST_OBJECT_NAME (GST_MESSAGE_SRC (forward_msg)));
                            //gst_element_set_state (app[i].filesink, GST_STATE_NULL);
                            //gst_element_set_state (app[i].muxer, GST_STATE_NULL);
                            app_update_filesink_location (&app[i]);
                            //gst_element_set_state (app[i].filesink, GST_STATE_PLAYING);
                            //gst_element_set_state (app[i].muxer, GST_STATE_PLAYING);
                            app[i].rec_state = REC_STATE_STOPPED;
                        }
                        gst_message_unref (forward_msg);
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    return TRUE;
}

static gboolean start_recording_cb(gpointer user_data)
{
    RecordApp *app = (RecordApp*)user_data;

    g_print ("GST: start recording. cam_id: %s\n", app->id);
    
//    g_object_set (app->vrecq, "max-size-time", (guint64) app->before_sec * GST_SECOND,
//      "max-size-bytes", 0, "max-size-buffers", 0, "leaky", 2, NULL);
    
    app->rec_state = REC_STATE_RECORDING;
    app_update_filesink_location(app);

    /* need to hook up another probe to drop the initial old buffer stuck
     * in the blocking pad probe */
    app->buffer_count = 0;
    gst_pad_add_probe (app->vrecq_src,
        GST_PAD_PROBE_TYPE_BUFFER, probe_drop_one_cb, app, NULL);

    /* now remove the blocking probe to unblock the pad */
    gst_pad_remove_probe (app->vrecq_src, app->vrecq_src_probe_id);
    app->vrecq_src_probe_id = 0;
    

    return FALSE;                 /* don't call us again */
}




#endif /* MYGST_H */

