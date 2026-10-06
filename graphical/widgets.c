#include "widgets.h"
#include "buttons_reactions.h"
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