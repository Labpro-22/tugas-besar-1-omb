# Nimonspoli  **IF2010 Pemrograman Berorientasi Objek — Tugas Besar 1**

---

## Deskripsi

Nimonspoli adalah permainan papan (board game) bertema monopoli yang memungkinkan 2–4 pemain untuk saling berkompetisi dalam membeli properti, membayar sewa, mengikuti lelang, dan mengelola kekayaan. Permainan ini mensimulasikan seluruh mekanisme permainan monopoli, termasuk kartu kemampuan spesial, efek festival, sistem gadai, dan mekanisme kebangkrutan.

---

## Fitur Utama

- **Papan 40 petak** dengan properti, stasiun, utilitas, dan petak aksi
- **2–4 pemain** dengan urutan giliran acak
- **Sistem dadu** (random & manual via `ATUR_DADU`)
- **Kepemilikan properti**: Street (beli/lelang), Railroad & Utility (otomatis)
- **Pembangunan rumah & hotel** dengan aturan pemerataan color group
- **Sistem lelang** otomatis saat pemain menolak/tidak mampu membeli
- **Pajak** (PPH dengan opsi flat/persentase, PBM flat)
- **Festival** — melipatgandakan sewa properti hingga 8x
- **6 jenis Kartu Kemampuan Spesial**: MoveCard, DiscountCard, ShieldCard, TeleportCard, LassoCard, DemolitionCard
- **Kartu Kesempatan & Dana Umum**
- **Sistem Gadai & Tebus**
- **Mekanisme kebangkrutan & likuidasi aset**
- **Save/Load** ke file teks terstruktur
- **Transaction Logger** — mencatat semua kejadian dalam permainan
- **Papan berwarna** dengan ANSI color codes

---

## Struktur Proyek

```
tugas-besar-1-omb/
├── config/                  # File konfigurasi permainan
│   ├── property.txt
│   ├── railroad.txt
│   ├── utility.txt
│   ├── tax.txt
│   ├── special.txt
│   └── misc.txt
├── include/                 # Header files
│   ├── core/                # Game logic layer
│   │   ├── bank.h
│   │   ├── board.h
│   │   ├── carddeck.h       # Generic class CardDeck<T>
│   │   ├── cards.h
│   │   ├── config.h
│   │   ├── dice.h
│   │   ├── festivaleffect.h
│   │   ├── game.h
│   │   ├── player.h
│   │   ├── property.h
│   │   ├── propertytypes.h
│   │   ├── tiles.h
│   │   ├── transactionlogger.h
│   │   └── types.h
│   ├── data/                # Data access layer
│   │   ├── configloader.h
│   │   └── savemanager.h
│   └── ui/                  # User interaction layer
│       ├── boardprinter.h
│       └── gameCLI.h
├── src/                     # Source files
│   ├── core/
│   │   ├── board.cpp
│   │   ├── cards.cpp
│   │   ├── player.cpp
│   │   ├── tiles.cpp
│   │   └── transactionlogger.cpp
│   ├── data/
│   │   ├── configloader.cpp
│   │   └── savemanager.cpp
│   ├── ui/
│   │   ├── boardprinter.cpp
│   │   └── gamecli.cpp
│   └── main.cpp
├── data/                    # Save files (runtime)
├── makefile
└── README.md
```

---

## Arsitektur (Layered Architecture)

Program mengimplementasikan **3-layer architecture**:

| Layer | Direktori | Tanggung Jawab |
|-------|-----------|----------------|
| **UI Layer** | `include/ui/`, `src/ui/` | Input/output ke terminal (`GameCLI`, `BoardPrinter`) |
| **Core Layer** | `include/core/`, `src/core/` | Aturan permainan, state management (`Game`, `Board`, `Player`, `Tile`, `Card`, `Dice`, `Bank`) |
| **Data Layer** | `include/data/`, `src/data/` | Baca/tulis file konfigurasi dan save/load (`ConfigLoader`, `SaveManager`) |

---

## Konsep OOP yang Diimplementasikan

| # | Konsep | Implementasi |
|---|--------|--------------|
| 1 | **Inheritance & Polymorphism** | `Tile` → `PropertyTile`, `GoTile`, `JailTile`, dll. `Card` → `ChanceCard`, `CommunityCard`, `SkillCard` → 6 subclass. `Property` → `Street`, `Railroad`, `Utility` |
| 2 | **Exception Handling** | Validasi uang, slot kartu, input, file I/O dengan `try/catch` dan `throw` |
| 3 | **Operator Overloading** | `Player::operator+=/-=` untuk uang, `operator</>/` untuk perbandingan kekayaan |
| 4 | **Abstract Class & Virtual Function** | `Tile::onLanded()`, `Card::execute()`, `Property::calcRent()`, `SkillCard::use()` |
| 5 | **Generic Class (Template)** | `CardDeck<T>` untuk tumpukan kartu Chance, Community, dan Skill |
| 6 | **STL** | `vector` (pemain, petak, kartu), `map` (properti, rent table, multiplier table) |

---

## Prasyarat

- **Compiler**: `g++` dengan dukungan C++17
- **OS**: Linux / WSL / macOS
- **Tools**: `make`

---

## Cara Kompilasi & Menjalankan

```bash
# Kompilasi
make

# Jalankan
make run

# Bersihkan file hasil kompilasi
make clean

# Rebuild dari awal
make rebuild
```

---

## Daftar Perintah

| Perintah | Deskripsi |
|----------|-----------|
| `CETAK_PAPAN` | Tampilkan papan permainan |
| `LEMPAR_DADU` | Lempar dadu secara acak |
| `ATUR_DADU X Y` | Atur dadu manual (1–6) |
| `CETAK_AKTA [KODE]` | Tampilkan akta kepemilikan properti |
| `CETAK_PROPERTI` | Tampilkan properti milik pemain |
| `GADAI` | Gadaikan properti |
| `TEBUS` | Tebus properti yang digadaikan |
| `BANGUN` | Bangun rumah/hotel |
| `GUNAKAN_KEMAMPUAN` | Gunakan kartu kemampuan spesial |
| `BAYAR_DENDA` | Bayar denda keluar penjara |
| `SIMPAN [FILE]` | Simpan permainan ke file |
| `MUAT [FILE]` | Muat permainan dari file |
| `CETAK_LOG [N]` | Tampilkan N log terakhir |
| `SELESAI` | Akhiri giliran |
| `BANTUAN` | Tampilkan daftar perintah |

---

## Anggota Kelompok

| NIM | Nama |
|-----|------|
| — | — |
| — | — |
| — | — |
| — | — |
| — | — |

---

*Tugas Besar 1 — IF2010 Pemrograman Berorientasi Objek — 2025/2026*
