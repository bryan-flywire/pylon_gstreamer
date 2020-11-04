/*  CPipelineHelper.cpp: Definition file for CPipelineHeader Class.
    Given a GStreamer pipeline and source, this will finish building a pipeline.

	Copyright 2017-2019 Matthew Breit <matt.breit@gmail.com>

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	THIS SOFTWARE REQUIRES ADDITIONAL SOFTWARE (IE: LIBRARIES) IN ORDER TO COMPILE
	INTO BINARY FORM AND TO FUNCTION IN BINARY FORM. ANY SUCH ADDITIONAL SOFTWARE
	IS OUTSIDE THE SCOPE OF THIS LICENSE.
*/

#include "CPipelineHelper.h"

#include <stdio.h>
#include <iostream>
#include <ctime>

using namespace std;

// ****************************************************************************
// For Debugging, the functions below can print the caps of elements
// These come from GStreamer Documentation.
// They are defined at the end of this file.
// example usage:
// print_pad_capabilities(convert, "src");
// print_pad_capabilities(encoder, "sink");

static gboolean print_field (GQuark field, const GValue * value, gpointer pfx);
static void print_caps (const GstCaps * caps, const gchar * pfx);
static void print_pad_templates_information (GstElementFactory * factory);
static void print_pad_capabilities (GstElement *element, gchar *pad_name);
GstElement *overlay;
int tz_Offset; // cant be passed as an arg to _on_format_location? (because of macro?)
// ****************************************************************************

CPipelineHelper::CPipelineHelper(GstElement *pipeline, GstElement *source)
{
	m_pipelineBuilt = false;
	m_pipeline = pipeline;
	m_source = source;
}

CPipelineHelper::~CPipelineHelper()
{
}


gchar* _on_format_location(GstElement* splitmux, guint fragment_id)
{
	time_t curr_time = time(0);
	tm* now = gmtime(&curr_time);
	string year = to_string(now->tm_year + 1900).erase(0,2);
	//TODO: Fix sigfig hack
	string month = to_string(now->tm_mon + 1);
	if (month.length() < 2){
		month = "0" + month;
	}
	string day = to_string(now->tm_mday);
	if (day.length() < 2){
		day = "0" + day;
	}
	string hour = to_string((now->tm_hour + tz_Offset)%24);
	if (hour.length() < 2){
		hour = "0" + hour;
	}
	string min = to_string(now->tm_min);
	if (min.length() < 2){
		min = "0" + min;
	}
	string sec = to_string(now->tm_sec);
	if (sec.length() < 2){
		sec = "0" + sec;
	}
	string datetime =  year + "." + month + "." + day + "_" + hour + "." + min + "." + sec + "_";
	//string path = "/media/56C7-FC96/" + datetime + "_%04d.mp4";
	string path = "/home/pi/flywire/tmp/videos/" + datetime + "_%04d.mp4";
    const char* location = path.c_str();
    //gchar* fileName = g_strdup_printf(location, fragment_id + *offset);
	gchar* fileName = g_strdup_printf(location, fragment_id);
	cout << fileName << endl;
    //g_free(location);
    return fileName;
}


bool CPipelineHelper::close_pipeline(){
	gst_element_send_event(m_source, gst_event_new_eos());
	return true;
}

bool CPipelineHelper::update_overlay(const gchar* updatetext){
	try{
		cout << updatetext << endl;
		g_object_set(G_OBJECT(overlay), "text", updatetext, NULL);
	}catch (std::exception &e){
		cerr << "An exception occurred in update_overlay: " << endl << e.what() << endl;
	}
}

// example of how to create a pipeline for display in a window
bool CPipelineHelper::build_pipeline_display()
{
		try
	{
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *queue;
		GstElement *videoflip;
		GstElement *convert;
		GstElement *sink;
		GstElement *filter;
		GstElement *filter2;
		GstCaps *filter_caps;

		cout << "Creating Pipeline for displaying images in local window..." << endl;
		// Create gstreamer elements
		queue = gst_element_factory_make("queue", "queue");
		videoflip = gst_element_factory_make("videoflip", "videoflip");
		convert = gst_element_factory_make("videoconvert", "converter");
		sink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink");
		filter = gst_element_factory_make("capsfilter", "capsfilter");
		filter2 = gst_element_factory_make("capsfilter", "capsfilter");

		if (!videoflip){ cout << "Could not make videoflip" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }
		
		filter = gst_element_factory_make("capsfilter", "filter");
		filter2 = gst_element_factory_make("capsfilter", "filter2");
		filter_caps = gst_caps_new_simple("video/x-raw",
			"format", G_TYPE_STRING, "I420",
		  	"width", G_TYPE_INT, 1920,
        	"height", G_TYPE_INT, 1080,
		  NULL);
		
		g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
		g_object_set(G_OBJECT(filter2), "caps", filter_caps, NULL);
		gst_caps_unref(filter_caps);

		// Set up elements
		g_object_set(G_OBJECT(queue), "leaky", 2, NULL);
		g_object_set(G_OBJECT(queue), "max-size-time", 5000000000, NULL);

		//g_object_set(G_OBJECT(videoflip), "method", "vertical-flip", NULL);
		g_object_set(G_OBJECT(videoflip), "video-direction", 3, NULL);
		
		
		g_object_set(G_OBJECT(sink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(sink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(sink), "set_mode", 0, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), m_source, videoflip, convert, filter2, filter, sink, NULL);
		gst_element_link_many(m_source, convert, videoflip, filter2, sink, NULL);
		
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
		
	}

}

// example of how to create a pipeline for encoding images in h264 format and streaming to local video file
bool CPipelineHelper::build_pipeline_h264file(int timezoneOffset)
{
	try
	{
		tz_Offset = timezoneOffset;
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -h264file. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *queue;
		GstElement *videoflip;
		GstElement *convert;
		GstElement *encode;
		GstElement *parse;
		GstElement *sink;

		cout << "Creating Pipeline for saving frames as h264 video on local host" << endl;

		// Create gstreamer elements
		queue = gst_element_factory_make("queue", "queue");
		videoflip = gst_element_factory_make("videoflip", "videoflip");
		convert = gst_element_factory_make("videoconvert", "converter");

		// depending on your platform, you may have to use some alternative encoder here.
		encode = gst_element_factory_make("omxh264enc", "omxh264enc"); // omxh264enc works good on Raspberry Pi
		parse = gst_element_factory_make("h264parse", "h264parse");
		sink = gst_element_factory_make("splitmuxsink", "splitmuxsink");

		if (!videoflip){ cout << "Could not make videoflip" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!encode){ cout << "Could not make encoder" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }

		// Set up elements
		g_object_set(G_OBJECT(queue), "leaky", 1, NULL);
		g_object_set(G_OBJECT(queue), "max-size-time", 200000000, NULL);

		g_object_set(G_OBJECT(videoflip), "video-direction", 3, NULL);

		g_object_set(G_OBJECT(encode), "control-rate", 2, NULL);
		g_object_set(G_OBJECT(encode), "bitrate", 7853000, NULL);

		g_object_set(G_OBJECT(sink), "location", "/media/56C7-FC96/video%02d.mp4", NULL);
		int offset = 0; //No current use, example for adding future functionality
		g_signal_connect (sink, "format-location", G_CALLBACK(_on_format_location), &offset);
		g_object_set(G_OBJECT(sink), "max-size-time", 300000000000, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), m_source, queue, videoflip, convert, encode, parse, sink, NULL);
		gst_element_link_many(m_source, convert, queue,  videoflip, encode, parse, sink, NULL);
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_h264file(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
	}
}

bool CPipelineHelper::build_pipeline_display_h264file(int timezoneOffset)
{
try
	{
		tz_Offset = timezoneOffset;
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}
		
		GstElement *pipequeue;
		GstElement *videoflip;
		GstElement *convert;
		GstElement *tee;

		GstElement *dispqueue;
		GstElement *overlay;
		GstElement *dispsink;

		GstElement *recqueue;
		GstElement *encode;
		GstElement *parse;
		GstElement *filesink;

		cout << "Creating Pipeline for displaying and encoding frames..." << endl;
		// Create gstreamer elements
		pipequeue = gst_element_factory_make("queue", "queue0");
		videoflip = gst_element_factory_make("videoflip", "videoflip");
		convert = gst_element_factory_make("videoconvert", "converter");
		tee = gst_element_factory_make("tee","tee");

		dispqueue = gst_element_factory_make("queue", "queue1");
		dispsink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink"); // depending on your platform, you may have to use some alternative here, like ("autovideosink", "sink")
		overlay = gst_element_factory_make("textoverlay", "textoverlay");		
		//dispsink = gst_element_factory_make("nveglglessink", "nveglglessink");

		recqueue = gst_element_factory_make("queue", "queu2e");
		encode = gst_element_factory_make("omxh264enc", "omxh264enc");
		parse = gst_element_factory_make("h264parse", "h264parse");
		filesink = gst_element_factory_make("splitmuxsink", "splitmuxsink");

		if (!pipequeue){ cout << "Could not make pipequeue" << endl; return false; }
		if (!videoflip){ cout << "Could not make videoflip" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!tee){ cout << "Could not make tee" << endl; return false; }

		if (!dispqueue){ cout << "Could not make dispqueue" << endl; return false; }
		if (!overlay){ cout << "Could not make overlay" << endl; return false; }
		if (!dispsink){ cout << "Could not make dispsink" << endl; return false; }

		if (!recqueue){ cout << "Could not make recqueue" << endl; return false; }
		if (!encode){ cout << "Could not make encode" << endl; return false; }
		if (!parse){ cout << "Could not make parse" << endl; return false; }
		if (!filesink){ cout << "Could not make filesink" << endl; return false; }
		
		g_object_set(G_OBJECT(pipequeue), "leaky", 1, NULL);
		//g_object_set(G_OBJECT(pipequeue), "max-size-time", 3000000000, NULL);
		g_object_set(G_OBJECT(recqueue), "leaky", 1, NULL);
		//g_object_set(G_OBJECT(recqueue), "max-size-time", 3000000000, NULL);

		g_object_set(G_OBJECT(overlay), "text", "Recording", NULL);
		g_object_set(G_OBJECT(overlay), "color", 4294901760, NULL); // #AARRGGBB -> int
		g_object_set(G_OBJECT(overlay), "draw-outline", 0, NULL);
		g_object_set(G_OBJECT(overlay), "deltax", -500, NULL); 
		g_object_set(G_OBJECT(overlay), "font-desc", "Sans, 15", NULL); 

		g_object_set(G_OBJECT(dispqueue), "leaky", 1, NULL);
		//g_object_set(G_OBJECT(dispqueue), "max-size-time", 2000000, NULL);

		g_object_set(G_OBJECT(videoflip), "video-direction", 3, NULL);

		g_object_set(G_OBJECT(encode), "control-rate", 2, NULL);
		g_object_set(G_OBJECT(encode), "bitrate", 5750000, NULL);
		//g_object_set(G_OBJECT(encode), "bitrate", 5750000, NULL);
		//g_object_set(G_OBJECT(encode), "bitrate", 8219473, NULL);
		//g_object_set(G_OBJECT(encode), "bitrate", 7000000, NULL);
		g_object_set(G_OBJECT(encode), "EnableTwopassCBR", 1, NULL);
		g_object_set(G_OBJECT(encode), "EnableStringentBitrate", 1, NULL);
		g_object_set(G_OBJECT(encode), "vbv-size", 30, NULL);

		g_object_set(G_OBJECT(encode), "profile", 8, NULL);
		g_object_set(G_OBJECT(encode), "preset-level", 3, NULL);

		g_object_set(G_OBJECT(filesink), "location", "/media/56C7-FC96/video%02d.mp4", NULL);
				int offset = 0; //No current use, example for adding future functionality
		g_signal_connect (filesink, "format-location", G_CALLBACK(_on_format_location), &offset);
		g_object_set(G_OBJECT(filesink), "max-size-time", 300000000000, NULL);
		
		g_object_set(G_OBJECT(dispsink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(dispsink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(dispsink), "set_mode", 0, NULL);

		gst_bin_add_many(GST_BIN(m_pipeline), m_source, pipequeue, videoflip, convert, tee, dispqueue, overlay, dispsink, recqueue, encode, parse, filesink, NULL);
		gst_element_link_many(m_source, pipequeue, videoflip, convert, tee, NULL);
		gst_element_link_many(tee, dispqueue, overlay, dispsink, NULL);
		gst_element_link_many(tee, recqueue, encode, parse, filesink, NULL);
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display_h264file(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
	}
}

bool CPipelineHelper::build_pipeline_camfail()
{
	try
	{
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *source;
		GstElement *filter;
		GstCaps *filter_caps;
		GstElement *convert;
		GstElement *errmess;
		GstElement *errinfo;
		GstElement *sink;

		cout << "Creating Pipeline for displaying images in local window..." << endl;
		// Create gstreamer elements
		source = gst_element_factory_make("videotestsrc", "videotestsrc");
		filter = gst_element_factory_make("capsfilter", "capsfilter");
		convert = gst_element_factory_make("videoconvert", "converter");
		errmess = gst_element_factory_make("textoverlay", "textoverlay");
		errinfo = gst_element_factory_make("textoverlay", "textoverlay2");
		sink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink"); // depending on your platform, you may have to use some alternative here, like ("autovideosink", "sink")

		if (!source){ cout << "Could not make videotestsrc" << endl; return false; }
		if (!filter){ cout << "Could not make filter" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!errmess){ cout << "Could not make errmess" << endl; return false; }
		if (!errinfo){ cout << "Could not make errinfo" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }
		
		// Set up elements
		//g_object_set(G_OBJECT(queue), "leaky", 2, NULL);
		//g_object_set(G_OBJECT(queue), "max-size-time", 5000000000, NULL);

		filter = gst_element_factory_make("capsfilter", "filter");
		filter_caps = gst_caps_new_simple("video/x-raw",
		  	"width", G_TYPE_INT, 1920,
        	"height", G_TYPE_INT, 1080,
		  NULL);
		
		g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
		gst_caps_unref(filter_caps);

		g_object_set(G_OBJECT(errmess), "text", "CAMERA FAILURE", NULL);
		g_object_set(G_OBJECT(errmess), "color", 4294901760, NULL); // #AARRGGBB -> int
		g_object_set(G_OBJECT(errmess), "draw-outline", 0, NULL);
		g_object_set(G_OBJECT(errmess), "ypad", 225, NULL); 
		g_object_set(G_OBJECT(errmess), "font-desc", "Sans, 65", NULL); 

		g_object_set(G_OBJECT(sink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(sink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(sink), "set_mode", 0, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), source, filter, convert, errmess, errinfo, sink, NULL);
		gst_element_link_many(source, filter, convert, errmess, errinfo, sink, NULL);
		
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
		
	}
}

bool CPipelineHelper::build_pipeline_powerfail()
{
	try
	{
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *source;
		GstElement *filter;
		GstCaps *filter_caps;
		GstElement *convert;
		GstElement *errmess;
		GstElement *errinfo;
		GstElement *sink;

		cout << "Creating Pipeline for displaying images in local window..." << endl;
		// Create gstreamer elements
		source = gst_element_factory_make("videotestsrc", "videotestsrc");
		filter = gst_element_factory_make("capsfilter", "capsfilter");
		convert = gst_element_factory_make("videoconvert", "converter");
		errmess = gst_element_factory_make("textoverlay", "textoverlay");
		errinfo = gst_element_factory_make("textoverlay", "textoverlay2");
		sink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink"); // depending on your platform, you may have to use some alternative here, like ("autovideosink", "sink")

		if (!source){ cout << "Could not make videotestsrc" << endl; return false; }
		if (!filter){ cout << "Could not make filter" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!errmess){ cout << "Could not make errmess" << endl; return false; }
		if (!errinfo){ cout << "Could not make errinfo" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }
		
		// Set up elements
		//g_object_set(G_OBJECT(queue), "leaky", 2, NULL);
		//g_object_set(G_OBJECT(queue), "max-size-time", 5000000000, NULL);

		filter = gst_element_factory_make("capsfilter", "filter");
		filter_caps = gst_caps_new_simple("video/x-raw",
		  	"width", G_TYPE_INT, 1920,
        	"height", G_TYPE_INT, 1080,
		  NULL);
		
		g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
		gst_caps_unref(filter_caps);

		g_object_set(G_OBJECT(errmess), "text", "LOW POWER", NULL);
		g_object_set(G_OBJECT(errmess), "color", 4294901760, NULL); // #AARRGGBB -> int
		g_object_set(G_OBJECT(errmess), "draw-outline", 0, NULL);
		g_object_set(G_OBJECT(errmess), "ypad", 225, NULL); 
		g_object_set(G_OBJECT(errmess), "font-desc", "Sans, 65", NULL); 

		g_object_set(G_OBJECT(sink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(sink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(sink), "set_mode", 0, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), source, filter, convert, errmess, errinfo, sink, NULL);
		gst_element_link_many(source, filter, convert, errmess, errinfo, sink, NULL);
		
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
		
	}
}

bool CPipelineHelper::build_pipeline_syserr()
{
	try
	{
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *source;
		GstElement *filter;
		GstCaps *filter_caps;
		GstElement *convert;
		GstElement *errmess;
		GstElement *errinfo;
		GstElement *sink;

		cout << "Creating Pipeline for displaying images in local window..." << endl;
		// Create gstreamer elements
		source = gst_element_factory_make("videotestsrc", "videotestsrc");
		filter = gst_element_factory_make("capsfilter", "capsfilter");
		convert = gst_element_factory_make("videoconvert", "converter");
		errmess = gst_element_factory_make("textoverlay", "textoverlay");
		errinfo = gst_element_factory_make("textoverlay", "textoverlay2");
		sink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink"); // depending on your platform, you may have to use some alternative here, like ("autovideosink", "sink")

		if (!source){ cout << "Could not make videotestsrc" << endl; return false; }
		if (!filter){ cout << "Could not make filter" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!errmess){ cout << "Could not make errmess" << endl; return false; }
		if (!errinfo){ cout << "Could not make errinfo" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }
		
		// Set up elements
		//g_object_set(G_OBJECT(queue), "leaky", 2, NULL);
		//g_object_set(G_OBJECT(queue), "max-size-time", 5000000000, NULL);

		filter = gst_element_factory_make("capsfilter", "filter");
		filter_caps = gst_caps_new_simple("video/x-raw",
		  	"width", G_TYPE_INT, 1920,
        	"height", G_TYPE_INT, 1080,
		  NULL);
		
		g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
		gst_caps_unref(filter_caps);

		g_object_set(G_OBJECT(errmess), "text", "RESTART SYSTEM", NULL);
		g_object_set(G_OBJECT(errmess), "color", 4294901760, NULL); // #AARRGGBB -> int
		g_object_set(G_OBJECT(errmess), "draw-outline", 0, NULL);
		g_object_set(G_OBJECT(errmess), "ypad", 225, NULL); 
		g_object_set(G_OBJECT(errmess), "font-desc", "Sans, 65", NULL); 

		g_object_set(G_OBJECT(sink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(sink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(sink), "set_mode", 0, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), source, filter, convert, errmess, errinfo, sink, NULL);
		gst_element_link_many(source, filter, convert, errmess, errinfo, sink, NULL);
		
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
		
	}
}

bool CPipelineHelper::build_pipeline_fullusb()
{
	try
	{
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *source;
		GstElement *filter;
		GstCaps *filter_caps;
		GstElement *convert;
		GstElement *errmess;
		GstElement *errinfo;
		GstElement *sink;

		cout << "Creating Pipeline for displaying images in local window..." << endl;
		// Create gstreamer elements
		source = gst_element_factory_make("videotestsrc", "videotestsrc");
		filter = gst_element_factory_make("capsfilter", "capsfilter");
		convert = gst_element_factory_make("videoconvert", "converter");
		errmess = gst_element_factory_make("textoverlay", "textoverlay");
		errinfo = gst_element_factory_make("textoverlay", "textoverlay2");
		sink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink"); // depending on your platform, you may have to use some alternative here, like ("autovideosink", "sink")

		if (!source){ cout << "Could not make videotestsrc" << endl; return false; }
		if (!filter){ cout << "Could not make filter" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!errmess){ cout << "Could not make errmess" << endl; return false; }
		if (!errinfo){ cout << "Could not make errinfo" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }
		
		// Set up elements
		//g_object_set(G_OBJECT(queue), "leaky", 2, NULL);
		//g_object_set(G_OBJECT(queue), "max-size-time", 5000000000, NULL);

		filter = gst_element_factory_make("capsfilter", "filter");
		filter_caps = gst_caps_new_simple("video/x-raw",
		  	"width", G_TYPE_INT, 1920,
        	"height", G_TYPE_INT, 1080,
		  NULL);
		
		g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
		gst_caps_unref(filter_caps);

		g_object_set(G_OBJECT(errmess), "text", "REPLACE USB DRIVE", NULL);
		g_object_set(G_OBJECT(errmess), "color", 4294901760, NULL); // #AARRGGBB -> int
		g_object_set(G_OBJECT(errmess), "draw-outline", 0, NULL);
		g_object_set(G_OBJECT(errmess), "ypad", 225, NULL); 
		g_object_set(G_OBJECT(errmess), "font-desc", "Sans, 65", NULL); 

		g_object_set(G_OBJECT(sink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(sink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(sink), "set_mode", 0, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), source, filter, convert, errmess, errinfo, sink, NULL);
		gst_element_link_many(source, filter, convert, errmess, errinfo, sink, NULL);
		
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
		
	}
}

bool CPipelineHelper::build_pipeline_temperr()
{
	try
	{
		if (m_pipelineBuilt == true)
		{
			cout << "Cancelling -display. Another pipeline has already been built." << endl;
			return false;
		}

		GstElement *source;
		GstElement *filter;
		GstCaps *filter_caps;
		GstElement *convert;
		GstElement *errmess;
		GstElement *errinfo;
		GstElement *sink;

		cout << "Creating Pipeline for displaying images in local window..." << endl;
		// Create gstreamer elements
		source = gst_element_factory_make("videotestsrc", "videotestsrc");
		filter = gst_element_factory_make("capsfilter", "capsfilter");
		convert = gst_element_factory_make("videoconvert", "converter");
		errmess = gst_element_factory_make("textoverlay", "textoverlay");
		errinfo = gst_element_factory_make("textoverlay", "textoverlay2");
		sink = gst_element_factory_make("nvdrmvideosink", "nvdrmvideosink"); // depending on your platform, you may have to use some alternative here, like ("autovideosink", "sink")

		if (!source){ cout << "Could not make videotestsrc" << endl; return false; }
		if (!filter){ cout << "Could not make filter" << endl; return false; }
		if (!convert){ cout << "Could not make convert" << endl; return false; }
		if (!errmess){ cout << "Could not make errmess" << endl; return false; }
		if (!errinfo){ cout << "Could not make errinfo" << endl; return false; }
		if (!sink){ cout << "Could not make sink" << endl; return false; }
		
		// Set up elements
		//g_object_set(G_OBJECT(queue), "leaky", 2, NULL);
		//g_object_set(G_OBJECT(queue), "max-size-time", 5000000000, NULL);

		filter = gst_element_factory_make("capsfilter", "filter");
		filter_caps = gst_caps_new_simple("video/x-raw",
		  	"width", G_TYPE_INT, 1920,
        	"height", G_TYPE_INT, 1080,
		  NULL);
		
		g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
		gst_caps_unref(filter_caps);

		g_object_set(G_OBJECT(errmess), "text", "SYSTEM OVERHEATED", NULL);
		g_object_set(G_OBJECT(errmess), "color", 4294901760, NULL); // #AARRGGBB -> int
		g_object_set(G_OBJECT(errmess), "draw-outline", 0, NULL);
		g_object_set(G_OBJECT(errmess), "ypad", 225, NULL); 
		g_object_set(G_OBJECT(errmess), "font-desc", "Sans, 65", NULL); 

		g_object_set(G_OBJECT(sink), "conn_id", 0, NULL);
		g_object_set(G_OBJECT(sink), "plane_id", 1, NULL);
		g_object_set(G_OBJECT(sink), "set_mode", 0, NULL);

		// add and link the pipeline elements
		gst_bin_add_many(GST_BIN(m_pipeline), source, filter, convert, errmess, errinfo, sink, NULL);
		gst_element_link_many(source, filter, convert, errmess, errinfo, sink, NULL);
		
		
		cout << "Pipeline Made." << endl;
		
		m_pipelineBuilt = true;

		return true;
	}
	catch (std::exception &e)
	{
		cerr << "An exception occurred in build_pipeline_display(): " << endl << e.what() << endl;
		gst_element_send_event(m_source, gst_event_new_eos());
		return false;
		
	}
}

// ****************************************************************************
// debugging functions

static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
  guint i;

  g_return_if_fail (caps != NULL);

  if (gst_caps_is_any (caps)) {
    g_print ("%sANY\n", pfx);
    return;
  }
  if (gst_caps_is_empty (caps)) {
    g_print ("%sEMPTY\n", pfx);
    return;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);

    g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
    gst_structure_foreach (structure, print_field, (gpointer) pfx);
  }
}

/* Prints information about a Pad Template, including its Capabilities */
static void print_pad_templates_information (GstElementFactory * factory) {
  const GList *pads;
  GstStaticPadTemplate *padtemplate;

  g_print ("Pad Templates for %s:\n", gst_element_factory_get_longname (factory));
  if (!gst_element_factory_get_num_pad_templates (factory)) {
    g_print ("  none\n");
    return;
  }

  pads = gst_element_factory_get_static_pad_templates (factory);
  while (pads) {
    padtemplate = (GstStaticPadTemplate*)pads->data;
    pads = g_list_next (pads);

    if (padtemplate->direction == GST_PAD_SRC)
      g_print ("  SRC template: '%s'\n", padtemplate->name_template);
    else if (padtemplate->direction == GST_PAD_SINK)
      g_print ("  SINK template: '%s'\n", padtemplate->name_template);
    else
      g_print ("  UNKNOWN!!! template: '%s'\n", padtemplate->name_template);

    if (padtemplate->presence == GST_PAD_ALWAYS)
      g_print ("    Availability: Always\n");
    else if (padtemplate->presence == GST_PAD_SOMETIMES)
      g_print ("    Availability: Sometimes\n");
    else if (padtemplate->presence == GST_PAD_REQUEST)
      g_print ("    Availability: On request\n");
    else
      g_print ("    Availability: UNKNOWN!!!\n");

    if (padtemplate->static_caps.string) {
      GstCaps *caps;
      g_print ("    Capabilities:\n");
      caps = gst_static_caps_get (&padtemplate->static_caps);
      print_caps (caps, "      ");
      gst_caps_unref (caps);

    }

    g_print ("\n");
  }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad (element, pad_name);
  if (!pad) {
    g_printerr ("Could not retrieve pad '%s'\n", pad_name);
    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  g_print ("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}
// ****************************************************************************
