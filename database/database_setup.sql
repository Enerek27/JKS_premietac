
PRAGMA foreign_keys = ON;


CREATE TABLE IF NOT EXISTS songs (
    database_id INTEGER PRIMARY KEY AUTOINCREMENT,
    real_id INTEGER NOT NULL,
    title TEXT NOT NULL

);


CREATE TABLE IF NOT EXISTS verses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    song_id INTEGER NOT NULL,
    verse_number INTEGER NOT NULL,
    text TEXT NOT NULL,
    
    FOREIGN KEY (song_id) REFERENCES songs(database_id)
);