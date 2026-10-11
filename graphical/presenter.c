#include "presenter.h"
#include "db.h"
#include "widget_structs.h"
#include "wifi.h"

#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>
#include <glibconfig.h>
#include <gtk/gtk.h>
#include <gtk/gtkshortcut.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool get_saved_ip(char *buf, size_t max_len) {
  FILE *f = fopen(IP_CONFIG_FILE, "r");

  if (!f) {
    return false;
  }

  if (!fgets(buf, max_len, f)) {
    fclose(f);
    return false;
  }
  fclose(f);

  buf[strcspn(buf, "\r\n")] = '\0';
  g_strstrip(buf);
  return (strlen(buf) > 0);
}

bool save_ip(const char *ip) {
  if (!ip || strlen(ip) == 0) {
    return false;
  }

  FILE *f = fopen(IP_CONFIG_FILE, "w");
  if (!f) {
    return false;
  }

  fputs(ip, f);
  fputs("\n", f);
  fclose(f);
  return true;
}

void proto_send_start() { network_send("%$1$0$%"); }

void proto_send_stop() { network_send("%$2$0$%"); }

void proto_send_blackout(bool zatmav) {
  if (zatmav) {
    network_send("%$4$0$%");
  } else {
    network_send("%$5$0$%");
  }
}

void proto_send_switch_to(int song_index, int verse_number) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%%|%d|%d|%%", song_index, verse_number);
  network_send(buf);
}

char *nahrad_novy_riadok_strieskou(const char *src) {
  if (!src) {
    return strdup("");
  }
  char *dest = strdup(src);
  for (int i = 0; dest[i] != '\0'; i++) {
    if (dest[i] == '\n') {
      dest[i] = '^';
    }
  }
  return dest;
}

void proto_send_all_selected_songs(sqlite3 *db, GtkWidget *vybrane_list) {
  int song_idx = 0;
  GtkListBoxRow *row;

  while ((row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(vybrane_list),
                                              song_idx)) != NULL) {
    gpointer id_ptr = g_object_get_data(G_OBJECT(row), SONG_DATABASE_ID);
    int db_id = GPOINTER_TO_INT(id_ptr);

    song_t *song = get_song_by_database_id(db, db_id);
    if (!song) {
      song_idx++;
      continue;
    }

    char cmd_song[64];
    snprintf(cmd_song, sizeof(cmd_song), "%%$3$%d$%%", song_idx);
    network_send(cmd_song);
    usleep(30000);

    int verse_count = 0;
    verse_t **verses =
        get_verses_for_song(db, song->real_id, song->song_type, &verse_count);

    if (verses) {
      for (int v = 0; v < verse_count; v++) {
        char *clean_text = nahrad_novy_riadok_strieskou(verses[v]->text);

        char *verse_buf = g_strdup_printf("$$%d$$%s%%%%%%",
                                          verses[v]->verse_number, clean_text);

        if (verse_buf) {
          network_send(verse_buf);
          g_free(verse_buf);
        }
        free(clean_text);
        usleep(20000);
      }
      free_verses(verses, verse_count);
    }

    free(song);
    song_idx++;
    usleep(30000);
  }
  proto_send_start();
  usleep(50000);
  proto_send_switch_to(0, 1);
}

void presentation_send_slide(PresentationController *ctrl) {
  if (ctrl == NULL || !network_is_connected()) {
    return;
  }

  proto_send_blackout(ctrl->is_blank);
  usleep(20000);
  PresentationSlide *cur = &ctrl->slides[ctrl->current_index];
  proto_send_switch_to(cur->song_index, cur->verse_number);
}

void update_presentation_view(PresentationController *ctrl) {
  if (ctrl->count == 0 || ctrl->current_index < 0 ||
      ctrl->current_index >= ctrl->count) {
    return;
  }

  PresentationSlide *cur = &ctrl->slides[ctrl->current_index];

  char info_buf[256];
  snprintf(info_buf, sizeof(info_buf), "<b>%d - %s</b> (Sloha %d z %d)",
           cur->song_real_id, cur->song_title, cur->verse_number,
           cur->total_verses);
  gtk_label_set_markup(GTK_LABEL(ctrl->lbl_song_info), info_buf);

  gtk_label_set_text(GTK_LABEL(ctrl->lbl_verse_preview), cur->verse_text);

  if (ctrl->is_blank) {
    gtk_button_set_label(GTK_BUTTON(ctrl->btn_blank),
                         "Obrazovka vypnutá(SPACE)");
  } else {
    gtk_button_set_label(GTK_BUTTON(ctrl->btn_blank),
                         "Zapnutá obrazovka (SPACE)");
  }
  gtk_widget_set_sensitive(ctrl->btn_prev, ctrl->current_index > 0);
  gtk_widget_set_sensitive(ctrl->btn_next,
                           ctrl->current_index < ctrl->count - 1);

  GtkListBoxRow *row = gtk_list_box_get_row_at_index(
      GTK_LIST_BOX(ctrl->list_slides), ctrl->current_index);
  if (row) {
    gtk_list_box_select_row(GTK_LIST_BOX(ctrl->list_slides), row);
  }

  presentation_send_slide(ctrl);
}

void on_prev_slide(GtkWidget *btn, gpointer user_data) {
  PresentationController *ctrl = (PresentationController *)user_data;
  if (ctrl->current_index > 0) {
    ctrl->current_index--;
    update_presentation_view(ctrl);
  }
}

void on_next_slide(GtkWidget *btn, gpointer user_data) {
  PresentationController *ctrl = (PresentationController *)user_data;
  if (ctrl->current_index < ctrl->count - 1) {
    ctrl->current_index++;
    update_presentation_view(ctrl);
  }
}

void on_toggle_blank(GtkWidget *btn, gpointer user_data) {
  PresentationController *ctrl = (PresentationController *)user_data;
  ctrl->is_blank = !ctrl->is_blank;
  update_presentation_view(ctrl);
}

void on_slide_row_selected(GtkListBox *box, GtkListBoxRow *row,
                           gpointer user_data) {
  if (!row) {
    return;
  }

  PresentationController *ctrl = (PresentationController *)user_data;
  int idx = gtk_list_box_row_get_index(row);
  if (idx >= 0 && idx < ctrl->count && idx != ctrl->current_index) {
    ctrl->current_index = idx;
    update_presentation_view(ctrl);
  }
}

static gboolean show_connection_error_idle(gpointer user_data) {
  LoadingContext *ctx = (LoadingContext *)user_data;

  if (ctx->loading_window) {
    gtk_window_destroy(GTK_WINDOW(ctx->loading_window));
  }

  GtkWidget *dlg = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Chyba pripojenia!");
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dlg), 380, 150);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);

  char err_msg[256];
  snprintf(err_msg, sizeof(err_msg),
           "Nepodarilo sa pripojiť k premietaču na IP:\n<b>%s</b>", ctx->ip);

  GtkWidget *lbl = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(lbl), err_msg);
  gtk_box_append(GTK_BOX(box), lbl);

  GtkWidget *btn = gtk_button_new_with_label("OK");
  gtk_box_append(GTK_BOX(box), btn);
  gtk_window_set_child(GTK_WINDOW(dlg), box);

  g_signal_connect_swapped(btn, "clicked", G_CALLBACK(gtk_window_destroy), dlg);
  gtk_window_present(GTK_WINDOW(dlg));

  if (ctx->slides) {
    for (int i = 0; i < ctx->slide_count; i++) {
      free(ctx->slides[i].song_title);
      free(ctx->slides[i].verse_text);
    }
    free(ctx->slides);
  }
  g_free(ctx);

  return G_SOURCE_REMOVE;
}

gboolean on_presentation_key(GtkEventControllerKey *controller, guint keyval,
                             guint keycode, GdkModifierType state,
                             gpointer user_data) {
  PresentationController *ctrl = (PresentationController *)user_data;

  switch (keyval) {
  case GDK_KEY_Right:
  case GDK_KEY_Page_Down:
  case GDK_KEY_Down:
    on_next_slide(NULL, ctrl);
    return TRUE;

  case GDK_KEY_Left:
  case GDK_KEY_Page_Up:
  case GDK_KEY_Up:
    on_prev_slide(NULL, ctrl);
    return TRUE;

  case GDK_KEY_space:
  case GDK_KEY_b:
  case GDK_KEY_B:
    on_toggle_blank(NULL, ctrl);
    return TRUE;

  case GDK_KEY_Escape:
    gtk_window_destroy(GTK_WINDOW(ctrl->window));
    return TRUE;
  }
  return FALSE;
}

void on_presentation_destroy(GtkWidget *widget, gpointer user_data) {
  PresentationController *ctrl = (PresentationController *)user_data;

  if (ctrl) {
    proto_send_stop();
    for (int i = 0; i < ctrl->count; i++) {
      free(ctrl->slides[i].song_title);
      free(ctrl->slides[i].verse_text);
    }
    free(ctrl->slides);
    free(ctrl);
  }
}

void create_presentation_window(PresentationSlide *slides, int count,
                                const char *ip) {
  PresentationController *ctrl = g_new0(PresentationController, 1);
  ctrl->slides = slides;
  ctrl->count = count;
  ctrl->current_index = 0;
  ctrl->is_blank = false;
  snprintf(ctrl->current_ip, sizeof(ctrl->current_ip), "%s", ip);

  GtkWidget *win = gtk_window_new();
  ctrl->window = win;
  gtk_window_set_title(GTK_WINDOW(win), "Premietanie - Ovládací pult");
  gtk_window_maximize(GTK_WINDOW(win));

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(main_box, 15);
  gtk_widget_set_margin_end(main_box, 15);
  gtk_widget_set_margin_top(main_box, 10);
  gtk_widget_set_margin_bottom(main_box, 15);
  gtk_window_set_child(GTK_WINDOW(win), main_box);

  GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  char status_text[128];
  snprintf(status_text, sizeof(status_text), "Pripojene k premietaču: %s:%d",
           ip, DEFAULT_PORT);
  ctrl->lbl_status = gtk_label_new(status_text);
  gtk_box_append(GTK_BOX(top_bar), ctrl->lbl_status);

  GtkWidget *btn_close = gtk_button_new_with_label("Ukončiť premietanie");
  gtk_widget_set_halign(btn_close, GTK_ALIGN_END);
  gtk_widget_set_hexpand(btn_close, TRUE);
  g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(gtk_window_destroy),
                           win);
  gtk_box_append(GTK_BOX(top_bar), btn_close);

  gtk_box_append(GTK_BOX(main_box), top_bar);
  gtk_box_append(GTK_BOX(main_box),
                 gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  GtkWidget *body_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_vexpand(body_box, TRUE);
  gtk_box_append(GTK_BOX(main_box), body_box);

  GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_size_request(left_box, 320, -1);
  GtkWidget *lbl_title_list = gtk_label_new("<b>PORADIE SLÔH</b>");
  gtk_label_set_markup(GTK_LABEL(lbl_title_list), "<b>PORADIE SLÔH</b>");
  gtk_box_append(GTK_BOX(left_box), lbl_title_list);

  GtkWidget *scroll_list = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll_list, TRUE);
  ctrl->list_slides = gtk_list_box_new();

  for (int i = 0; i < count; i++) {
    char row_lbl[256];
    snprintf(row_lbl, sizeof(row_lbl), "%d - [%d/%d]", slides[i].song_real_id,
             slides[i].verse_number, slides[i].total_verses);

    GtkWidget *lbl = gtk_label_new(row_lbl);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_margin_start(lbl, 8);
    gtk_widget_set_margin_top(lbl, 6);
    gtk_widget_set_margin_bottom(lbl, 6);
    gtk_list_box_append(GTK_LIST_BOX(ctrl->list_slides), lbl);
  }

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_list),
                                ctrl->list_slides);
  gtk_box_append(GTK_BOX(left_box), scroll_list);
  gtk_box_append(GTK_BOX(body_box), left_box);

  GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_hexpand(right_box, TRUE);

  ctrl->lbl_song_info = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(ctrl->lbl_song_info), 0.0);
  gtk_box_append(GTK_BOX(right_box), ctrl->lbl_song_info);

  GtkWidget *preview_scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(preview_scroll, TRUE);

  ctrl->lbl_verse_preview = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(ctrl->lbl_verse_preview), TRUE);
  gtk_label_set_justify(GTK_LABEL(ctrl->lbl_verse_preview), GTK_JUSTIFY_CENTER);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(preview_scroll),
                                ctrl->lbl_verse_preview);
  gtk_box_append(GTK_BOX(right_box), preview_scroll);

  GtkWidget *btn_panel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_halign(btn_panel, GTK_ALIGN_CENTER);

  ctrl->btn_prev = gtk_button_new_with_label("Predchádzajúca");
  ctrl->btn_blank = gtk_button_new_with_label("Zatmaviť (SPACE)");
  ctrl->btn_next = gtk_button_new_with_label("Nasledujúca");

  gtk_box_append(GTK_BOX(btn_panel), ctrl->btn_prev);
  gtk_box_append(GTK_BOX(btn_panel), ctrl->btn_blank);
  gtk_box_append(GTK_BOX(btn_panel), ctrl->btn_next);
  gtk_box_append(GTK_BOX(right_box), btn_panel);

  gtk_box_append(GTK_BOX(body_box), right_box);

  g_signal_connect(ctrl->btn_prev, "clicked", G_CALLBACK(on_prev_slide), ctrl);
  g_signal_connect(ctrl->btn_next, "clicked", G_CALLBACK(on_next_slide), ctrl);
  g_signal_connect(ctrl->btn_blank, "clicked", G_CALLBACK(on_toggle_blank),
                   ctrl);
  g_signal_connect(ctrl->list_slides, "row-activated",
                   G_CALLBACK(on_slide_row_selected), ctrl);

  GtkEventController *key_ctrl = gtk_event_controller_key_new();
  g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_presentation_key),
                   ctrl);
  gtk_widget_add_controller(win, key_ctrl);

  g_signal_connect(win, "destroy", G_CALLBACK(on_presentation_destroy), ctrl);

  update_presentation_view(ctrl);
  gtk_window_present(GTK_WINDOW(win));
}

static gboolean update_progress_idle(gpointer user_data) {
  ProgressMsg *msg = (ProgressMsg *)user_data;
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(msg->progress_bar),
                                msg->fraction);
  if (msg->text) {
    gtk_label_set_text(GTK_LABEL(msg->lbl_status), msg->text);
    g_free(msg->text);
  }
  g_free(msg);
  return G_SOURCE_REMOVE;
}

static gboolean finish_loading_file(gpointer user_data) {
  LoadingContext *ctx = (LoadingContext *)user_data;
  gtk_window_destroy(GTK_WINDOW(ctx->loading_window));
  create_presentation_window(ctx->slides, ctx->slide_count, ctx->ip);
  g_free(ctx);
  return G_SOURCE_REMOVE;
}

static gpointer loading_worker_thread(gpointer user_data) {
  LoadingContext *ctx = (LoadingContext *)user_data;

  if (!network_is_connected()) {
    ProgressMsg *msg = g_new0(ProgressMsg, 1);
    msg->progress_bar = ctx->progress_bar;
    msg->lbl_status = ctx->lbl_status;
    msg->fraction = 0.05;
    msg->text = g_strdup("Pripájam sa k premietaču...");
    g_idle_add(update_progress_idle, msg);

    network_connect(ctx->ip, DEFAULT_PORT);
    usleep(50000);

    if (!network_is_connected()) {
      g_idle_add(show_connection_error_idle, ctx);
      return NULL;
    }
    save_ip(ctx->ip);
  }

  proto_send_start();
  usleep(30000);

  for (int song_idx = 0; song_idx < ctx->total_songs; song_idx++) {
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(
        GTK_LIST_BOX(ctx->vybrane_list), song_idx);
    if (!row)
      continue;
    gpointer id_ptr = g_object_get_data(G_OBJECT(row), "song_database_id");
    int db_id = GPOINTER_TO_INT(id_ptr);
    song_t *song = get_song_by_database_id(ctx->db, db_id);
    if (!song)
      continue;
    // Hlavička piesne
    char cmd_song[64];
    snprintf(cmd_song, sizeof(cmd_song), "%%$3$%d$%%", song_idx);
    network_send(cmd_song);
    usleep(25000);
    // Slohy piesne
    int verse_count = 0;
    verse_t **verses = get_verses_for_song(ctx->db, song->real_id,
                                           song->song_type, &verse_count);
    if (verses) {
      for (int v = 0; v < verse_count; v++) {
        char *clean_text = nahrad_novy_riadok_strieskou(verses[v]->text);
        char *verse_buf = g_strdup_printf("$$%d$$%s%%%%%%",
                                          verses[v]->verse_number, clean_text);
        if (verse_buf) {
          network_send(verse_buf);
          g_free(verse_buf);
        }
        free(clean_text);
        usleep(20000);
      }
      free_verses(verses, verse_count);
    }
    free(song);
    // Aktualizácia percent (napr. 1/3, 2/3, 3/3)
    double fraction = (double)(song_idx + 1) / (double)ctx->total_songs;
    ProgressMsg *msg = g_new0(ProgressMsg, 1);
    msg->progress_bar = ctx->progress_bar;
    msg->lbl_status = ctx->lbl_status;
    msg->fraction = fraction;
    msg->text = g_strdup_printf("Odosielanie piesní: %d z %d...", song_idx + 1,
                                ctx->total_songs);
    g_idle_add(update_progress_idle, msg);
    usleep(30000);
  }

  proto_send_switch_to(0, 1);
  usleep(100000);
  g_idle_add(finish_loading_file, ctx);
  return NULL;
}

void start_presentation_process(PresentationStartData *data, const char *ip,
                                int total_songs) {
  int capacity = 16;
  int slide_count = 0;
  PresentationSlide *slides = malloc(capacity * sizeof(PresentationSlide));

  for (int s_idx = 0; s_idx < total_songs; s_idx++) {
    GtkListBoxRow *row =
        gtk_list_box_get_row_at_index(GTK_LIST_BOX(data->vybrane_list), s_idx);
    gpointer id_ptr = g_object_get_data(G_OBJECT(row), SONG_DATABASE_ID);
    int db_id = GPOINTER_TO_INT(id_ptr);

    song_t *song = get_song_by_database_id(data->db, db_id);
    if (!song) {
      continue;
    }

    int verse_count = 0;
    verse_t **verses = get_verses_for_song(data->db, song->real_id,
                                           song->song_type, &verse_count);

    if (verses) {
      for (int v = 0; v < verse_count; v++) {
        if (slide_count >= capacity) {
          capacity *= 2;
          PresentationSlide *temp =
              realloc(slides, capacity * sizeof(PresentationSlide));
          if (!temp) {
            perror("Dosla pamat pri alokovanie pre slidy");
            return;
          }
          slides = temp;
        }
        slides[slide_count].song_index = s_idx;
        slides[slide_count].song_title = strdup(song->title);
        slides[slide_count].song_real_id = song->real_id;
        slides[slide_count].verse_number = verses[v]->verse_number;
        slides[slide_count].total_verses = verse_count;
        slides[slide_count].verse_text =
            strdup(verses[v]->text ? verses[v]->text : "");
        slide_count++;
      }
      free_verses(verses, verse_count);
    }
    free(song);
  }

  GtkWidget *load_win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(load_win), "Príprava premietania");
  gtk_window_set_modal(GTK_WINDOW(load_win), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(load_win), 420, 160);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_widget_set_margin_start(vbox, 25);
  gtk_widget_set_margin_end(vbox, 25);
  gtk_widget_set_margin_top(vbox, 25);
  gtk_widget_set_margin_bottom(vbox, 25);
  gtk_window_set_child(GTK_WINDOW(load_win), vbox);

  GtkWidget *lbl_status = gtk_label_new("Spúšťam odosielanie do premietača");
  gtk_label_set_xalign(GTK_LABEL(lbl_status), 0.0);
  gtk_box_append(GTK_BOX(vbox), lbl_status);

  GtkWidget *prog_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(prog_bar), TRUE);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prog_bar), 0.0);
  gtk_box_append(GTK_BOX(vbox), prog_bar);

  gtk_window_present(GTK_WINDOW(load_win));

  LoadingContext *ctx = g_new0(LoadingContext, 1);
  ctx->loading_window = load_win;
  ctx->progress_bar = prog_bar;
  ctx->lbl_status = lbl_status;
  ctx->db = data->db;
  ctx->vybrane_list = data->vybrane_list;
  snprintf(ctx->ip, sizeof(ctx->ip), "%s", ip);
  ctx->slides = slides;
  ctx->slide_count = slide_count;
  ctx->total_songs = total_songs;

  g_thread_new("presentation_loader", loading_worker_thread, ctx);
}

void on_save_ip_clicked(GtkWidget *button, gpointer user_data) {
  IpPromptContext *promt_ctx = (IpPromptContext *)user_data;
  const char *eneterd_ip =
      gtk_editable_get_text(GTK_EDITABLE(promt_ctx->entry_ip));

  if (strlen(eneterd_ip) == 0) {
    gtk_label_set_text(GTK_LABEL(promt_ctx->lbl_error),
                       "IP adresa nemôže byť prázdna");
    return;
  }

  int total_songs = 0;

  while (
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(promt_ctx->data->vybrane_list),
                                    total_songs) != NULL) {
    total_songs++;
  }

  GtkWidget *dialog = promt_ctx->dialog;
  PresentationStartData *data = promt_ctx->data;
  char saved_ip[64] = {0};
  snprintf(saved_ip, sizeof(saved_ip), "%s", eneterd_ip);

  g_free(promt_ctx);
  start_presentation_process(data, saved_ip, total_songs);
}

void show_ip_prompt_dialog(PresentationStartData *data) {
  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Nastavenie IP adresy");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 380, 220);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);
  gtk_window_set_child(GTK_WINDOW(dialog), box);

  GtkWidget *lbl_info = gtk_label_new("Zadajte ip adresu premietača:");
  gtk_label_set_xalign(GTK_LABEL(lbl_info), 0.0);
  gtk_box_append(GTK_BOX(box), lbl_info);

  GtkWidget *entry_ip = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_ip), "napr. 192.168.1.1");
  gtk_box_append(GTK_BOX(box), entry_ip);

  GtkWidget *lbl_error = gtk_label_new("");
  gtk_widget_add_css_class(lbl_error, "error-label");
  gtk_box_append(GTK_BOX(box), lbl_error);

  IpPromptContext *prompt_ctx = g_new0(IpPromptContext, 1);
  prompt_ctx->dialog = dialog;
  prompt_ctx->entry_ip = entry_ip;
  prompt_ctx->lbl_error = lbl_error;
  prompt_ctx->data = data;

  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *btn_save = gtk_button_new_with_label("Uložiť a pokračovať");
  GtkWidget *btn_cancel = gtk_button_new_with_label("Zrušiť");

  gtk_box_append(GTK_BOX(btn_box), btn_save);
  gtk_box_append(GTK_BOX(btn_box), btn_cancel);
  gtk_box_append(GTK_BOX(box), btn_box);

  g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_ip_clicked),
                   prompt_ctx);
  g_signal_connect_swapped(btn_cancel, "clicked",
                           G_CALLBACK(gtk_window_destroy), dialog);

  gtk_window_present(GTK_WINDOW(dialog));
}

void on_start_presentation_clicked(GtkWidget *button, gpointer user_data) {
  PresentationStartData *data = (PresentationStartData *)user_data;
  if (!data || !data->vybrane_list || !data->db) {
    return;
  }

  int total_songs = 0;
  while (gtk_list_box_get_row_at_index(GTK_LIST_BOX(data->vybrane_list),
                                       total_songs) != NULL) {
    total_songs++;
  }

  if (total_songs == 0) {
    GtkWidget *dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "Upozornenie!!");
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 360, 140);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_box_append(
        GTK_BOX(box),
        gtk_label_new("Nie sú vybraté žiadne piesne na premietanie!"));

    GtkWidget *btn = gtk_button_new_with_label("OK");
    gtk_box_append(GTK_BOX(box), btn);
    gtk_window_set_child(GTK_WINDOW(dlg), box);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(gtk_window_destroy),
                             dlg);
    gtk_window_present(GTK_WINDOW(dlg));
    return;
  }

  char ip[64] = {0};

  if (!get_saved_ip(ip, sizeof(ip))) {
    show_ip_prompt_dialog(data);
    return;
  }

  start_presentation_process(data, ip, total_songs);
}
