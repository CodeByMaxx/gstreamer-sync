#pragma once

#include <gst/gst.h>
#include <gst/gstdebugutils.h>
#include <gst/app/gstappsrc.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Overlay.hpp"

class VideoSource {

public:

  VideoSource() = delete;


  VideoSource(
      const std::vector<std::pair<std::string, std::string>>& pipelineStructure,
      Overlay& overlay)
  {
    pipeline = gst_pipeline_new("pipeline");

    if (!pipeline) {
      throw std::runtime_error(
          "Could not create pipeline");
    }


    for (const auto& p : pipelineStructure)
    {
      GstElement* element =
          gst_element_factory_make(
              p.first.c_str(),
              p.second.c_str());


      if (!element) {
        throw std::runtime_error(
            "Cannot create element: " + p.first);
      }


      if (p.first == "appsrc") {
        appsrc = element;
      }


      gstElements.push_back(element);
    }



    if (gstElements.size() < 2) {
      throw std::runtime_error(
          "Pipeline needs source and sink");
    }



    if (!appsrc) {
      throw std::runtime_error(
          "Pipeline requires appsrc");
    }



    setupAppSrc();



    //
    // insert overlay before sink
    //
    gstElements.insert(gstElements.end() - 2, overlay.getElement());

    std::cout << "Pipeline elements:" << std::endl;

    for (auto* e : gstElements)
    {
      std::cout
          << "  "
          << GST_ELEMENT_NAME(e)
          << std::endl;
    }



    for (auto* e : gstElements)
    {
      gst_bin_add(
          GST_BIN(pipeline),
          e);
    }



    for (size_t i = 0;
         i + 1 < gstElements.size();
         ++i)
    {
      gboolean ok =
          gst_element_link(
              gstElements[i],
              gstElements[i + 1]);


      std::cout
          << GST_ELEMENT_NAME(gstElements[i])
          << " -> "
          << GST_ELEMENT_NAME(gstElements[i + 1])
          << " : "
          << (ok ? "OK" : "FAILED")
          << std::endl;


      if (!ok) {
        throw std::runtime_error(
            "Link failed");
      }
    }



    GstPad* overlaySinkPad =
        gst_element_get_static_pad(
            overlay.getElement(),
            "sink");



    if (overlaySinkPad)
    {
      gst_pad_add_probe(
          overlaySinkPad,
          GST_PAD_PROBE_TYPE_BUFFER,
          frameProbe,
          nullptr,
          nullptr);


      gst_object_unref(
          overlaySinkPad);
    }




    bus =
        gst_element_get_bus(
            pipeline);



    gst_bus_add_watch(
        bus,
        busCallback,
        nullptr);




    GstStateChangeReturn ret =
        gst_element_set_state(
            pipeline,
            GST_STATE_PLAYING);



    std::cout
        << "State change: ";



    switch(ret)
    {
      case GST_STATE_CHANGE_SUCCESS:
        std::cout << "SUCCESS";
        break;

      case GST_STATE_CHANGE_ASYNC:
        std::cout << "ASYNC";
        break;

      case GST_STATE_CHANGE_FAILURE:
        std::cout << "FAILURE";
        break;

      default:
        std::cout << "OTHER";
    }


    std::cout << std::endl;



    GstState state;
    GstState pending;



    gst_element_get_state(
        pipeline,
        &state,
        &pending,
        GST_SECOND);



    std::cout
        << "Current state: "
        << gst_element_state_get_name(state)
        << " pending: "
        << gst_element_state_get_name(pending)
        << std::endl;



    std::cout
        << "GStreamer started"
        << std::endl;
  }



  ~VideoSource()
  {
    if (pipeline)
    {
      gst_element_set_state(
          pipeline,
          GST_STATE_NULL);


      gst_object_unref(
          pipeline);
    }


    if (bus)
    {
      gst_object_unref(
          bus);
    }


    std::cout
        << "GStreamer stopped"
        << std::endl;
  }




  void pushFrame(
      const std::vector<uint8_t>& data,
      GstClockTime pts)
  {

    if (!appsrc)
    {
      throw std::runtime_error(
          "No appsrc available");
    }



    std::cout
        << "Frame size: "
        << data.size()
        << std::endl;



    GstBuffer* buffer =
        gst_buffer_new_allocate(
            nullptr,
            data.size(),
            nullptr);



    if (!buffer)
    {
      throw std::runtime_error(
          "Could not allocate buffer");
    }



    GstMapInfo map;



    if (gst_buffer_map(
            buffer,
            &map,
            GST_MAP_WRITE))
    {

      memcpy(
          map.data,
          data.data(),
          data.size());


      gst_buffer_unmap(
          buffer,
          &map);
    }




    GST_BUFFER_PTS(buffer) =
        pts;


    GST_BUFFER_DTS(buffer) =
        pts;



    GST_BUFFER_DURATION(buffer) =
        GST_SECOND / 30;




    GstFlowReturn ret =
        gst_app_src_push_buffer(
            GST_APP_SRC(appsrc),
            buffer);



    if (ret != GST_FLOW_OK)
    {
      std::cerr
          << "push buffer failed: "
          << ret
          << std::endl;
    }
  }



private:


  void setupAppSrc()
  {

    GstCaps* caps =
        gst_caps_new_simple(
            "video/x-raw",
            "format",
            G_TYPE_STRING,
            "BGR",
            "width",
            G_TYPE_INT,
            640,
            "height",
            G_TYPE_INT,
            480,
            "framerate",
            GST_TYPE_FRACTION,
            30,
            1,
            "pixel-aspect-ratio",
            GST_TYPE_FRACTION,
            1,
            1,
            NULL);



    g_object_set(
        appsrc,
        "caps",
        caps,
        "format",
        GST_FORMAT_TIME,
        "is-live",
        TRUE,
        "block",
        TRUE,
        "stream-type",
        GST_APP_STREAM_TYPE_STREAM,
        NULL);



    gst_caps_unref(
        caps);
  }




  static GstPadProbeReturn frameProbe(
      GstPad*,
      GstPadProbeInfo* info,
      gpointer)
  {

    GstBuffer* buffer =
        GST_PAD_PROBE_INFO_BUFFER(info);



    if (!buffer)
      return GST_PAD_PROBE_OK;



    GstClockTime pts =
        GST_BUFFER_PTS(buffer);



    std::cout
        << "Frame PTS: "
        << GST_TIME_AS_MSECONDS(pts)
        << " ms"
        << std::endl;



    return GST_PAD_PROBE_OK;
  }





  static gboolean busCallback(
      GstBus*,
      GstMessage* message,
      gpointer)
  {

    switch(GST_MESSAGE_TYPE(message))
    {

      case GST_MESSAGE_ERROR:
      {
        GError* error = nullptr;
        gchar* debug = nullptr;


        gst_message_parse_error(
            message,
            &error,
            &debug);



        std::cerr
            << "GStreamer ERROR: "
            << error->message
            << std::endl;



        if (debug)
        {
          std::cerr
              << "Debug: "
              << debug
              << std::endl;
        }



        g_error_free(error);
        g_free(debug);

        break;
      }



      case GST_MESSAGE_EOS:

        std::cout
            << "EOS"
            << std::endl;

        break;



      default:
        break;
    }


    return TRUE;
  }




private:

  GstElement* pipeline = nullptr;

  GstBus* bus = nullptr;

  GstElement* appsrc = nullptr;

  std::vector<GstElement*> gstElements;
};
