#ifndef PRESENTER_H
#define PRESENTER_H

#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>

#define IP_CONFIG_FILE "server_ip.txt"
#define DEFAULT_PORT 2020

typedef struct {
  int song_index;
  char *song_title;
  int song_real_id;
  int verse_number;
  int total_verses;
  char *verse_text;

} PresentationSlide;

typedef struct {
  PresentationSlide *slides;
  int count;
  int current_index;
  bool is_blank;
  char current_ip[64];

  GtkWidget *window;
  GtkWidget *lbl_song_info;
  GtkWidget *lbl_verse_preview;
  GtkWidget *lbl_status;
  GtkWidget *btn_prev;
  GtkWidget *btn_next;
  GtkWidget *btn_blank;
  GtkWidget *list_slides;
} PresentationController;

typedef struct {
  GtkWidget *vybrane_list;
  sqlite3 *db;
} PresentationStartData;

typedef struct {
  GtkWidget *loading_window;
  GtkWidget *progress_bar;
  GtkWidget *lbl_status;
  sqlite3 *db;
  GtkWidget *vybrane_list;
  char ip[64];
  PresentationSlide *slides;
  int slide_count;
  int total_songs;
} LoadingContext;

typedef struct {
  GtkWidget *progress_bar;
  GtkWidget *lbl_status;
  double fraction;
  char *text;
} ProgressMsg;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *entry_ip;
  GtkWidget *lbl_error;
  PresentationStartData *data;
} IpPromptContext;

void start_presentation_process(PresentationStartData *data, const char *ip, int total_songs);

void on_save_ip_clicked(GtkWidget *button, gpointer user_data);

void show_ip_prompt_dialog(PresentationStartData *data);

bool get_saved_ip(char *buf, size_t max_len);

bool save_ip(const char *ip);

void proto_send_start(void);

void proto_send_stop(void);

void proto_send_blackout(bool zatmav);

void proto_send_switch_to(int song_index, int verse_number);

char *nahrad_novy_riadok_strieskou(const char *src);

void proto_send_all_selected_songs(sqlite3 *db, GtkWidget *vybrane_list);

void presentation_send_slide(PresentationController *ctrl);

void update_presentation_view(PresentationController *ctrl);

void on_prev_slide(GtkWidget *btn, gpointer user_data);

void on_next_slide(GtkWidget *btn, gpointer user_data);

void on_toggle_blank(GtkWidget *btn, gpointer user_data);

void on_slide_row_selected(GtkListBox *box, GtkListBoxRow *row,
                           gpointer user_data);

gboolean on_presentation_key(GtkEventControllerKey *controller, guint keyval,
                             guint keycode, GdkModifierType state,
                             gpointer user_data);

void on_presentation_destroy(GtkWidget *widget, gpointer user_data);

void create_presentation_window(PresentationSlide *slides, int count,
                                const char *ip);

void on_start_presentation_clicked(GtkWidget *button, gpointer user_data);

#endif