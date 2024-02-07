#include <iostream>
#include <cstring>
#include <gtk/gtk.h>
#include <gst/gst.h>

class CustomData {
public:
  GstElement *playbin;           /* Our one and only pipeline */
  GtkWidget *sink_widget;         /* The widget where our video will be displayed */
  GtkWidget *slider;              /* Slider widget to keep track of the current position */
  GtkWidget *streams_list;        /* Text widget to display info about the streams */
  gulong slider_update_signal_id; /* Signal ID for the slider update signal */
  GstState state;                 /* Current state of the pipeline */
  gint64 duration;                /* Duration of the clip, in nanoseconds */

  CustomData() : playbin(nullptr), sink_widget(nullptr), slider(nullptr), streams_list(nullptr),
                 slider_update_signal_id(0), state(GST_STATE_NULL), duration(GST_CLOCK_TIME_NONE) {}
};

static void play_cb(GtkButton *button, CustomData *data) {
  gst_element_set_state(data->playbin, GST_STATE_PLAYING);
}

static void pause_cb(GtkButton *button, CustomData *data) {
  gst_element_set_state(data->playbin, GST_STATE_PAUSED);
}

static void stop_cb(GtkButton *button, CustomData *data) {
  gst_element_set_state(data->playbin, GST_STATE_READY);
}

static void delete_event_cb(GtkWidget *widget, GdkEvent *event, CustomData *data) {
  stop_cb(nullptr, data);
  gtk_main_quit();
}

static void slider_cb(GtkRange *range, CustomData *data) {
  gdouble value = gtk_range_get_value(GTK_RANGE(data->slider));
  gst_element_seek_simple(data->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                          static_cast<gint64>(value * GST_SECOND));
}

static void create_ui(CustomData *data) {
  GtkWidget *main_window;
  GtkWidget *main_box;
  GtkWidget *main_hbox;
  GtkWidget *controls;
  GtkWidget *play_button, *pause_button, *stop_button;

  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(main_window), "delete-event", G_CALLBACK(delete_event_cb), data);

  play_button = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_signal_connect(G_OBJECT(play_button), "clicked", G_CALLBACK(play_cb), data);

  pause_button = gtk_button_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_signal_connect(G_OBJECT(pause_button), "clicked", G_CALLBACK(pause_cb), data);

  stop_button = gtk_button_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_signal_connect(G_OBJECT(stop_button), "clicked", G_CALLBACK(stop_cb), data);

  data->slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_scale_set_draw_value(GTK_SCALE(data->slider), 0);
  data->slider_update_signal_id = g_signal_connect(G_OBJECT(data->slider), "value-changed", G_CALLBACK(slider_cb), data);

  data->streams_list = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(data->streams_list), FALSE);

  controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(controls), play_button, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(controls), pause_button, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(controls), stop_button, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(controls), data->slider, TRUE, TRUE, 2);

  main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(main_hbox), data->sink_widget, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(main_hbox), data->streams_list, FALSE, FALSE, 2);

  main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start(GTK_BOX(main_box), main_hbox, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(main_box), controls, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(main_window), main_box);
  gtk_window_set_default_size(GTK_WINDOW(main_window), 640, 480);

  gtk_widget_show_all(main_window);
}

static gboolean refresh_ui(CustomData *data) {
  gint64 current = -1;

  if (data->state < GST_STATE_PAUSED)
    return TRUE;

  if (!GST_CLOCK_TIME_IS_VALID(data->duration)) {
    if (!gst_element_query_duration(data->playbin, GST_FORMAT_TIME, &data->duration)) {
      std::cerr << "Could not query current duration." << std::endl;
    } else {
      gtk_range_set_range(GTK_RANGE(data->slider), 0, static_cast<gdouble>(data->duration) / GST_SECOND);
    }
  }

  if (gst_element_query_position(data->playbin, GST_FORMAT_TIME, &current)) {
    g_signal_handler_block(data->slider, data->slider_update_signal_id);
    gtk_range_set_value(GTK_RANGE(data->slider), static_cast<gdouble>(current) / GST_SECOND);
    g_signal_handler_unblock(data->slider, data->slider_update_signal_id);
  }
  return TRUE;
}

static void tags_cb(GstElement *playbin, gint stream, CustomData *data) {
  gst_element_post_message(playbin, gst_message_new_application(GST_OBJECT(playbin),
                                                                gst_structure_new_empty("tags-changed")));
}

static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  GError *err;
  gchar *debug_info;

  gst_message_parse_error(msg, &err, &debug_info);
  g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
  g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error(&err);
  g_free(debug_info);

  gst_element_set_state(data->playbin, GST_STATE_READY);
}

static void eos_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  g_print("End-Of-Stream reached.\n");
  gst_element_set_state(data->playbin, GST_STATE_READY);
}

static void state_changed_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
  if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin)) {
    data->state = new_state;
    g_print("State set to %s\n", gst_element_state_get_name(new_state));
    if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
      refresh_ui(data);
    }
  }
}

static void application_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  if (g_strcmp0(gst_structure_get
