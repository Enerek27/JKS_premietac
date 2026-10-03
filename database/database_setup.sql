
PRAGMA foreign_keys = ON;


CREATE TABLE IF NOT EXISTS songs (
    database_id INTEGER PRIMARY KEY AUTOINCREMENT,
    real_id INTEGER NOT NULL,
    typ_piesne TEXT NOT NULL,
    title TEXT NOT NULL,
    UNIQUE (real_id, typ_piesne)

);


CREATE TABLE IF NOT EXISTS verses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    song_real_id INTEGER NOT NULL,
    song_typ_piesne TEXT NOT NULL,
    verse_number INTEGER NOT NULL,
    text TEXT NOT NULL,
    
    FOREIGN KEY (song_real_id, song_typ_piesne) REFERENCES songs(real_id, typ_piesne) ON DELETE CASCADE
);