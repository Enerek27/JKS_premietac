#ifndef WIDGET_H
#define WIDGET_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "db.h"

#define SPEVNIK_MODES (const char*[]) {"JKS Spevník", "Ostatné Spevníky", NULL}

static const char *JKS_KATEGORIE[] = {
    "Advent",
    "Vianoce",
    "K Najsvätejšiemu menu Ježiš",
    "Pôst",
    "Veľká noc",
    "Nanebovstúpenie Pána",
    "Na Ducha Svätého",
    "Na Najsvätejšiu Trojicu",
    "Na Božie Telo",
    "K Srdcu Ježišovmu",
    "K Svätej Omši",
    "K Najsvätejšej Sviatosti Oltárnej",
    "Litánie",
    "Antifóna k Panne Márii",
    "K Panne Márii",
    "Ku Svätým",
    "Za zomrelých",
    "Kajúce",
    "Piesne príležitostné",
    "Pred požehnaním",
    NULL
};

static const char *OSTATNE_KATEGORIE[] = {
    "Antifóna Šurín",
    "Antifóna",
    "Taizé",
    "Mládežnícka",
    "Hymna",
    "Žalm",
    "Responzórium",
    "Latinské",
    "Pohrebné",
    "Svadobné",
    "Ostatné",
    NULL
};

typedef struct {
  GtkWidget *dialog;
  GtkWidget *dropdown_mode;    // JKS / Ostatné
  GtkWidget *dropdown_subcat;  // Konkrétna kategória
  GtkWidget *entry_id;        // Číslo piesne
  GtkWidget *entry_title;     // Názov piesne
  GtkTextBuffer *text_buffer; // buffer pre text piesne
  GtkWidget *lbl_error;       // Chybová hláska (napr. "ID už existuje!")
  sqlite3 *db;
} AddSongDialogData;

typedef struct {
    GtkWidget *song_list;
    sqlite3 *db;
} SelectedKategoriaData;

typedef struct {
    GtkWidget *entry_search;
    GtkWidget *song_list;
    sqlite3 *db;
} SearchData;

typedef struct {
  GtkWidget *dialog;
  GtkListBox *song_list;
  GtkListBoxRow *selected_row;
  int database_id;
  sqlite3 *db;
} DeleteSongDialogData;

typedef struct {
  GtkWidget *song_list;
  sqlite3 *db;
} DeleteSongData;

typedef struct {
  GtkWidget *dialog;
  GtkTextBuffer *text_buffer;
  GtkWidget *lbl_error;
  int real_id;
  song_type_t song_type;
  sqlite3 *db;
} EditSongDialogData;

typedef struct {
  GtkWidget *song_list;
  sqlite3 *db;
} EditSongData;


void build_main_ui(GtkApplication *app, gpointer user_data);


#endif 