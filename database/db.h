// tools to work with database
#ifndef DB_h
#define DB_h
#include "sqlite3.h"
#include <stdbool.h>
#include <stddef.h>

// TODO upravil som celý sql treba zmenit vsetky funkcie upravit aby isli

#define MAX_TITLE 500

typedef enum {
  JKS,
  ANTIFONA_SURIN,
  ANTIFONA,
  TAIZE,
  MLADEZNICKA,
  HYMNA,
  ZALM,
  RESPONZ,
  LATINSKE,
  POHREBNE,
  SVADOBNE,
  OSTATNE
} song_type_fake_t;

typedef enum {
  ADVENT,
  VIANOCE,
  KNAJSVMENUJEZISOVMU,
  POST,
  VELKANOC,
  NANEBOVSTUPENIEPANA,
  NADUCHASVATEHO,
  NANAJSVATEJSIUTROJICU,
  NABOZIETELO,
  KSRDCUJEZISOVMU,
  KSVATEJOMSI,
  KNAJSVATATESVIATOSTIOLTARNEJ,
  LITANIE,
  ANTIFONAKPM,
  KPANNEMARII,
  KSVATYM,
  ZAZOMRELYCH,
  KAJUCE,
  PIESNEPRILEZITOSTNE,
  PREDPOZEHNANIM
} jks_type_t;

typedef struct {
  song_type_fake_t kind;
  union {
    jks_type_t jks_typ;
  } data;

} song_type_t;

typedef struct {
  int id;
  int song_real_id;
  song_type_t song_type;
  int verse_number;
  char *text;
} verse_t;

typedef struct {
  int database_id;
  int real_id;
  song_type_t song_type;
  char title[MAX_TITLE];
} song_t;

typedef struct {
  song_t *songs;
  int count;
} song_list_t;

song_t *get_song_by_database_id(sqlite3 *db, int database_id);

bool update_song_verses(sqlite3 *db, int real_id, song_type_t song_type,
                        const char *full_text);

char *get_song_verses_text(sqlite3 *db, int real_id, song_type_t song_type);

bool save_song_with_verses(sqlite3 *db, song_t *song, const char *full_text);

bool check_song_id_exists(sqlite3 *db, song_type_t song_type, int real_id);

song_list_t *search_songs(sqlite3 *db, const char *query);

void remove_diacritics(const char *src, char *dest, size_t dest_size);

bool add_song(sqlite3 *db, song_t *song_to_add);

song_t *get_song_by_id(sqlite3 *db, int real_id, song_type_t song_type);

song_list_t *get_songs_by_song_type(sqlite3 *db, song_type_t song_type);

bool add_verse(sqlite3 *db, int real_id, song_type_t song_type,
               int verse_number, const char *text);

bool delete_verse_by_id(sqlite3 *db, int verse_id);

verse_t **get_verses_for_song(sqlite3 *db, int real_id, song_type_t song_type,
                              int *count);

const char *song_type_get_text(song_type_t song_type);
song_type_t song_type_from_text(const char *text);

bool delete_song(sqlite3 *db, int database_id);

bool update_song(sqlite3 *db, song_t *song);

bool load_database(sqlite3 **db);

void close_database(sqlite3 *db);

bool create_tables(sqlite3 *db);

void free_verses(verse_t **verses, int count);

void free_song_list(song_list_t *list);
#endif