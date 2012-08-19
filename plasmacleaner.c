// Displays a moving white bar to remove image retention from plasma monitors or
// HDTVs.
//
// Copyright (c) 2012 Tristan Schmelcher <tristan_schmelcher@alumni.uwaterloo.ca>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

#include <assert.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/X.h>

// Number of milliseconds for the white bar to move across the screen.
static const guint PERIOD_MS = 4000;
// White bar's width as a fraction of the screen width.
static const double BAR_FRACTION = 3.0/8;
// How often to simulate mouse movement to suppress screensaver.
static const guint SCREENSAVER_SUPPRESSION_PERIOD_MS = 1000;

struct data_t {
  GtkWidget *window;
  cairo_pattern_t *pattern;
  guint draw_timeout_id;
  guint x;
  guint last_width;
};

static void free_draw_timeout_and_pattern(struct data_t *data) {
  if (data->draw_timeout_id) {
    g_source_remove(data->draw_timeout_id);
    data->draw_timeout_id = 0;
  }
  if (data->pattern) {
    cairo_pattern_destroy(data->pattern);
    data->pattern = NULL;
  }
}

static gboolean on_draw_timer(gpointer user_data) {
  struct data_t *data = (struct data_t *)user_data;
  data->x++;
  gtk_widget_queue_draw(data->window);
  return TRUE;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
  struct data_t *data = (struct data_t *)user_data;

  gint width = gtk_widget_get_allocated_width(widget);
  if (data->last_width != width) {
    free_draw_timeout_and_pattern(data);

    data->draw_timeout_id = g_timeout_add(PERIOD_MS / (double)width,
        &on_draw_timer, data);

    cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 0, width, 0);
    cairo_pattern_add_color_stop_rgb(pattern, 0.0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgb(pattern, BAR_FRACTION, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgb(pattern, BAR_FRACTION, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgb(pattern, 1.0, 0.0, 0.0, 0.0);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    data->pattern = pattern;
  }
  data->last_width = width;

  data->x %= width;

  cairo_translate(cr, data->x, 0.0);
  cairo_set_source(cr, data->pattern);
  cairo_paint(cr);

  return TRUE;
}

static gboolean on_button_or_key_press(GtkWidget *widget, GdkEvent *event,
    gpointer unused) {
  gtk_widget_destroy(widget);
  return TRUE;
}

static void on_destroy(GtkWidget *widget, gpointer user_data) {
  struct data_t *data = (struct data_t *)user_data;
  free_draw_timeout_and_pattern(data);
  gtk_main_quit();
}

static gboolean on_screensaver_suppression_timer(gpointer unused) {
  // The XScreenSaverSuspend method doesn't work with gnome-screensaver, so
  // instead we synthesize a mouse mouse event (but with offset of 0x0, so it
  // doesn't actually move).
  Display *display = gdk_x11_display_get_xdisplay(gdk_display_get_default());
  assert(display);
  XWarpPointer(display, None, None, 0, 0, 0, 0, 0, 0);
  return TRUE;
}

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);

  struct data_t data = {0};
  data.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  assert(data.window);
  gtk_window_set_title(GTK_WINDOW(data.window), "Plasma Cleaner");
  gtk_window_set_keep_above(GTK_WINDOW(data.window), TRUE);
  gtk_widget_add_events(data.window, GDK_BUTTON_PRESS_MASK|GDK_KEY_PRESS_MASK);
  gtk_window_fullscreen(GTK_WINDOW(data.window));
  g_signal_connect(G_OBJECT(data.window), "draw", G_CALLBACK(&on_draw), &data);
  g_signal_connect(G_OBJECT(data.window), "destroy", G_CALLBACK(&on_destroy),
      &data);
  g_signal_connect(G_OBJECT(data.window), "button-press-event",
      G_CALLBACK(&on_button_or_key_press), NULL);
  g_signal_connect(G_OBJECT(data.window), "key-press-event",
      G_CALLBACK(&on_button_or_key_press), NULL);
  gtk_widget_realize(data.window);
  GdkCursor *cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
  assert(cursor);
  gdk_window_set_cursor(gtk_widget_get_window(data.window), cursor);
  g_object_unref(cursor);
  gtk_window_present(GTK_WINDOW(data.window));
  guint screensaver_suppression_timeout_id = g_timeout_add(
      SCREENSAVER_SUPPRESSION_PERIOD_MS, &on_screensaver_suppression_timer,
      NULL);

  gtk_main();

  g_source_remove(screensaver_suppression_timeout_id);
  return 0;
}
