
#include "db.h"

#include <ctype.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void remove_diacritics(const char *src, char *dest, size_t dest_size) {
  size_t j = 0;
  for (size_t i = 0; src[i] != '\0' && j < dest_size - 1; i++) {
    unsigned char c = (unsigned char)src[i];

    if ((c == 0xC3 || c == 0xC4 || c == 0xC5) && src[i + 1] != '\0') {
      unsigned char next = (unsigned char)src[i + 1];

      if (c == 0xC3) {
        switch (next) {
        case 0x81:
        case 0xA1:
        case 0x84:
        case 0xA4:
          dest[j++] = 'a';
          break;
        case 0x89:
        case 0xA9:
          dest[j++] = 'e';
          break;
        case 0x8D:
        case 0xAD:
          dest[j++] = 'i';
          break;
        case 0x93:
        case 0xB3:
        case 0x94:
        case 0xB4:
          dest[j++] = 'o';
          break;
        case 0x9A:
        case 0xBA:
          dest[j++] = 'u';
          break;
        case 0x9D:
        case 0xBD:
          dest[j++] = 'y';
          break;
        default:
          dest[j++] = ' ';
          break;
        }
      } else if (c == 0xC4) {
        switch (next) {
        case 0x8C:
        case 0x8D:
          dest[j++] = 'c';
          break;
        case 0x8E:
        case 0x8F:
          dest[j++] = 'd';
          break;
        case 0xB5:
        case 0xB6:
        case 0xB9:
        case 0xBA:
          dest[j++] = 'l';
          break;
        case 0x87:
        case 0x88:
          dest[j++] = 'n';
          break;
        default:
          dest[j++] = ' ';
          break;
        }
      } else if (c == 0xC5) {
        switch (next) {
        case 0x94:
        case 0x95:
          dest[j++] = 'r';
          break;
        case 0xA0:
        case 0xA1:
          dest[j++] = 's';
          break;
        case 0xAB:
        case 0xAC:
          dest[j++] = 't';
          break;
        case 0xBD:
        case 0xBE:
          dest[j++] = 'z';
          break;
        default:
          dest[j++] = ' ';
          break;
        }
      }
    } else {

      dest[j++] = (char)tolower(c);
    }
  }
  dest[j] = '\0';
}

song_list_t *search_songs(sqlite3 *db, const char *query) {
  if (db == NULL || query == NULL || strlen(query) == 0) {
    return NULL;
  }

  char clean_query[MAX_TITLE];
  remove_diacritics(query, clean_query, sizeof(clean_query));

  sqlite3_stmt *stmt = NULL;
  const char *sql =
      "SELECT database_id, real_id, title FROM songs ORDER BY real_id ASC;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in prepare (search_songs): %s\n", sqlite3_errmsg(db));
    return NULL;
  }

  int capacity = 10;
  int count = 0;
  song_t *songs = malloc(capacity * sizeof(song_t));
  if (songs == NULL) {
    sqlite3_finalize(stmt);
    return NULL;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int db_id = sqlite3_column_int(stmt, 0);
    int real_id = sqlite3_column_int(stmt, 1);
    const char *title_text = (const char *)sqlite3_column_text(stmt, 2);
    if (title_text == NULL) {
      title_text = "";
    }
    char real_id_str[32];
    snprintf(real_id_str, sizeof(real_id_str), "%d", real_id);

    char clean_title[MAX_TITLE];
    remove_diacritics(title_text, clean_title, sizeof(clean_title));

    if (strcmp(real_id_str, query) == 0 ||
        strstr(clean_title, clean_query) != NULL) {

      if (count >= capacity) {
        capacity += 10;
        song_t *temp = realloc(songs, capacity * sizeof(song_t));
        if (temp == NULL) {
          break;
        }
        songs = temp;
      }
      songs[count].database_id = db_id;
      songs[count].real_id = real_id;
      snprintf(songs[count].title, sizeof(songs[count].title), "%s",
               title_text);
      count++;
    }
  }
  sqlite3_finalize(stmt);

  song_list_t *result = malloc(sizeof(song_list_t));
  if (result == NULL) {
    free(songs);
    return NULL;
  }

  result->count = count;
  result->songs = songs;
  return result;
}

bool add_song(sqlite3 *db, song_t *song_to_add) {
  if (db == NULL || song_to_add == NULL) {
    return false;
  }

  const char *sql =
      "INSERT INTO songs (real_id, typ_piesne, title) VALUES (?,?,?);";
  sqlite3_stmt *stmt = NULL;

  int status = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (status != SQLITE_OK) {
    printf("ERROR in preparing (add_song): %s\n", sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, song_to_add->real_id);
  sqlite3_bind_text(stmt, 2, song_type_get_text(song_to_add->song_type), -1,
                    SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, song_to_add->title, -1, SQLITE_STATIC);

  int step_result = sqlite3_step(stmt);

  if (step_result != SQLITE_DONE) {
    printf("ERROR in step (add_song): %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return false;
  }

  song_to_add->database_id = (int)sqlite3_last_insert_rowid(db);

  sqlite3_finalize(stmt);

  return true;
}

song_t *get_song_by_id(sqlite3 *db, int real_id, song_type_t song_type) {

  if (db == NULL) {
    return NULL;
  }

  const char *sql = "SELECT database_id, real_id, typ_piesne, title FROM songs "
                    "WHERE real_id = ? AND typ_piesne = ?;";
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in preparing (get_song_by_id): %s\n", sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_int(stmt, 1, real_id);
  sqlite3_bind_text(stmt, 2, song_type_get_text(song_type), -1, SQLITE_STATIC);

  song_t *song = NULL;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    song = malloc(sizeof(song_t));
    if (song != NULL) {
      song->database_id = sqlite3_column_int(stmt, 0);
      song->real_id = sqlite3_column_int(stmt, 1);
      song->song_type = song_type;
      const unsigned char *tmp = sqlite3_column_text(stmt, 3);
      if (tmp != NULL) {
        snprintf(song->title, sizeof(song->title), "%s", (const char *)tmp);
      } else {
        song->title[0] = '\0';
      }
    }
  }

  sqlite3_finalize(stmt);

  return song;
}
song_list_t *get_songs_by_song_type(sqlite3 *db, song_type_t song_type) {
  if (db == NULL) {
    return NULL;
  }

  const char *typ_str = song_type_get_text(song_type);
  if (typ_str == NULL) {
    return NULL;
  }

  sqlite3_stmt *stmt = NULL;
  const char *sql = "SELECT database_id, real_id, title FROM songs WHERE "
                    "typ_piesne = ? ORDER BY real_id ASC;";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in prepare (get_songs_by_song_type): %s\n",
           sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_text(stmt, 1, typ_str, -1, SQLITE_STATIC);

  int capacity = 10;
  int count = 0;

  song_t *songs = malloc(capacity * sizeof(song_t));

  if (songs == NULL) {
    printf("Nedostatok miesta na alokaciu (get_songs_by_song_type)\n");
    sqlite3_finalize(stmt);
    return NULL;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    if (count >= capacity) {
      capacity += 10;
      song_t *temp = realloc(songs, capacity * sizeof(song_t));
      if (temp == NULL) {
        break;
      }
      songs = temp;
    }
    songs[count].database_id = sqlite3_column_int(stmt, 0);
    songs[count].real_id = sqlite3_column_int(stmt, 1);
    songs[count].song_type = song_type;

    const char *title_text = (const char *)sqlite3_column_text(stmt, 2);
    if (title_text != NULL) {
      snprintf(songs[count].title, sizeof(songs[count].title), "%s",
               (const char *)title_text);
    } else {
      songs[count].title[0] = '\0';
    }
    count++;
  }
  sqlite3_finalize(stmt);

  song_list_t *result = malloc(sizeof(song_list_t));
  if (result == NULL) {
    free(songs);
    return NULL;
  }

  result->songs = songs;
  result->count = count;

  return result;
}

song_type_t song_type_from_text(const char *text) {
  song_type_t st;
  if (text == NULL) {
    st.kind = OSTATNE;
    return st;
  }

  if (strcmp(text, "Advent") == 0) {
    st.kind = JKS;
    st.data.jks_typ = ADVENT;
    return st;
  }
  if (strcmp(text, "Vianoce") == 0) {
    st.kind = JKS;
    st.data.jks_typ = VIANOCE;
    return st;
  }
  if (strcmp(text, "K Najsvätejšiemu menu Ježiš") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KNAJSVMENUJEZISOVMU;
    return st;
  }
  if (strcmp(text, "Pôst") == 0) {
    st.kind = JKS;
    st.data.jks_typ = POST;
    return st;
  }
  if (strcmp(text, "Veľká Noc") == 0) {
    st.kind = JKS;
    st.data.jks_typ = VELKANOC;
    return st;
  }
  if (strcmp(text, "Nanebovstúpenie Pána") == 0) {
    st.kind = JKS;
    st.data.jks_typ = NANEBOVSTUPENIEPANA;
    return st;
  }
  if (strcmp(text, "Na Ducha Svätého") == 0) {
    st.kind = JKS;
    st.data.jks_typ = NADUCHASVATEHO;
    return st;
  }
  if (strcmp(text, "Na Najsvätejšiu Trojicu") == 0) {
    st.kind = JKS;
    st.data.jks_typ = NANAJSVATEJSIUTROJICU;
    return st;
  }
  if (strcmp(text, "Na Božie Telo") == 0) {
    st.kind = JKS;
    st.data.jks_typ = NABOZIETELO;
    return st;
  }
  if (strcmp(text, "K Najsvätejšiemu Srdcu Ježišovmu") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KSRDCUJEZISOVMU;
    return st;
  }
  if (strcmp(text, "K Svätej Omši") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KSVATEJOMSI;
    return st;
  }
  if (strcmp(text, "K Najsvätejšej Sviatosti Oltárnej") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KNAJSVATATESVIATOSTIOLTARNEJ;
    return st;
  }
  if (strcmp(text, "Litánie") == 0) {
    st.kind = JKS;
    st.data.jks_typ = LITANIE;
    return st;
  }

  if (strcmp(text, "Antifona k Panne Márii") == 0) {
    st.kind = JKS;
    st.data.jks_typ = ANTIFONAKPM;
    return st;
  }

  if (strcmp(text, "K Panne Márii") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KPANNEMARII;
    return st;
  }
  if (strcmp(text, "Ku Svätým") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KSVATYM;
    return st;
  }
  if (strcmp(text, "Za Zomrelých") == 0) {
    st.kind = JKS;
    st.data.jks_typ = ZAZOMRELYCH;
    return st;
  }
  if (strcmp(text, "Kajúce") == 0) {
    st.kind = JKS;
    st.data.jks_typ = KAJUCE;
    return st;
  }
  if (strcmp(text, "Piesne Príležitostné") == 0) {
    st.kind = JKS;
    st.data.jks_typ = PIESNEPRILEZITOSTNE;
    return st;
  }
  if (strcmp(text, "Pred Požehnaním") == 0) {
    st.kind = JKS;
    st.data.jks_typ = PREDPOZEHNANIM;
    return st;
  }
  // toto uz su dalsie ####################x
  if (strcmp(text, "Antifona") == 0) {
    st.kind = ANTIFONA;
    return st;
  }
  if (strcmp(text, "Antifona Šurin") == 0) {
    st.kind = ANTIFONA_SURIN;
    return st;
  }
  if (strcmp(text, "Taize") == 0) {
    st.kind = TAIZE;
    return st;
  }
  if (strcmp(text, "Mladežnícka") == 0) {
    st.kind = MLADEZNICKA;
    return st;
  }
  if (strcmp(text, "Hymna") == 0) {
    st.kind = HYMNA;
    return st;
  }
  if (strcmp(text, "Žalm") == 0) {
    st.kind = ZALM;
    return st;
  }
  if (strcmp(text, "Responz") == 0) {
    st.kind = RESPONZ;
    return st;
  }
  if (strcmp(text, "Latinské") == 0) {
    st.kind = LATINSKE;
    return st;
  }
  if (strcmp(text, "Pohrebné") == 0) {
    st.kind = POHREBNE;
    return st;
  }
  if (strcmp(text, "Svadobné") == 0) {
    st.kind = SVADOBNE;
    return st;
  }

  st.kind = OSTATNE;
  return st;
}

const char *song_type_get_text(song_type_t song_type) {
  if (song_type.kind == JKS) {
    switch (song_type.data.jks_typ) {
    case ADVENT:
      return "Advent";
      break;
    case VIANOCE:
      return "Vianoce";
      break;
    case KNAJSVMENUJEZISOVMU:
      return "K Najsvätejšiemu menu Ježiš";
      break;
    case POST:
      return "Pôst";
      break;
    case VELKANOC:
      return "Veľká Noc";
      break;
    case NANEBOVSTUPENIEPANA:
      return "Nanebovstúpenie Pána";
      break;
    case NADUCHASVATEHO:
      return "Na Ducha Svätého";
      break;
    case NANAJSVATEJSIUTROJICU:
      return "Na Najsvätejšiu Trojicu";
      break;
    case NABOZIETELO:
      return "Na Božie Telo";
      break;
    case KSRDCUJEZISOVMU:
      return "K Najsvätejšiemu Srdcu Ježišovmu";
      break;
    case KSVATEJOMSI:
      return "K Svätej Omši";
      break;
    case KNAJSVATATESVIATOSTIOLTARNEJ:
      return "K Najsvätejšej Sviatosti Oltárnej";
      break;
    case LITANIE:
      return "Litánie";
      break;

    case ANTIFONAKPM:
      return "Antifona k Panne Márii";
      break;
    case KPANNEMARII:
      return "K Panne Márii";
      break;
    case KSVATYM:
      return "Ku Svätým";
      break;
    case ZAZOMRELYCH:
      return "Za Zomrelých";
      break;
    case KAJUCE:
      return "Kajúce";
      break;
    case PIESNEPRILEZITOSTNE:
      return "Piesne Príležitostné";
      break;
    case PREDPOZEHNANIM:
      return "Pred Požehnaním";
      break;
    }
  } else {
    switch (song_type.kind) {
    case ANTIFONA:
      return "Antifona";
      break;
    case ANTIFONA_SURIN:
      return "Antifona Šurin";
      break;
    case TAIZE:
      return "Taize";
      break;
    case MLADEZNICKA:
      return "Mladežnícka";
      break;
    case HYMNA:
      return "Hymna";
      break;
    case ZALM:
      return "Žalm";
      break;
    case RESPONZ:
      return "Responz";
      break;
    case LATINSKE:
      return "Latinské";
      break;
    case POHREBNE:
      return "Pohrebné";
      break;
    case SVADOBNE:
      return "Svadobné";
      break;
    case OSTATNE:
      return "Ostatné";
      break;
    }
  }
  printf("Nenasiel som taky (song_type_get_text)\n");
  return "Ostatné";
}

bool load_database(sqlite3 **db) {

  int result = sqlite3_open("songs.db", db);
  if (result != SQLITE_OK) {

    printf("Error loading database: %s\n", sqlite3_errmsg(*db));
    sqlite3_close(*db);
    *db = NULL;
    return false;
  }
  sqlite3_exec(*db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
  printf("Database loaded\n");

  return true;
}

void close_database(sqlite3 *db) {
  if (db != NULL) {
    sqlite3_close(db);
    printf("Database closed\n");
  }
}

bool create_tables(sqlite3 *db) {
  FILE *file = fopen("database/database_setup.sql", "r");
  if (file == NULL) {
    printf("Error opening database setup \n");
    return false;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  char *sql = malloc(size + 1);
  if (sql == NULL) {
    fclose(file);
    printf("Error allocating memory\n");
    return false;
  }

  size_t read_bytes = fread(sql, 1, size, file);
  sql[read_bytes] = '\0';

  fclose(file);

  char *error_msg = NULL;

  int result = sqlite3_exec(db, sql, NULL, NULL, &error_msg);

  free(sql);
  if (result != SQLITE_OK) {
    printf("Database setup error: %s\n", error_msg);
    sqlite3_free(error_msg);
    return false;
  }

  printf("Database setup completed\n");

  return true;
}

bool add_verse(sqlite3 *db, int real_id, song_type_t song_type,
               int verse_number, const char *text) {
  if (db == NULL || text == NULL) {
    return false;
  }

  const char *sql = "INSERT INTO verses (song_real_id, song_typ_piesne, "
                    "verse_number, text) VALUES (?,?,?,?);";
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in preparing (add_verse):%s\n", sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, real_id);
  sqlite3_bind_text(stmt, 2, song_type_get_text(song_type), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 3, verse_number);
  sqlite3_bind_text(stmt, 4, text, -1, SQLITE_STATIC);

  int step_result = sqlite3_step(stmt);
  if (step_result != SQLITE_DONE) {
    printf("ERROR in step (add_verse):%s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}
verse_t **get_verses_for_song(sqlite3 *db, int real_id, song_type_t song_type,
                              int *count) {
  if (db == NULL || count == NULL) {
    return NULL;
  }
  *count = 0;

  const char *sql = "SELECT id, song_real_id, song_typ_piesne, verse_number, "
                    "text FROM verses "
                    "WHERE song_real_id = ? AND song_typ_piesne = ? ORDER BY "
                    "verse_number ASC;";
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in preparing (get_verses_for_song):%s\n", sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_int(stmt, 1, real_id);
  sqlite3_bind_text(stmt, 2, song_type_get_text(song_type), -1, SQLITE_STATIC);

  int capacity = 4;
  verse_t **verses = malloc(capacity * sizeof(verse_t *));
  if (verses == NULL) {
    sqlite3_finalize(stmt);
    printf("ERROR not enough space to allocate (get_verses_for_song)");
    return NULL;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    if (*count >= capacity) {
      capacity += 4;
      verse_t **temp = realloc(verses, capacity * sizeof(verse_t *));
      if (temp == NULL) {
        for (int i = 0; i < *count; i++) {
          free(verses[i]->text);
          free(verses[i]);
        }
        free(verses);
        sqlite3_finalize(stmt);
        printf("ERROR not enough space to allocate (get_verses_for_song)");
        return NULL;
      }
      verses = temp;
    }

    verse_t *v = malloc(sizeof(verse_t));
    if (v == NULL) {
      printf("ERROR not enough space to allocate (get_verses_for_song)");
      for (int i = 0; i < *count; i++) {
        free(verses[i]->text);
        free(verses[i]);
      }
      free(verses);
      sqlite3_finalize(stmt);
      return NULL;
    }
    v->id = sqlite3_column_int(stmt, 0);
    v->song_real_id = sqlite3_column_int(stmt, 1);
    v->song_type = song_type;
    v->verse_number = sqlite3_column_int(stmt, 3);
    const unsigned char *tmp_text = sqlite3_column_text(stmt, 4);
    if (tmp_text != NULL) {
      v->text = strdup((const char *)tmp_text);
    } else {
      v->text = strdup("");
    }
    verses[*count] = v;
    (*count)++;
  }
  sqlite3_finalize(stmt);
  return verses;
}
void free_verses(verse_t **verses, int count) {
  if (verses == NULL) {
    return;
  }

  for (int i = 0; i < count; i++) {
    if (verses[i] != NULL) {
      free(verses[i]->text);
      free(verses[i]);
    }
  }
  free(verses);
}

void free_song_list(song_list_t *list) {
  if (list != NULL) {
    if (list->songs != NULL) {
      free(list->songs);
    }
    free(list);
  }
}

bool delete_song(sqlite3 *db, int database_id) {
  if (db == NULL) {
    return false;
  }

  const char *sql = "DELETE FROM songs WHERE database_id = ?;";

  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in preparing (delete_song):%s\n", sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, database_id);

  int step_result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (step_result != SQLITE_DONE) {
    printf("ERROR in step (delete_song):%s\n", sqlite3_errmsg(db));
    return false;
  }

  int a = sqlite3_changes(db);

  return (a > 0);
}
bool update_song(sqlite3 *db, song_t *song) {
  if (db == NULL || song == NULL) {
    return false;
  }
  const char *sql = "UPDATE songs SET real_id = ?, typ_piesne = ?, title = ? "
                    "WHERE database_id = ?;";
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in prepare (update_song):%s\n", sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, song->real_id);
  sqlite3_bind_text(stmt, 2, song_type_get_text(song->song_type), -1,
                    SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, song->title, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 4, song->database_id);

  int step_result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (step_result != SQLITE_DONE) {
    printf("ERROR in step (update_song):%s\n", sqlite3_errmsg(db));
    return false;
  }

  return (sqlite3_changes(db) > 0);
}

bool delete_verse_by_id(sqlite3 *db, int verse_id) {
  if (db == NULL) {
    return false;
  }

  const char *sql = "DELETE FROM verses WHERE id = ?;";
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in preparing (delete_verse_by_id): %s\n", sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, verse_id);

  int step_result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (step_result != SQLITE_DONE) {
    printf("ERROR in step (delete_verse_by_id): %s\n", sqlite3_errmsg(db));
    return false;
  }

  return (sqlite3_changes(db) > 0);
}

bool check_song_id_exists(sqlite3 *db, song_type_t song_type, int real_id) {
  if (db == NULL) {
    return false;
  }

  const char *type_str = song_type_get_text(song_type);
  if (type_str == NULL) {
    return false;
  }

  sqlite3_stmt *stmt = NULL;
  const char *sql =
      "SELECT COUNT(*) FROM songs WHERE typ_piesne = ? AND real_id = ?;";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in prepare(chech_song_id_exists): %s\n", sqlite3_errmsg(db));
    return false;
  }
  sqlite3_bind_text(stmt, 1, type_str, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, real_id);

  bool exists = false;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    if (count > 0) {
      exists = true;
    }
  }

  sqlite3_finalize(stmt);
  return exists;
}

void remove_cr(char *str) {
  char *src = str, *dst = str;
  while (*src) {
    if (*src != '\r') {
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0';
}

bool save_song_with_verses(sqlite3 *db, song_t *song, const char *full_text) {
  if (db == NULL || song == NULL || full_text == NULL ||
      strlen(full_text) == 0) {
    return false;
  }

  if (!add_song(db, song)) {
    printf("ERROR in adding song (save_song_with_verses)\n");
    return false;
  }

  char *text_copy = strdup(full_text);

  if (text_copy == NULL) {
    return false;
  }
  remove_cr(text_copy);

  char current_verse[5000] = "";
  int verse_number = 1;

  char *line = text_copy;
  char *next_line = NULL;

  while (line != NULL && *line != '\0') {
    next_line = strchr(line, '\n');
    if (next_line != NULL) {
      *next_line = '\0';
      next_line++;
    }

    size_t len = strlen(line);
    bool is_empty = (len == 0 || strspn(line, " \t") == len);

    if (is_empty) {

      if (strlen(current_verse) > 0) {
        add_verse(db, song->real_id, song->song_type, verse_number,
                  current_verse);
        verse_number++;
        current_verse[0] = '\0';
      }
    } else {

      if (strlen(current_verse) > 0) {
        strcat(current_verse, "\n");
      }
      strcat(current_verse, line);
    }

    line = next_line;
  }

  if (strlen(current_verse) > 0) {
    add_verse(db, song->real_id, song->song_type, verse_number, current_verse);
  }

  free(text_copy);
  return true;
}

char *get_song_verses_text(sqlite3 *db, int real_id, song_type_t song_type) {
  if (db == NULL) {
    return NULL;
  }

  const char * typ_str = song_type_get_text(song_type);
  if (typ_str == NULL) {
    return NULL;
  }

  sqlite3_stmt * stmt = NULL;
  const char* sql = "SELECT text FROM verses WHERE song_real_id = ? AND song_typ_piesne = ? ORDER BY verse_number ASC;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR in preparing (get_song_verses_text): %s\n", sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_int(stmt, 1, real_id);
  sqlite3_bind_text(stmt, 2, typ_str, -1, SQLITE_STATIC);

  int capacity = 4096;
  char *full_text = malloc(capacity);
  if (full_text == NULL) {
    sqlite3_finalize(stmt);
    return NULL;
  }

  full_text[0] = '\0';

  bool first = true;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char * verse_text = (const char*)sqlite3_column_text(stmt, 0);
    if (verse_text != NULL) {
      size_t needed = strlen(full_text) + strlen(verse_text) + 4;
      if (needed >= capacity) {
        capacity *= 2;
        char * temp = realloc(full_text, capacity);
        if (temp == NULL) {
          break;
        }
        full_text = temp;
      }

      if (!first) {
        strcat(full_text, "\n\n");
      }
      strcat(full_text, verse_text);
      first = false;

    }
  }

  sqlite3_finalize(stmt);
  return full_text;

}

bool update_song_verses(sqlite3 *db, int real_id, song_type_t song_type, const char *full_text) {
  if (db == NULL || full_text == NULL) return false;

  const char *typ_str = song_type_get_text(song_type);

 
  sqlite3_stmt *stmt = NULL;
  const char *sql_del = "DELETE FROM verses WHERE song_real_id = ? AND song_typ_piesne = ?;";
  if (sqlite3_prepare_v2(db, sql_del, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, real_id);
    sqlite3_bind_text(stmt, 2, typ_str, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }

  
  char *text_copy = strdup(full_text);
  if (!text_copy) return false;

  remove_cr(text_copy);

  char current_verse[5000] = "";
  int verse_number = 1;
  char *line = text_copy;
  char *next_line = NULL;

  while (line != NULL && *line != '\0') {
    next_line = strchr(line, '\n');
    if (next_line != NULL) {
      *next_line = '\0';
      next_line++;
    }

    size_t len = strlen(line);
    bool is_empty = (len == 0 || strspn(line, " \t") == len);

    if (is_empty) {
      if (strlen(current_verse) > 0) {
        add_verse(db, real_id, song_type, verse_number, current_verse);
        verse_number++;
        current_verse[0] = '\0';
      }
    } else {
      if (strlen(current_verse) > 0) {
        strcat(current_verse, "\n");
      }
      strcat(current_verse, line);
    }

    line = next_line;
  }

  if (strlen(current_verse) > 0) {
    add_verse(db, real_id, song_type, verse_number, current_verse);
  }

  free(text_copy);
  return true;
}

song_t *get_song_by_database_id(sqlite3 *db, int database_id) {
  if (db == NULL || database_id <= 0) return NULL;

  const char *sql = "SELECT database_id, real_id, title, typ_piesne FROM songs WHERE database_id = ?;";
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("ERROR (get_song_by_database_id): %s\n", sqlite3_errmsg(db));
    return NULL;
  }

  sqlite3_bind_int(stmt, 1, database_id);

  song_t *song = NULL;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    song = malloc(sizeof(song_t));
    if (song != NULL) {
      memset(song, 0, sizeof(song_t));
      song->database_id = sqlite3_column_int(stmt, 0);
      song->real_id = sqlite3_column_int(stmt, 1);

      const char *title = (const char *)sqlite3_column_text(stmt, 2);
      if (title != NULL) {
        snprintf(song->title, sizeof(song->title), "%s", title);
      }

      const char *typ_str = (const char *)sqlite3_column_text(stmt, 3);
      if (typ_str != NULL) {
        song->song_type = song_type_from_text(typ_str);
      }
    }
  }

  sqlite3_finalize(stmt);
  return song;
}