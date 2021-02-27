/*
 *  audioreceiver - audio stream receiving utility 
 *  Copyright (C) 2021  Pasi Patama
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * This work is based on gstreamer example applications and references
 * indicated in README.
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <gst/gst.h>

int
main (int argc, char *argv[])
{
	int usetestsource=0;
	int c, index;
	int port=6000;
	opterr = 0;
	
	while ((c = getopt (argc, argv, "hp:")) != -1)
	switch (c)
	{
	case 'p':
		port = atoi(optarg);
		break;
	case 'h':
		fprintf(stderr,"audioreceiver \n");
		fprintf(stderr,"Usage: -p [port] set receiving port\n");
		return 1;
	break;
		default:
		break;
	}
	/* Initialize gstreamer */
	GstElement *pipeline, *udpsource,*rtpopusdepay,*opusdecoder, *pulsesink;
	GstCaps *filtercaps;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	gst_init (&argc, &argv);
	/* caps */
	GstElement *capsfilter = gst_element_factory_make("capsfilter", NULL);
	GstCaps *caps = gst_caps_from_string ("application/x-rtp,media=(string)audio,payload=(int)[ 96, 127 ],clock-rate=(int)48000,encoding-name=(string){OPUS,X-GST-OPUS-DRAFT-SPITTKA-00}");
	g_object_set (capsfilter, "caps", caps, NULL);
	/* udpsource */
	udpsource = gst_element_factory_make ("udpsrc", NULL);
	g_object_set(G_OBJECT(udpsource), "port", port, NULL);
	g_object_set(G_OBJECT(udpsource), "caps", caps , NULL);
	/* unref caps */
	gst_caps_unref(caps);
	/* rtpopusdepay */	
	rtpopusdepay = gst_element_factory_make ("rtpopusdepay", NULL);
	/* Opus decoder */
	opusdecoder = gst_element_factory_make ("opusdec", NULL);
	g_object_set (G_OBJECT ( opusdecoder ), "plc", TRUE, NULL);
	g_object_set (G_OBJECT ( opusdecoder ), "use-inband-fec", TRUE, NULL);  
	/* pulsesink */
	pulsesink = gst_element_factory_make ("pulsesink", NULL);
	if (pulsesink == NULL)
		g_error ("Could not create udpsink");
	g_object_set(G_OBJECT(pulsesink), "sync", FALSE, NULL);
	
	pipeline = gst_pipeline_new ("test-pipeline");

	if (!pipeline ) {
		g_printerr ("Not all elements could be created: pipeline \n");
		return -1;
	}
	if (!udpsource) {
		g_printerr ("Not all elements could be created: udpsource \n");
		return -1;
	}
	if (!pulsesink) {
		g_printerr ("Not all elements could be created: pulsesink \n");
		return -1;
	}

	/* Build the pipeline (capsfilter before rptopusdepay?) */
	gst_bin_add_many (GST_BIN (pipeline),udpsource,rtpopusdepay,opusdecoder,pulsesink, NULL);

	if ( gst_element_link_many (udpsource,rtpopusdepay,opusdecoder,pulsesink,NULL) != TRUE) {
		g_printerr ("Elements could not be linked.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	/* Start playing */
	ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr ("Unable to set the pipeline to the playing state.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	/* Wait until error or EOS */
	bus = gst_element_get_bus (pipeline);
	msg =
	  gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
	  GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	/* Parse message */
	if (msg != NULL) {
	GError *err;
	gchar *debug_info;

	switch (GST_MESSAGE_TYPE (msg)) {
	  case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &err, &debug_info);
		g_printerr ("Error received from element %s: %s\n",
			GST_OBJECT_NAME (msg->src), err->message);
		g_printerr ("Debugging information: %s\n",
			debug_info ? debug_info : "none");
		g_clear_error (&err);
		g_free (debug_info);
		break;
	  case GST_MESSAGE_EOS:
		g_print ("End-Of-Stream reached.\n");
		break;
	  default:
		/* We should not reach here because we only asked for ERRORs and EOS */
		g_printerr ("Unexpected message received.\n");
		break;
	}
	gst_message_unref (msg);
	}

	/* Free resources */
	gst_object_unref (bus);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);

	return 0;
}
