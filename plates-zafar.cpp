#include <gtk/gtk.h>
#include <gst/gst.h>

class HelloWindow {
public:
    HelloWindow() {
        app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
        g_signal_connect(app, "activate", G_CALLBACK(onActivate), this);
    }

    int run(int argc, char **argv) {
        return g_application_run(G_APPLICATION(app), argc, argv);
    }

private:
    GtkApplication *app;
    
    static void printHello(GtkWidget *widget, gpointer data) {
        g_print("Hello Boss\n");
    }

    static void onActivate(GtkApplication *papp, gpointer user_data) {



        GtkWidget *g_window = gtk_window_new();
        GtkWidget* vid_window, *widg_window;

        gtk_window_set_default_size(GTK_WINDOW(g_window), 640, 480);
        gtk_window_set_title(GTK_WINDOW(g_window), "testing input gstreamer");
        

        // vid_window = gtk_drawing_area_new();
        // gtk_container_add (GTK_CONTAINER (g_window), vid_window);
        // gtk_container_set_border_width(GTK_CONTAINER(g_window), 2);


        gtk_window_present(GTK_WINDOW(g_window));

        GstElement *src, *sink;

        src = gst_element_factory_make("v4l2src", NULL);
        sink = gst_element_factory_make("ximagesink", NULL);
        
        GstElement *pipeline;
        pipeline = gst_pipeline_new("pip-plates");

        gst_bin_add_many(GST_BIN(pipeline), src, sink, NULL);
        int res = gst_element_link(src, sink);

        if(res == -1)
        {
            return;
        }

        int ret_pip = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret_pip == GST_STATE_CHANGE_FAILURE)
            gst_element_set_state (pipeline, GST_STATE_NULL);

        GtkWidget *window = gtk_application_window_new(papp);
        gtk_window_set_title(GTK_WINDOW(window), "Hello");
        gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

        GtkWidget *button = gtk_button_new_with_label("Hello World");
        g_signal_connect(button, "clicked", G_CALLBACK(printHello), NULL);
        gtk_window_set_child(GTK_WINDOW(window), button);

        gtk_window_present(GTK_WINDOW(window));
    }
};

int main(int argc, char **argv) {
    gst_init (&argc, &argv);
    HelloWindow hellowindow;
    return hellowindow.run(argc, argv);
}
