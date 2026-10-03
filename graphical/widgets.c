#include "widgets.h"
#include "db.h"
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <glibconfig.h>
#include <gtk/gtk.h>
#include <gtk/gtkdropdown.h>
#include <gtk/gtkshortcut.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_JKS_INDEX 0
#define MODE_OSTATNE_INDEX 1

#define ROW_JKS "is_jks"
#define ROW_ENUM_VAL "enum_val"

#define SONG_DATABASE_ID "song_database_id"

static void on_save_edit_song_clicked(GtkWidget *button, gpointer user_data) {
  EditSongDialogData *data = (EditSongDialogData *)user_data;

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(data->text_buffer, &start, &end);
  char *song_text =
      gtk_text_buffer_get_text(data->text_buffer, &start, &end, FALSE);

  if (song_text == NULL || strlen(song_text) == 0) {
    gtk_label_set_text(GTK_LABEL(data->lbl_error),
                       "Text nesmie byť prázdny!!!");
    if (song_text) {
      g_free(song_text);
      return;
    }
  }

  if (update_song_verses(data->db, data->real_id, data->song_type, song_text)) {
    g_free(song_text);
    GtkWidget *dialog = data->dialog;
    g_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
  } else {
    gtk_label_set_text(GTK_LABEL(data->lbl_error),
                       "Chyba pri aktualizácii databázy!!");
    g_free(song_text);
  }
}

static void on_btn_edit_song_clicked(GtkWidget *button, gpointer user_data) {
  EditSongData *data = (EditSongData *)user_data;
  if (data == NULL || data->song_list == NULL || data->db == NULL) {
    return;
  }

  GtkListBoxRow *selected_row =
      gtk_list_box_get_selected_row(GTK_LIST_BOX(data->song_list));
  if (selected_row == NULL) {
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Upozornenie");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 150);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *lbl = gtk_label_new("Nie je vybraná pieseň zo zoznamu!");
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *btn_ok = gtk_button_new_with_label("OK");
    gtk_box_append(GTK_BOX(box), btn_ok);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    g_signal_connect_swapped(btn_ok, "clicked", G_CALLBACK(gtk_window_destroy),
                             dialog);
    gtk_window_present(GTK_WINDOW(dialog));
    return;
  }

  gpointer id_ptr = g_object_get_data(G_OBJECT(selected_row), SONG_DATABASE_ID);
  int database_id = GPOINTER_TO_INT(id_ptr);

  song_t *song = get_song_by_database_id(data->db, database_id);
  if (song == NULL) {
    return;
  }

  char *fullverses =
      get_song_verses_text(data->db, song->real_id, song->song_type);

  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Upraviť slohy piesne");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_maximize(GTK_WINDOW(dialog));

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 15);
  gtk_widget_set_margin_end(box, 15);
  gtk_widget_set_margin_top(box, 15);
  gtk_widget_set_margin_bottom(box, 15);
  gtk_window_set_child(GTK_WINDOW(dialog), box);

  EditSongDialogData *dlg_data = g_new0(EditSongDialogData, 1);
  dlg_data->dialog = dialog;
  dlg_data->song_type = song->song_type;
  dlg_data->db = data->db;
  dlg_data->real_id = song->real_id;

  char title_buf[MAX_TITLE];

  snprintf(title_buf, sizeof(title_buf), "<b>Pieseň %d: %s</b>", song->real_id,
           song->title);
  GtkWidget *lbl_title = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(lbl_title), title_buf);
  gtk_label_set_xalign(GTK_LABEL(lbl_title), 0.0);
  gtk_box_append(GTK_BOX(box), lbl_title);

  gtk_box_append(
      GTK_BOX(box),
      gtk_label_new("Text piesne (slohy oddeľujte prázdnym riadkom):"));

  GtkWidget *scrolled_text = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scrolled_text, TRUE);
  gtk_widget_set_hexpand(scrolled_text, TRUE);
  GtkWidget *text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_text), text_view);

  dlg_data->text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  if (fullverses != NULL) {
    gtk_text_buffer_set_text(dlg_data->text_buffer, fullverses, -1);
    free(fullverses);
  }

  gtk_box_append(GTK_BOX(box), scrolled_text);

  dlg_data->lbl_error = gtk_label_new("");
  gtk_widget_add_css_class(dlg_data->lbl_error, "error-label");
  gtk_box_append(GTK_BOX(box), dlg_data->lbl_error);

  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *btn_save = gtk_button_new_with_label("Uložiť zmeny");
  GtkWidget *btn_cancel = gtk_button_new_with_label("Zrušiť");

  gtk_box_append(GTK_BOX(btn_box), btn_save);
  gtk_box_append(GTK_BOX(btn_box), btn_cancel);
  gtk_box_append(GTK_BOX(box), btn_box);

  g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_edit_song_clicked),
                   dlg_data);
  g_signal_connect_swapped(btn_cancel, "clicked",
                           G_CALLBACK(gtk_window_destroy), dialog);

  free(song);

  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_confirm_delete_clicked(GtkWidget *button, gpointer user_data) {
  DeleteSongDialogData *data = (DeleteSongDialogData *)user_data;

  if (delete_song(data->db, data->database_id)) {
    if (data->selected_row != NULL && data->song_list != NULL) {
      gtk_list_box_remove(data->song_list, GTK_WIDGET(data->selected_row));
    }
  }

  GtkWidget *dialog = data->dialog;
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dialog));
  ;
}

static void on_btn_delete_song_clicked(GtkWidget *button, gpointer user_data) {
  DeleteSongData *data = (DeleteSongData *)user_data;

  if (data == NULL || data->song_list == NULL || data->db == NULL) {
    return;
  }

  GtkListBoxRow *selected_row =
      gtk_list_box_get_selected_row(GTK_LIST_BOX(data->song_list));
  if (selected_row == NULL) {
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Upozornenie");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 150);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *lbl =
        gtk_label_new("Nebola vybratá žiadna pieseň zo zoznamu !!");
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *btn_ok = gtk_button_new_with_label("OK");
    gtk_box_append(GTK_BOX(box), btn_ok);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    g_signal_connect_swapped(btn_ok, "clicked", G_CALLBACK(gtk_window_destroy),
                             dialog);

    gtk_window_present(GTK_WINDOW(dialog));
    return;
  }

  gpointer id_ptr = g_object_get_data(G_OBJECT(selected_row), SONG_DATABASE_ID);
  int database_id = GPOINTER_TO_INT(id_ptr);

  GtkWidget *child = gtk_list_box_row_get_child(selected_row);
  const char *song_title = GTK_IS_LABEL(child)
                               ? gtk_label_get_text(GTK_LABEL(child))
                               : "vybranú pieseň";

  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Potvrdenie vymazania");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 180);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);
  gtk_window_set_child(GTK_WINDOW(dialog), box);

  char msg[512] = {0};
  snprintf(msg, sizeof(msg), "Noazaj chcete vymazať pieseň:\n<b>%s</b>?",
           song_title);

  GtkWidget *lbl_msg = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(lbl_msg), msg);
  gtk_label_set_justify(GTK_LABEL(lbl_msg), GTK_JUSTIFY_CENTER);
  gtk_box_append(GTK_BOX(box), lbl_msg);

  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);

  GtkWidget *btn_delete = gtk_button_new_with_label("Vymazať");
  gtk_widget_add_css_class(btn_delete, "destructive-action");

  GtkWidget *btn_cancel = gtk_button_new_with_label("Zrušiť");

  gtk_box_append(GTK_BOX(btn_box), btn_delete);
  gtk_box_append(GTK_BOX(btn_box), btn_cancel);
  gtk_box_append(GTK_BOX(box), btn_box);

  DeleteSongDialogData *dlg_data = g_new0(DeleteSongDialogData, 1);
  dlg_data->dialog = dialog;
  dlg_data->song_list = GTK_LIST_BOX(data->song_list);
  dlg_data->database_id = database_id;
  dlg_data->db = data->db;
  dlg_data->selected_row = selected_row;

  g_signal_connect(btn_delete, "clicked", G_CALLBACK(on_confirm_delete_clicked),
                   dlg_data);
  g_signal_connect_swapped(btn_cancel, "clicked",
                           G_CALLBACK(gtk_window_destroy), dialog);

  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_add_mode_changed(GtkDropDown *dropdown, GParamSpec *pspec,
                                gpointer user_data) {
  AddSongDialogData *data = (AddSongDialogData *)user_data;
  guint selected = gtk_drop_down_get_selected(dropdown);

  const char **kategorie =
      (selected == MODE_JKS_INDEX) ? JKS_KATEGORIE : OSTATNE_KATEGORIE;

  GtkStringList *sl = gtk_string_list_new(kategorie);
  gtk_drop_down_set_model(GTK_DROP_DOWN(data->dropdown_subcat),
                          G_LIST_MODEL(sl));
  g_object_unref(sl);
}

static void on_save_song_clicked(GtkWidget *button, gpointer user_data) {
  AddSongDialogData *data = (AddSongDialogData *)user_data;

  const char *id_str = gtk_editable_get_text(GTK_EDITABLE(data->entry_id));
  const char *title_str =
      gtk_editable_get_text(GTK_EDITABLE(data->entry_title));

  if (strlen(id_str) == 0 || strlen(title_str) == 0) {
    gtk_label_set_text(GTK_LABEL(data->lbl_error),
                       "Vyplňte číslo aj názov piesne!!!");
    return;
  }

  int real_id = atoi(id_str);

  if (real_id <= 0) {
    gtk_label_set_text(GTK_LABEL(data->lbl_error), "Zadaj platné číslo!!!");
    return;
  }

  guint mode_idx =
      gtk_drop_down_get_selected(GTK_DROP_DOWN(data->dropdown_mode));
  guint subcat_idx =
      gtk_drop_down_get_selected(GTK_DROP_DOWN(data->dropdown_subcat));

  song_type_t st;
  if (mode_idx == MODE_JKS_INDEX) {
    st.kind = JKS;
    st.data.jks_typ = (jks_type_t)subcat_idx;
  } else {
    st.kind = (song_type_fake_t)(subcat_idx + 1);
  }

  if (check_song_id_exists(data->db, st, real_id)) {
    gtk_label_set_text(GTK_LABEL(data->lbl_error),
                       "Pieseň s týmto ID už v danej kategórii existuje!!!!");
    return;
  }

  gtk_label_set_text(GTK_LABEL(data->lbl_error), "OK! ID je unikátne");

  GtkTextIter start, end;

  gtk_text_buffer_get_bounds(data->text_buffer, &start, &end);
  char *song_text =
      gtk_text_buffer_get_text(data->text_buffer, &start, &end, FALSE);

  if (song_text == NULL || strlen(song_text) == 0) {
    gtk_label_set_text(GTK_LABEL(data->lbl_error), "Zadajte text piesne!!!");
    if (song_text) {
      g_free(song_text);
      return;
    }
  }

  song_t song;
  memset(&song, 0, sizeof(song_t));
  song.real_id = real_id;
  song.song_type = st;
  snprintf(song.title, sizeof(song.title), "%s", title_str);

  if (save_song_with_verses(data->db, &song, song_text)) {
    g_free(song_text);

    GtkWidget *dialog = data->dialog;
    g_free(data);

    gtk_window_destroy(GTK_WINDOW(dialog));
  } else {
    gtk_label_set_text(GTK_LABEL(data->lbl_error),
                       "Chyba pri zápise do databázy!!!");
    g_free(song_text);
  }
}

static void on_btn_add_song_clicked(GtkWidget *button, gpointer user_data) {
  sqlite3 *db = (sqlite3 *)user_data;

  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Priadnie novej piesne");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_maximize(GTK_WINDOW(dialog));

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 15);
  gtk_widget_set_margin_end(box, 15);
  gtk_widget_set_margin_top(box, 15);
  gtk_widget_set_margin_bottom(box, 15);
  gtk_window_set_child(GTK_WINDOW(dialog), box);

  AddSongDialogData *data = g_new0(AddSongDialogData, 1);
  data->dialog = dialog;
  data->db = db;

  gtk_box_append(GTK_BOX(box), gtk_label_new("Typ spevníka: "));
  data->dropdown_mode = gtk_drop_down_new_from_strings(SPEVNIK_MODES);
  gtk_box_append(GTK_BOX(box), data->dropdown_mode);

  gtk_box_append(GTK_BOX(box), gtk_label_new("Kategória:"));
  GtkStringList *sl = gtk_string_list_new(JKS_KATEGORIE);
  data->dropdown_subcat = gtk_drop_down_new(G_LIST_MODEL(sl), NULL);
  gtk_box_append(GTK_BOX(box), data->dropdown_subcat);

  g_signal_connect(data->dropdown_mode, "notify::selected",
                   G_CALLBACK(on_add_mode_changed), data);

  // cislo piesne
  gtk_box_append(GTK_BOX(box), gtk_label_new("ID piesne na pridanie"));
  data->entry_id = gtk_entry_new();
  gtk_box_append(GTK_BOX(box), data->entry_id);

  // nazov piesne
  gtk_box_append(GTK_BOX(box), gtk_label_new("Názov piesne:"));
  data->entry_title = gtk_entry_new();
  gtk_box_append(GTK_BOX(box), data->entry_title);

  // textovy editor pre pridanie sloh
  gtk_box_append(
      GTK_BOX(box),
      gtk_label_new("Text piesne (slohy oddeľujte prázdnym riadkom):"));

  GtkWidget *scrolled_text = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scrolled_text, TRUE);
  gtk_widget_set_hexpand(scrolled_text, TRUE);

  GtkWidget *text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_text), text_view);

  data->text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

  gtk_box_append(GTK_BOX(box), scrolled_text);

  // vypis chyby
  data->lbl_error = gtk_label_new("");
  gtk_widget_add_css_class(data->lbl_error, "error-label");
  gtk_box_append(GTK_BOX(box), data->lbl_error);

  // tlacidla ulozit zrusit
  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *btn_save = gtk_button_new_with_label("Uložiť");
  GtkWidget *btn_cancel = gtk_button_new_with_label("Zrušiť");

  gtk_box_append(GTK_BOX(btn_box), btn_save);
  gtk_box_append(GTK_BOX(btn_box), btn_cancel);
  gtk_box_append(GTK_BOX(box), btn_box);

  g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_song_clicked), data);
  g_signal_connect_swapped(btn_cancel, "clicked",
                           G_CALLBACK(gtk_window_destroy), dialog);

  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_search_clicked(GtkWidget *widget, gpointer user_data) {
  SearchData *data = (SearchData *)user_data;
  if (data == NULL || data->entry_search == NULL || data->song_list == NULL ||
      data->db == NULL) {
    return;
  }

  const char *text = gtk_editable_get_text(GTK_EDITABLE(data->entry_search));
  if (text == NULL || strlen(text) == 0) {
    return;
  }

  gtk_list_box_remove_all(GTK_LIST_BOX(data->song_list));

  song_list_t *list = search_songs(data->db, text);
  if (list == NULL) {
    return;
  }

  for (int i = 0; i < list->count; i++) {
    char label_buf[MAX_TITLE];
    snprintf(label_buf, sizeof(label_buf), "%d - %s", list->songs[i].real_id,
             list->songs[i].title);

    GtkWidget *label = gtk_label_new(label_buf);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);

    GtkWidget *song_row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(song_row), label);

    g_object_set_data(G_OBJECT(song_row), SONG_DATABASE_ID,
                      GINT_TO_POINTER(list->songs[i].database_id));

    gtk_list_box_append(GTK_LIST_BOX(data->song_list), song_row);
  }
  free_song_list(list);
}

static void on_clear_search_clicked(GtkWidget *widget, gpointer user_data) {
  SearchData *data = (SearchData *)user_data;
  if (data == NULL) {
    return;
  }

  gtk_editable_set_text(GTK_EDITABLE(data->entry_search), "");

  gtk_list_box_remove_all((GTK_LIST_BOX(data->song_list)));
}

static void on_song_double_clicked_remove(GtkListBox *box, GtkListBoxRow *row,
                                          gpointer user_data) {
  if (row == NULL || box == NULL) {
    return;
  }
  gtk_list_box_remove(box, GTK_WIDGET(row));
}

static void on_song_double_clicked_add(GtkListBox *box, GtkListBoxRow *row,
                                       gpointer user_data) {
  if (row == NULL) {
    return;
  }
  GtkWidget *vybrane_list = GTK_WIDGET(user_data);
  if (vybrane_list == NULL) {
    return;
  }
  GtkWidget *child_label = gtk_list_box_row_get_child(row);
  if (!GTK_IS_LABEL(child_label)) {
    return;
  }

  const char *song_text = gtk_label_get_text(GTK_LABEL(child_label));

  gpointer song_db_id = g_object_get_data(G_OBJECT(row), SONG_DATABASE_ID);
  GtkWidget *label = gtk_label_new(song_text);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_widget_set_margin_start(label, 8);
  gtk_widget_set_margin_top(label, 6);
  gtk_widget_set_margin_bottom(label, 6);

  GtkWidget *new_row = gtk_list_box_row_new();
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(new_row), label);

  g_object_set_data(G_OBJECT(new_row), SONG_DATABASE_ID, song_db_id);
  gtk_list_box_append(GTK_LIST_BOX(vybrane_list), new_row);
}

static void on_category_selected(GtkListBox *box, GtkListBoxRow *row,
                                 gpointer user_data) {
  if (row == NULL) {
    return;
  }
  SelectedKategoriaData *data = (SelectedKategoriaData *)user_data;
  if (data == NULL || data->song_list == NULL || data->db == NULL) {
    return;
  }

  int is_jks = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), ROW_JKS));
  int enum_val =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), ROW_ENUM_VAL));

  gtk_list_box_remove_all(GTK_LIST_BOX(data->song_list));

  song_type_t st;
  if (is_jks) {
    st.kind = JKS;
    st.data.jks_typ = (jks_type_t)enum_val;
  } else {
    st.kind = (song_type_fake_t)enum_val;
  }
  const char *debug_text = song_type_get_text(st);

  song_list_t *list = get_songs_by_song_type(data->db, st);
  if (list == NULL) {
    return;
  }

  for (int i = 0; i < list->count; i++) {
    char label_buf[MAX_TITLE];

    snprintf(label_buf, MAX_TITLE, "%d - %s", list->songs[i].real_id,
             list->songs[i].title);

    GtkWidget *label = gtk_label_new(label_buf);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    GtkWidget *song_row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(song_row), label);

    g_object_set_data(G_OBJECT(song_row), SONG_DATABASE_ID,
                      GINT_TO_POINTER(list->songs[i].database_id));

    gtk_list_box_append(GTK_LIST_BOX(data->song_list), song_row);
  }
  free_song_list(list);
}

static void apply_custom_css(void) {
  GtkCssProvider *provider = gtk_css_provider_new();

  const char *css = "window, dialog, label, entry, textview, dropdown{ "
                    "   font-size: 14pt; "
                    "} "
                    "dropdown.velky-dropdown { "
                    "   font-size: 15pt; "
                    "   font-weight: bold; "
                    "   padding: 8px 12px; "
                    "} "
                    "listbox row { "
                    "   padding: 6px 10px; "
                    "} "

                    ".error-label { "
                    "   color: #e74c3c; "
                    "   font-weight: bold; "
                    "   font-size: 14pt; "
                    "   margin-top: 5px; "
                    "   margin-bottom: 5px; "
                    "}";

  gtk_css_provider_load_from_string(provider, css);

  // Aplikovanie CSS na výpredvolený displej
  GdkDisplay *display = gdk_display_get_default();
  if (display != NULL) {
    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  g_object_unref(provider);
}

static void vypln_kategorie(GtkWidget *list_kategorii, int mode) {
  if (list_kategorii == NULL) {
    return;
  }
  gtk_list_box_remove_all(GTK_LIST_BOX(list_kategorii));
  const char **kategorie =
      (mode == MODE_JKS_INDEX) ? JKS_KATEGORIE : OSTATNE_KATEGORIE;

  for (int i = 0; kategorie[i] != NULL; i++) {
    GtkWidget *label = gtk_label_new(kategorie[i]);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);

    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);

    if (mode == MODE_JKS_INDEX) {
      g_object_set_data(G_OBJECT(row), ROW_JKS, GINT_TO_POINTER(1));
      g_object_set_data(G_OBJECT(row), ROW_ENUM_VAL, GINT_TO_POINTER(i));
    } else {
      g_object_set_data(G_OBJECT(row), ROW_JKS, GINT_TO_POINTER(0));
      g_object_set_data(G_OBJECT(row), ROW_ENUM_VAL, GINT_TO_POINTER(i + 1));
    }
    gtk_list_box_append(GTK_LIST_BOX(list_kategorii), row);
  }
}

static void on_mode_changed(GtkDropDown *dropdown, GParamSpec *pspec,
                            gpointer user_data) {
  GtkWidget *categories_list = GTK_WIDGET(user_data);
  guint selected = gtk_drop_down_get_selected(dropdown);
  vypln_kategorie(categories_list, (int)selected);
}

void build_main_ui(GtkApplication *app, gpointer user_data) {
  apply_custom_css();
  sqlite3 *db = (sqlite3 *)user_data;

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Premietač Dominikani");
  gtk_window_maximize(GTK_WINDOW(window));

  // Rozdelenie do viacerých okien
  // main
  GtkWidget *box_main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_set_margin_start(box_main, 10);
  gtk_widget_set_margin_end(box_main, 10);
  gtk_widget_set_margin_top(box_main, 10);
  gtk_widget_set_margin_bottom(box_main, 10);
  gtk_window_set_child(GTK_WINDOW(window), box_main);

  // stlpec pre tlacidla vkladame z prava
  GtkWidget *stlpec_tlacidla = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

  GtkWidget *btn_premietac = gtk_button_new_with_label("START premietanie");
  GtkWidget *btn_pridaj_piesen = gtk_button_new_with_label("Pridaj pieseň");
  GtkWidget *btn_odstran_piesen = gtk_button_new_with_label("Odstran pieseň");
  GtkWidget *btn_uprav_piesen = gtk_button_new_with_label("Uprav pieseň");

  gtk_box_append(GTK_BOX(stlpec_tlacidla), btn_premietac);
  gtk_box_append(GTK_BOX(stlpec_tlacidla), btn_uprav_piesen);
  gtk_box_append(GTK_BOX(stlpec_tlacidla), btn_odstran_piesen);
  gtk_box_append(GTK_BOX(stlpec_tlacidla), btn_pridaj_piesen);

  // graficke oddelenie vyhladavanie od tlacidiel
  GtkWidget *oddelovac = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_margin_top(oddelovac, 100);
  gtk_widget_set_margin_bottom(oddelovac, 100);
  gtk_box_append(GTK_BOX(stlpec_tlacidla), oddelovac);
  // vyhladavanie tlacidlo a box
  GtkWidget *entry_search = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_search),
                                 "Hľadať názov / číslo");

  // tlacidla na hladanie
  GtkWidget *btn_hladaj = gtk_button_new_with_label("Hľadaj");
  GtkWidget *btn_vycisti = gtk_button_new_with_label("Vyčisti");

  gtk_box_append(GTK_BOX(stlpec_tlacidla), entry_search);
  gtk_box_append(GTK_BOX(stlpec_tlacidla), btn_hladaj);
  gtk_box_append(GTK_BOX(stlpec_tlacidla), btn_vycisti);

  gtk_box_append(GTK_BOX(box_main), stlpec_tlacidla);

  // prepojenie tlacidiel
  g_signal_connect(btn_pridaj_piesen, "clicked",
                   G_CALLBACK(on_btn_add_song_clicked), db);

  // druhy stlpec zoznam kategorii piesni ###############################
  GtkWidget *stlpec_kategorie = gtk_scrolled_window_new();
  gtk_widget_set_size_request(stlpec_kategorie, 300, -1);
  gtk_widget_set_vexpand(stlpec_kategorie, TRUE);

  // nadpis nastavenie zvyraznenia
  GtkWidget *box_kategorie = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *title_kategorie_box = gtk_label_new("<b>TYPY PIESNÍ</b>");
  gtk_label_set_use_markup(GTK_LABEL(title_kategorie_box), TRUE);
  gtk_box_append(GTK_BOX(box_kategorie), title_kategorie_box);

  // prepnutie kategorii
  GtkWidget *dropdown_mode = gtk_drop_down_new_from_strings(SPEVNIK_MODES);
  gtk_widget_add_css_class(dropdown_mode, "velky-dropdown");
  gtk_widget_set_margin_start(dropdown_mode, 5);
  gtk_widget_set_margin_end(dropdown_mode, 5);
  gtk_box_append(GTK_BOX(box_kategorie), dropdown_mode);

  // list box setup na prezeranie
  GtkWidget *categories_list = gtk_list_box_new();
  gtk_box_append(GTK_BOX(box_kategorie), categories_list);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stlpec_kategorie),
                                box_kategorie);

  // NAstavenie prepnutia kategorii na selected
  vypln_kategorie(categories_list, MODE_JKS_INDEX);
  g_signal_connect(dropdown_mode, "notify::selected",
                   G_CALLBACK(on_mode_changed), categories_list);
  gtk_box_append(GTK_BOX(box_main), stlpec_kategorie);

  // stlpec piesne v kategorii ##################
  GtkWidget *stlpec_piesne = gtk_scrolled_window_new();
  gtk_widget_set_hexpand(stlpec_piesne, TRUE);
  gtk_widget_set_vexpand(stlpec_piesne, TRUE);

  // nastavenie labelu pre stlpec
  GtkWidget *box_piesne = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *title_piesne = gtk_label_new("<b>PIESNE V KATEGÓRII</b>");
  gtk_label_set_use_markup(GTK_LABEL(title_piesne), TRUE);
  gtk_box_append(GTK_BOX(box_piesne), title_piesne);

  GtkWidget *song_list = gtk_list_box_new();
  gtk_box_append(GTK_BOX(box_piesne), song_list);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stlpec_piesne), box_piesne);
  gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(song_list), FALSE);
  // prepojenie na reagovanie
  SelectedKategoriaData *cat_data = g_new(SelectedKategoriaData, 1);
  cat_data->song_list = song_list;
  cat_data->db = db;

  g_signal_connect(categories_list, "row-selected",
                   G_CALLBACK(on_category_selected), cat_data);

  // doplnujuce prepojenie pre tlacidla
  EditSongData *edit_data = g_new(EditSongData, 1);
  edit_data->song_list = song_list;
  edit_data->db = db;

  g_signal_connect(btn_uprav_piesen, "clicked",
                   G_CALLBACK(on_btn_edit_song_clicked), edit_data);

  DeleteSongData *del_data = g_new(DeleteSongData, 1);
  del_data->song_list = song_list;
  del_data->db = db;

  g_signal_connect(btn_odstran_piesen, "clicked",
                   G_CALLBACK(on_btn_delete_song_clicked), del_data);

  gtk_box_append(GTK_BOX(box_main), stlpec_piesne);

  // stlpec pre vybrate piesne na premietanie #################################
  GtkWidget *stlpec_vybrate = gtk_scrolled_window_new();
  gtk_widget_set_size_request(stlpec_vybrate, 280, -1);
  gtk_widget_set_vexpand(stlpec_vybrate, TRUE);

  // nastavenie labelu
  GtkWidget *box_vybrate = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *label_vybrate = gtk_label_new("<b>VYBRANÉ PIESNE</b>");
  gtk_label_set_use_markup(GTK_LABEL(label_vybrate), TRUE);
  gtk_box_append(GTK_BOX(box_vybrate), label_vybrate);

  // pridanie zoznamu na scrolovanie
  GtkWidget *vybrane_list = gtk_list_box_new();
  gtk_box_append(GTK_BOX(box_vybrate), vybrane_list);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stlpec_vybrate),
                                box_vybrate);

  // PREPojenie na výber piestne na premietanie
  gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(vybrane_list), FALSE);
  g_signal_connect(song_list, "row-activated",
                   G_CALLBACK(on_song_double_clicked_add), vybrane_list);

  g_signal_connect(vybrane_list, "row-activated",
                   G_CALLBACK(on_song_double_clicked_remove), NULL);
  gtk_box_append(GTK_BOX(box_main), stlpec_vybrate);

  // search bar nastavenie
  SearchData *search_data = g_new(SearchData, 1);
  search_data->entry_search = entry_search;
  search_data->song_list = song_list;
  search_data->db = db;

  g_signal_connect(btn_hladaj, "clicked", G_CALLBACK(on_search_clicked),
                   search_data);
  g_signal_connect(entry_search, "activate", G_CALLBACK(on_search_clicked),
                   search_data);
  g_signal_connect(btn_vycisti, "clicked", G_CALLBACK(on_clear_search_clicked),
                   search_data);

  gtk_window_present(GTK_WINDOW(window));
}