/*  CPipelineHelper.h: header file for CPipelineHeader Class.
    Given a GStreamer pipeline and source, this will finish building a pipeline.

	Copyright 2017 Matthew Breit <matt.breit@gmail.com>

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

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <string>

using namespace std;

// Given a pipeline and source, this class will finish building pipelines of various elements for various purposes.

class CPipelineHelper
{
public:
	CPipelineHelper(GstElement *pipeline, GstElement *source);
	~CPipelineHelper();
	
	bool close_pipeline();
	bool update_overlay(const gchar* updatetext);
	// example of how to create a pipeline for display in a window
	bool build_pipeline_display();
	bool build_pipeline_h264file(const int tzOffset);
	bool build_pipeline_display_h264file(const int tzOffset);
	bool build_pipeline_camfail();
	bool build_pipeline_syserr();
	bool build_pipeline_powerfail();
	bool build_pipeline_fullusb();
	bool build_pipeline_temperr();
	
private:
	bool m_pipelineBuilt;
	GstElement *m_pipeline;
	GstElement *m_source;
};
