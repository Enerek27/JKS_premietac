#ifndef BUTTONS_REACT
#define BUTTONS_REACT


#include "db.h"
#include "widget_structs.h"
#include <gtk/gtk.h>
#include <gtk/gtkshortcut.h>

void on_btn_delete_song_clicked(GtkWidget *button, gpointer user_data);

void on_clear_search_clicked(GtkWidget *widget, gpointer user_data);

void on_save_edit_song_clicked(GtkWidget *button, gpointer user_data);

void on_btn_edit_song_clicked(GtkWidget *button, gpointer user_data);

void on_confirm_delete_clicked(GtkWidget *button, gpointer user_data);

void on_add_mode_changed(GtkDropDown *dropdown, GParamSpec *pspec,
                         gpointer user_data);

void on_save_song_clicked(GtkWidget *button, gpointer user_data);

void on_btn_add_song_clicked(GtkWidget *button, gpointer user_data);

void on_search_clicked(GtkWidget *widget, gpointer user_data);

void on_song_double_clicked_remove(GtkListBox *box, GtkListBoxRow *row,
                                   gpointer user_data);

void on_song_double_clicked_add(GtkListBox *box, GtkListBoxRow *row,
                                gpointer user_data);

void on_category_selected(GtkListBox *box, GtkListBoxRow *row,
                          gpointer user_data);

void apply_custom_css(void);

void vypln_kategorie(GtkWidget *list_kategorii, int mode);

void on_mode_changed(GtkDropDown *dropdown, GParamSpec *pspec,
                     gpointer user_data);

#endif