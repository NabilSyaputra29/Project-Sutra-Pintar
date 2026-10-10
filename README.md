# Project-Sutra-Pintar

**Sutra Pintar** adalah perangkat bantu ibadah berbasis **ESP32** yang membantu pengguna mengikuti urutan gerakan sholat serta **menghitung rakaat dan sujud secara otomatis**. Perangkat membaca gerakan memakai dua sensor ultrasonik dan metode **Finite State Machine (FSM)**, lalu menampilkan hasilnya di layar sentuh TFT. Untuk belajar, perangkat memutar panduan suara lewat DFPlayer Mini.

> Catatan: versi awal alat ini memakai OLED. Perangkat **v2.0** memakai **TFT LCD Touchscreen**.

## Fitur Utama

- **Mode Sholat**: mendeteksi gerakan, menghitung rakaat dan total sujud, dan menampilkan SHOLAT SELESAI di akhir. Mode ini **tanpa suara** agar tidak mengganggu kekhusyukan.
- **Mode Belajar**: bacaan tampil di layar (teks Latin) dan diputar dengan suara panduan. Pengguna memilih 2 dari 7 surah pendek. Ada animasi gerakan sholat di sisi kanan layar.
- **Pengaturan**: volume audio (0-30) dan kecerahan layar (10%-100%).
- **Bantuan**: penjelasan per slide untuk 3 topik: Cara Menggunakan, Mode Sholat, dan Mode Belajar.
- **Kalibrasi otomatis** posisi berdiri pengguna sebelum sholat dimulai.
- Pilihan sholat: Subuh (2 rakaat), Dzuhur (4), Ashar (4), Maghrib (3), dan Isya (4).

## Cara Kerja

Alat diletakkan di depan pengguna (seperti sutrah) dengan dua sensor ultrasonik menghadap tubuh:

- **Sensor atas** membaca gerakan berdiri dan ruku'.
- **Sensor bawah** membaca gerakan sujud, duduk, dan tahiyat.

Setelah kalibrasi, jarak baseline tiap sensor dicatat. Perpindahan gerakan ditentukan dari **rasio jarak terhadap baseline**:

| Perpindahan | Sensor | Syarat (rasio terhadap baseline) |
|-------------|--------|----------------------------------|
| Berdiri → Ruku' | Atas | antara 0,30 dan 0,80 |
| Ruku' → I'tidal | Atas | ≥ 0,85 |
| I'tidal → Sujud | Bawah | < 0,40 |
| Sujud → Duduk | Bawah | 0,40 sampai 0,70 |
| Duduk → Sujud ke-2 | Bawah | < 0,40 |
| Sujud ke-2 → Berdiri (rakaat berikutnya) | Bawah | ≥ 0,80 |
| Sujud ke-2 → Tahiyat (rakaat 2 / terakhir) | Bawah | ≥ 0,40 |
| Tahiyat awal → Berdiri | Bawah | ≥ 0,85 |

Urutan gerakan: **Berdiri → Ruku' → I'tidal → Sujud → Duduk → Sujud ke-2 → (Tahiyat awal) → Berdiri → ... → Tahiyat akhir → Selesai**.

Agar tidak salah hitung, ada waktu tahan minimal tiap gerakan (mis. berdiri 6 detik, ruku' 3 detik, tahiyat 5 detik) dan pembacaan sensor memakai median serta penghalusan.

### Pembacaan lebih ketat di Mode Belajar

Mode Belajar memakai ambang yang sama, tetapi dengan pengaman tambahan:

- Syarat pindah gerakan harus terpenuhi **beberapa siklus beruntun** (sekitar 0,8 detik), bukan sekali baca.
- Gerakan baru dibaca setelah **bacaan dan keterangan gerakan selesai diputar** (pin BUSY DFPlayer).
- Perubahan posisi dinilai dari **posisi diam (plateau)** sebelumnya, sehingga lebih stabil.
- Ada penanganan khusus untuk sholat Subuh agar ruku' dan sujud terakhir terbaca lebih toleran.

## Komponen

| Komponen | Keterangan |
|----------|------------|
| ESP32 | Mikrokontroler utama |
| TFT ST7789 320x240 | Layar (jalur HSPI) |
| XPT2046 | Touchscreen (jalur VSPI) |
| 2x sensor ultrasonik HC-SR04 | Sensor atas dan bawah |
| DFPlayer Mini + microSD (FAT32) + speaker | Audio panduan |

> Pin ECHO HC-SR04 berlogika 5V. Disarankan memakai pembagi tegangan (mis. 1k + 2k) agar aman untuk ESP32.

## Pin Mapping

**Layar TFT (HSPI)**

| Fungsi | GPIO |
|--------|------|
| CS | 15 |
| DC | 2 |
| RST | 4 |
| MOSI | 13 |
| CLK | 14 |
| MISO | 12 |
| LED (backlight, PWM) | 21 |

**Touchscreen XPT2046 (VSPI)**

| Fungsi | GPIO |
|--------|------|
| CS | 33 |
| IRQ | 36 |
| MOSI | 32 |
| CLK | 25 |
| MISO | 39 |

**Sensor ultrasonik**

| Sensor | TRIG | ECHO |
|--------|------|------|
| Atas | 18 | 19 |
| Bawah | 22 | 23 |

**DFPlayer Mini**

| Fungsi | GPIO |
|--------|------|
| RX ESP32 (← TX DFPlayer) | 16 |
| TX ESP32 (→ RX DFPlayer) | 17 |
| BUSY | 26 |

## Struktur Audio di SD Card

Format SD card ke **FAT32**. Nama file memakai **4 digit** (dipanggil dengan `playLargeFolder`).

**Folder `01`** (panduan sholat):

| Track | Isi |
|-------|-----|
| 0001 | Takbiratul Ihram |
| 0002 | Al-Fatihah |
| 0003 | Keterangan sebelum ruku' |
| 0004 | Doa ruku' |
| 0005 | Keterangan sebelum i'tidal |
| 0006 | Bangun dari ruku' (Sami'allahu liman hamidah) |
| 0007 | Doa i'tidal (Rabbana lakal hamd) |
| 0008 | Keterangan sebelum sujud |
| 0009 | Doa sujud |
| 0010 | Keterangan sebelum duduk di antara 2 sujud |
| 0011 | Doa duduk di antara 2 sujud |
| 0012 | Keterangan sebelum sujud kedua |
| 0013 | Keterangan sebelum tahiyat awal |
| 0014 | Doa tahiyat awal |
| 0015 | Keterangan sebelum tahiyat akhir |
| 0016 | Doa tahiyat akhir |
| 0017 | Keterangan sebelum salam |
| 0018 | Salam |
| 0019 | Keterangan penutup |

**Folder `02`** (surah pendek):

| Track | Surah |
|-------|-------|
| 0001 | Al-Ikhlas |
| 0002 | Al-Falaq |
| 0003 | An-Nas |
| 0004 | Al-Kautsar |
| 0005 | Al-'Ashr |
| 0006 | Al-Ma'un |
| 0007 | Al-Kafirun |

## Library yang Dibutuhkan

Instal lewat Library Manager Arduino IDE:

- `Adafruit GFX Library`
- `Adafruit ST7789 Library`
- `XPT2046_Touchscreen`
- `DFRobotDFPlayerMini`

Board: **ESP32** (Boards Manager: *esp32 by Espressif Systems*). Kode mendukung ESP32 Arduino core 2.x maupun 3.x untuk PWM backlight.

## Struktur File Kode

Semua file berada dalam satu folder sketch:

| File | Fungsi |
|------|--------|
| `Main.ino` | Setup, loop utama, dan variabel global |
| `Config.h` | Pin, konstanta track audio, warna, enum state, dan deklarasi |
| `Ultrasonic.ino` | Pembacaan sensor, kalibrasi, FSM Mode Sholat, dan FSM Mode Belajar |
| `DFPlayer.ino` | Inisialisasi dan pemutaran audio, serta antrean audio |
| `UI.ino` | Semua tampilan layar, animasi, sentuhan, dan urutan audio Mode Belajar |

## Cara Menggunakan

1. Letakkan alat di depan Anda, lalu arahkan kedua sensor ke tubuh. Sesuaikan tinggi sensor atas dengan tinggi badan.
2. Nyalakan alat, tunggu animasi loading, lalu sentuh **MULAI**.
3. Pilih **MODE SHOLAT** atau **MODE BELAJAR**, lalu pilih jenis sholat.
4. Khusus Mode Belajar: pilih 2 surah pendek, lalu pilih tampilan bacaan.
5. Berdiri tegak dan diam sampai **kalibrasi** selesai.
6. Mulai sholat. Alat membaca gerakan Anda tanpa perlu menekan tombol.
7. Volume dan kecerahan dapat diatur di menu **PENGATURAN**.

## Parameter yang Bisa Disesuaikan

- `TS_MINX`, `TS_MAXX`, `TS_MINY`, `TS_MAXY` di `Config.h`: kalibrasi touchscreen.
- `STAND_COOLDOWN_MS`, `RUKUK_MIN_MS`, `SUJUD_MIN_MS`, `TAHIYAT_MIN_MS` di `Main.ino`: waktu tahan minimal tiap gerakan.
- `ITIDAL_PUTAR_TRACK6` di `Config.h`: `1` memutar track 0006 lalu 0007 saat i'tidal, `0` hanya track 0007.
- `BELAJAR_DEBUG` di `Ultrasonic.ino`: `1` menampilkan log sensor di Serial Monitor (115200 baud).

## Pemecahan Masalah

| Gejala | Kemungkinan penyebab |
|--------|----------------------|
| `DFPlayer Error!` di Serial Monitor | TX/RX tidak disilang, VCC bukan 5V, atau SD card bukan FAT32 |
| Audio tidak terdengar / track salah | Nama file tidak 4 digit atau salah folder (`01` / `02`) |
| Urutan audio loncat | Kabel BUSY (GPIO 26) bermasalah |
| Gerakan tidak terbaca | Posisi sensor tidak pas atau kalibrasi dilakukan saat tidak berdiri tegak |
| Sentuhan meleset | Perlu menyesuaikan nilai `TS_MINX` / `TS_MAXX` / `TS_MINY` / `TS_MAXY` |

## Lisensi

Proyek ini dilisensikan di bawah [MIT License](LICENSE).
