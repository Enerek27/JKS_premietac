#include "db.h"
#include "widgets.h"
#include <gio/gio.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdbool.h>



int main(int argc, char *argv[])
{
    sqlite3 *db = NULL;

    
    if (!load_database(&db)) {
        printf("Chyba pri otváraní databázy!\n");
        return 1;
    }

    if (!create_tables(db)) {
        printf("Chyba pri vytváraní tabuliek!\n");
        close_database(db);
        return 1;
    }

    GtkApplication * app = gtk_application_new("sk.jks.spevnik", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(build_main_ui), db);
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);


    close_database(db);
   
    return status;
}