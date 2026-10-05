#include "Config.h"

int belajarAudioStep = 0; 
bool isProcessingSequence = false; // Flag pengunci urutan audio sekuensial saat berdiri
int globalVolume = 20;             // Nilai default volume (0 - 30)
int globalBrightness = 100;        // Kecerahan layar dalam persen (10 - 100)

// ===== PENDETEKSI AUDIO SELESAI (KHUSUS URUTAN MODE BELAJAR) =====
// Masalah lama: pin BUSY dianggap "tidak main" (HIGH) sebelum DFPlayer sempat menurunkannya ke LOW,
// atau sempat berkedip HIGH -> urutan audio langsung loncat. Sekarang audio dianggap SELESAI hanya jika:
//  - BUSY sudah pernah LOW (audio benar-benar mulai), lalu
//  - BUSY kembali HIGH stabil minimal AUDIO_END_DEBOUNCE_MS.
// Bila BUSY tidak pernah LOW (kabel BUSY bermasalah), dipakai cadangan: tanya status lewat serial.
const unsigned long AUDIO_START_WAIT_MS   = 2500; // waktu tunggu BUSY turun ke LOW setelah perintah play
const unsigned long AUDIO_END_DEBOUNCE_MS = 700;  // BUSY harus HIGH stabil selama ini baru dianggap selesai
const unsigned long AUDIO_FALLBACK_MAX_MS = 4000; // batas tunggu jika audio tidak terdeteksi sama sekali

unsigned long seqAudioStartMs = 0;   // kapan perintah play dikirim
bool seqBusySeenLow = false;         // BUSY pernah LOW?
unsigned long seqBusyHighSince = 0;  // sejak kapan BUSY HIGH (0 = sedang tidak HIGH)
bool seqSerialSeenPlaying = false;   // cadangan: DFPlayer pernah melapor "playing" via serial
int seqSerialNotPlayingCnt = 0;
unsigned long seqLastStatePoll = 0;

// Dipanggil setiap kali audio urutan baru dimulai
void startSequenceAudio(int folderNo, int trackNo) {
  seqBusySeenLow = false;
  seqBusyHighSince = 0;
  seqSerialSeenPlaying = false;
  seqSerialNotPlayingCnt = 0;
  seqLastStatePoll = 0;
  triggerAudioOnce(folderNo, trackNo, true); // forcePlay agar pasti diputar
  seqAudioStartMs = millis();                // hitung waktu tunggu SETELAH perintah terkirim
}

// true = audio urutan saat ini masih berjalan (atau belum sempat mulai)
bool isAudioPlaying() {
  unsigned long now = millis();

  // BUSY LOW = DFPlayer sedang memutar
  if (digitalRead(PIN_DFPLAYER_BUSY) == LOW) {
    seqBusySeenLow = true;
    seqBusyHighSince = 0;
    return true;
  }

  // BUSY HIGH, dan sebelumnya sudah pernah LOW -> tunggu HIGH stabil dulu baru dianggap selesai
  if (seqBusySeenLow) {
    if (seqBusyHighSince == 0) seqBusyHighSince = now;
    return (now - seqBusyHighSince < AUDIO_END_DEBOUNCE_MS);
  }

  // BUSY belum pernah LOW: beri waktu DFPlayer untuk mulai
  if (now - seqAudioStartMs < AUDIO_START_WAIT_MS) return true;

  // CADANGAN: BUSY tidak pernah LOW -> tanya status lewat serial (513 = playing)
  if (now - seqLastStatePoll >= 500) {
    seqLastStatePoll = now;
    int st = myDFPlayer.readState();
    if (st == 513) {
      seqSerialSeenPlaying = true;
      seqSerialNotPlayingCnt = 0;
    } else if (st != -1 && seqSerialSeenPlaying) {
      seqSerialNotPlayingCnt++;
    }
  }
  if (seqSerialSeenPlaying) return (seqSerialNotPlayingCnt < 2);

  // Audio tidak terdeteksi sama sekali (file hilang/modul tidak merespons) -> jangan macet selamanya
  return (now - seqAudioStartMs < AUDIO_FALLBACK_MAX_MS);
}

// Daftar Nama Surah (Folder 02)
const String listSurah[7] = {
  "1. Al-Ikhlas", "2. Al-Falaq", "3. An-Nas",
  "4. Al-Kautsar", "5. Al-'Ashr", "6. Al-Ma'un", "7. Al-Kafirun"
};

// BACAAN AL-FATIHAH
const BacaanText surahFatihah = {
  "Bismillahir-rahmanir-rahim. Alhamdu lillahi rabbil-'alamin. Ar-rahmanir-rahim. Maliki yaumid-din. Iyyaka na'budu wa iyyaka nasta'in. Ihdinas-siratal-mustaqim. Siratalladzina an'amta 'alaihim ghairil-maghdubi 'alaihim wa lad-dallin.",
  "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ . الْحَمْدُ لِلَّهِ رَبِّ الْعَالَمِينَ . الرَّحْمَٰنِ الرَّحِيمِ . مَالِكِ يَوْمِ الدِّينِ . إِيَّاكَ نَعْبُدُ وَإِيَّاكَ نَسْتَعِينُ . اهْدِنَا الصِّرَاطَ الْمُسْتَقِيمَ . صِرَاطَ الَّذِينَ أَنْعَمْتَ عَلَيْهِمْ غَيْرِ الْمَغْضُوبِ عَلَيْهِمْ وَلَا الضَّالِّينَ"
};

// DATA BACAAN SURAH PENDEK (FOLDER 02)
const BacaanText surahIkhlas = {
  "Qul huwallahu ahad. Allahus-samad. Lam yalid wa lam yulad. Wa lam yakul lahu kufuwan ahad.",
  "قُلْ هُوَ اللَّهُ أَحَدٌ . اللَّهُ الصَّمَدُ . لَمْ يَلِدْ وَلَمْ يُولَدْ . وَلَمْ يَكُن لَّهُ كُفُوًا أَحَدٌ"
};

const BacaanText surahFalaq = {
  "Qul a'udzu birabbil-falaq. Min syarri ma khalaq. Wa min syarri ghasiqin iza waqab. Wa min syarrin-naffatsati fil-'uqad. Wa min syarri hasidin iza hasad.",
  "قُلْ أَعُوذُ بِرَبِّ الْفَلَقِ . مِن شَرِّ مَا خَلَقَ . وَمِن شَرِّ غَاسِقٍ إِذَا وَقَبَ . وَمِن شَرِّ النَّفَّاثَاتِ فِي الْعُقَدِ . وَمِن شَرِّ حَاسِدٍ إِذَا حَسَدَ"
};

const BacaanText surahNas = {
  "Qul a'udzu birabbin-nas. Malikin-nas. Ilahin-nas. Min syarril-waswasil-khannas. Alladzi yuwaswisu fi sudurin-nas. Minal-jinnati wan-nas.",
  "قُلْ أَعُوذُ بِرَبِّ النَّاسِ . مَلِكِ النَّاسِ . إِلَٰهِ النَّاسِ . مِن شَرِّ الْوَسْوَاسِ الْخَنَّاسِ . الَّذِي يُوَسْوِسُ فِي صُدُورِ النَّاسِ . مِنَ الْجِنَّةِ وَالنَّاسِ"
};

const BacaanText surahKautsar = {
  "Inna a'tainakal-kaustar. Fa salli lirabbika wan-har. Inna syani'aka huwal-abtar.",
  "إِنَّا أَعْطَيْنَاكَ الْكَوْثَرَ . فَصَلِّ لِرَبِّكا وَانْحَرْ . إِنَّ شَانِئَكِ هُوَ الْأَبْتَرُ"
};

const BacaanText surahAshr = {
  "Wal-'asr. Innal-insana lafi khusr. Illalladzina amanu wa 'amilus-salihati wa tawasau bil-haqqi wa tawasau bis-sabr.",
  "وَالْعَصْرِ . إِنَّ الْإِنسَانَ لَفِي خُسْرٍ . إِلَّا الَّذِينَ آمَنُوا وَعَمِلُوا الصَّالِحَاتِ وَتَوَاصَوْا بِالْحَقِّ وَتَوَاصَوْا بِالصَّبْرِ"
};

const BacaanText surahMaun = {
  "Ara'aitalladzi yukadzdzibu bid-din. Fa dzalikal-ladzi yadu''ul-yatim. Wa la yahuddu 'ala ta'amil-miskin. Fawailul lil-musallin. Alladzina hum 'an salatihim sahun. Alladzina hum yurau'na. Wa yamna'unal-ma'un.",
  "أَرَأَيْتَ الَّذِي يُكَذِّبُ بِالدِّينِ . فَذَٰلِكَ الَّذِي يَدُعُّ الْيَتِيمَ . وَلَا يَحُضُّ عَلَىٰ طَعَامِ الْمِسْكِينِ . فَوَيْلٌ لِّلْمُصَلِّينَ . الَّذِينَ هُمْ عَن صَلَاتِهِمْ سَاهُونَ . الَّذِينَ هُمْ يُرَاءُونَ . وَيَمْنَعُونَ الْمَاعُونَ"
};

const BacaanText surahKafirun = {
  "Qul ya ayyuhal-kafirun. La a'budu ma ta'budun. Wa la antum 'abiduna ma a'bud. Wa la ana 'abidum ma 'abattum. Wa la antum 'abiduna ma a'bud. Lakum dinukum wa liya din.",
  "قُلْ يَا أَيُّهَا الْكَافِرُونَ . لَا أَعْبُدُ مَا تَعْبُدُونَ . وَلَا أَنتُمْ عَابِدُونَ مَا أَعْبُدُ . وَلَا أَنَا عَابِدٌ مَّا عَبَدتُّمْ . وَلَا أَنتُمْ عَابِدُونَ مَا أَعْبُدُ . لَكُمْ دِينُكُمْ وَلِيَ دِينِ"
};

BacaanText getSurahContent(int index) {
  switch (index) {
    case 0: return surahIkhlas;
    case 1: return surahFalaq;
    case 2: return surahNas;
    case 3: return surahKautsar;
    case 4: return surahAshr;
    case 5: return surahMaun;
    case 6: return surahKafirun;
    default: return surahIkhlas;
  }
}

void drawWrappedText(String text, int x, int y, int maxW, int maxH, uint16_t color, uint16_t bg, uint8_t textSize) {
  tft.fillRect(x, y, maxW, maxH, bg); 
  tft.setTextColor(color, bg);
  tft.setTextSize(textSize);

  int charWidth = 6 * textSize; 
  int lineHeight = 8 * textSize + 2; 
  int maxCharsPerLine = maxW / charWidth;

  int currentY = y;
  int startIdx = 0;

  while (startIdx < text.length() && (currentY + lineHeight) <= (y + maxH)) {
    int nextNewline = text.indexOf('\n', startIdx);
    int endIdx = startIdx + maxCharsPerLine;

    if (nextNewline != -1 && nextNewline < endIdx) {
      endIdx = nextNewline;
    } else if (endIdx >= text.length()) {
      endIdx = text.length();
    } else {
      int lastSpace = text.lastIndexOf(' ', endIdx);
      if (lastSpace > startIdx) {
        endIdx = lastSpace;
      }
    }

    String lineText = text.substring(startIdx, endIdx);
    lineText.trim();

    tft.setCursor(x, currentY);
    tft.print(lineText);

    startIdx = endIdx;
    if (startIdx < text.length() && text.charAt(startIdx) == '\n') {
      startIdx++;
    } else {
      while (startIdx < text.length() && text.charAt(startIdx) == ' ') {
        startIdx++; 
      }
    }

    currentY += lineHeight;
  }
}

void drawBackButton() {
  tft.fillRoundRect(8, 4, 60, 24, 4, COLOR_RED);
  tft.drawRoundRect(8, 4, 60, 24, 4, COLOR_WHITE);
  tft.setTextColor(COLOR_WHITE, COLOR_RED);
  tft.setTextSize(1);
  tft.setCursor(18, 12);
  tft.print("< BACK");
}

bool getTouchCoordinates(int &outX, int &outY) {
  if (!touch.touched()) return false;

  long sumX = 0, sumY = 0;
  int samples = 5; 

  for (int i = 0; i < samples; i++) {
    TS_Point p = touch.getPoint();
    sumX += p.x;
    sumY += p.y;
    delayMicroseconds(500);
  }

  int rawX = sumX / samples;
  int rawY = sumY / samples;

  int mappedX = map(rawX, TS_MINX, TS_MAXX, SCREEN_W, 0);
  int mappedY = map(rawY, TS_MINY, TS_MAXY, SCREEN_H, 0);

  outX = constrain(mappedX, 0, SCREEN_W - 1);
  outY = constrain(mappedY, 0, SCREEN_H - 1);

  return true;
}

// =====================================================================
// HELPER TAMPILAN BARU (LOADING, HALAMAN PEMBUKA, MENU UTAMA, BANTUAN)
// =====================================================================

// Cetak teks rata tengah pada area lebar w yang dimulai dari x (font bawaan: 6 px per karakter)
void drawCenteredText(const String &txt, int x, int w, int y, uint8_t size, uint16_t color, uint16_t bg) {
  int textW = txt.length() * 6 * size;
  int cx = x + (w - textW) / 2;
  if (cx < x) cx = x;
  tft.setTextSize(size);
  tft.setTextColor(color, bg);
  tft.setCursor(cx, y);
  tft.print(txt);
}

// Meredupkan warna RGB565 (k = 0 terang ... 7 paling redup)
uint16_t fadeColor(uint16_t c, uint8_t k) {
  float sc = 1.0 - (k * 0.12);
  uint16_t r = (c >> 11) & 0x1F;
  uint16_t g = (c >> 5) & 0x3F;
  uint16_t bl = c & 0x1F;
  return ((uint16_t)(r * sc) << 11) | ((uint16_t)(g * sc) << 5) | (uint16_t)(bl * sc);
}

// LOADING SYSTEM (ANIMASI) -> otomatis lanjut ke HALAMAN PEMBUKA
void drawSplashScreen() {
  initBrightness();   // aktifkan PWM backlight (kecerahan bisa diatur di menu Pengaturan)
  currentState = STATE_SPLASH;
  tft.fillScreen(COLOR_BG);

  const int cx = 160, cy = 48, R = 22;       // pusat & radius spinner
  const String title = "SUTRA PINTAR";
  const int totalFrames = 60;                // 60 frame x 50 ms = +/- 3 detik
  const int barX = 50, barY = 188, barW = 220, barH = 12;
  const char* statusMsg[5] = {
    "Memulai sistem...",
    "Menyiapkan sensor...",
    "Menyiapkan layar sentuh...",
    "Menyiapkan audio...",
    "Siap!"
  };
  int lastMsg = -1;
  int shownChars = 0;

  tft.drawRoundRect(barX, barY, barW, barH, 4, COLOR_TEAL);

  for (int f = 0; f <= totalFrames; f++) {
    // 1) Spinner 8 titik berputar dengan ekor memudar
    int head = f % 8;
    for (int i = 0; i < 8; i++) {
      float ang = (i * (PI / 4.0)) - (PI / 2.0);
      int dx = cx + (int)(R * cos(ang));
      int dy = cy + (int)(R * sin(ang));
      int k = (head - i + 8) % 8;
      tft.fillCircle(dx, dy, 4, fadeColor(COLOR_CYAN, k));
    }

    // 2) Judul muncul huruf demi huruf
    int want = min((int)title.length(), f / 2 + 1);
    if (want != shownChars) {
      shownChars = want;
      tft.setTextSize(3);
      tft.setTextColor(COLOR_CYAN, COLOR_BG);
      tft.setCursor(52, 100);
      tft.print(title.substring(0, shownChars));
    }

    // 3) Tagline muncul setelah judul lengkap
    if (f == 26) {
      drawCenteredText("Pembantu Belajar & Mengingat Urutan Sholat", 0, 320, 138, 1, COLOR_TEXT_DIM, COLOR_BG);
    }

    // 4) Progress bar
    int fillW = (barW - 4) * f / totalFrames;
    if (fillW > 0) tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, COLOR_CYAN);

    // 5) Teks status berganti sesuai progres
    int msgIdx = (f < 15) ? 0 : (f < 30) ? 1 : (f < 45) ? 2 : (f < 58) ? 3 : 4;
    if (msgIdx != lastMsg) {
      lastMsg = msgIdx;
      tft.fillRect(0, 206, 320, 14, COLOR_BG);
      drawCenteredText(statusMsg[msgIdx], 0, 320, 208, 1, COLOR_WHITE, COLOR_BG);
    }

    delay(50);
  }

  delay(300);
  drawOpeningPage();
}

// HALAMAN PEMBUKA (identitas perangkat + tombol MULAI)
void drawOpeningPage() {
  currentState = STATE_PEMBUKA;
  tft.fillScreen(COLOR_BG);

  // Ornamen bulan sabit & bintang
  tft.fillCircle(160, 50, 24, COLOR_CYAN);
  tft.fillCircle(170, 44, 20, COLOR_BG);
  tft.fillCircle(192, 38, 3, COLOR_WHITE);
  tft.fillCircle(200, 56, 2, COLOR_WHITE);

  drawCenteredText("SUTRA PINTAR", 0, 320, 92, 3, COLOR_CYAN, COLOR_BG);
  tft.drawFastHLine(90, 122, 140, COLOR_TEAL);

  drawCenteredText("Pembantu Belajar &", 0, 320, 132, 2, COLOR_WHITE, COLOR_BG);
  drawCenteredText("Mengingat Urutan Sholat", 0, 320, 152, 2, COLOR_WHITE, COLOR_BG);

  // Tombol MULAI
  tft.fillRoundRect(90, 184, 140, 40, 8, COLOR_GREEN);
  tft.drawRoundRect(90, 184, 140, 40, 8, COLOR_WHITE);
  drawCenteredText("MULAI", 90, 140, 194, 3, COLOR_WHITE, COLOR_GREEN);
}

// MENU UTAMA (4 PILIHAN: MODE SHOLAT, MODE BELAJAR, PENGATURAN, BANTUAN)
void drawMainMenu() {
  currentState = STATE_MAIN_MENU;
  tft.fillScreen(COLOR_BG);

  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawCenteredText("MENU UTAMA", 0, 320, 10, 2, COLOR_WHITE, COLOR_CARD);

  const char* labels[4] = {"MODE SHOLAT", "MODE BELAJAR", "PENGATURAN", "BANTUAN"};
  const uint16_t accents[4] = {COLOR_GREEN, COLOR_BLUE_BTN, COLOR_ACCENT, COLOR_CYAN};

  for (int i = 0; i < 4; i++) {
    int y = 44 + (i * 46);                       // 44, 90, 136, 182 (tinggi 40, jarak 6)
    tft.fillRoundRect(30, y, 260, 40, 8, COLOR_CARD);
    tft.drawRoundRect(30, y, 260, 40, 8, COLOR_TEAL);
    tft.fillRoundRect(38, y + 8, 5, 24, 2, accents[i]);   // aksen warna di kiri
    drawCenteredText(labels[i], 30, 260, y + 12, 2, COLOR_CYAN, COLOR_CARD);
  }
}

// =====================================================================
// MENU BANTUAN (3 TOPIK, PENJELASAN PER SLIDE)
// =====================================================================
const HelpSlide helpCara[] = {
  {"Posisi Alat", "Letakkan Sutra Pintar di depan Anda, seperti sutrah saat sholat."},
  {"Arah Sensor", "Arahkan kedua sensor ke tubuh. Sesuaikan tinggi sensor atas dengan tinggi badan Anda."},
  {"Pilih Mode", "Di Menu Utama, sentuh MODE SHOLAT atau MODE BELAJAR."},
  {"Pilih Sholat", "Sentuh jenis sholat yang akan dikerjakan. Jumlah rakaat mengikuti pilihan Anda."},
  {"Kalibrasi", "Berdiri tegak dan diam sampai kalibrasi selesai. Alat mencatat posisi awal Anda."},
  {"Mulai Sholat", "Setelah kalibrasi, silakan mulai. Alat membaca gerakan tanpa perlu menekan tombol."},
  {"Atur Volume", "Suara panduan dipakai di Mode Belajar. Atur volumenya di menu PENGATURAN."},
};

const HelpSlide helpSholat[] = {
  {"Fungsi Mode Sholat", "Membantu Anda mengetahui posisi gerakan, jumlah rakaat, dan jumlah sujud."},
  {"Tanpa Suara", "Tidak ada suara panduan, agar tidak mengganggu kekhusyukan sholat Anda."},
  {"Membaca Gerakan", "Dua sensor membaca gerakan: berdiri, ruku', i'tidal, sujud, dan duduk."},
  {"Urutan Gerakan", "Gerakan dibaca berurutan. Satu pembacaan yang tidak stabil tidak langsung dihitung."},
  {"Tampilan Layar", "Layar menampilkan nama sholat, rakaat, jumlah sujud, dan posisi saat ini."},
  {"Selesai", "Rakaat dan sujud dihitung otomatis. Setelah tahiyat akhir, layar menampilkan SHOLAT SELESAI."},
};

const HelpSlide helpBelajar[] = {
  {"Fungsi Mode Belajar", "Mengajarkan urutan sholat lewat bacaan di layar dan suara panduan."},
  {"Persiapan", "Pilih sholat, pilih 2 surah pendek, lalu pilih tampilan bacaan Latin atau Arab."},
  {"Mulai Belajar", "Diawali Takbiratul Ihram, lalu Al-Fatihah, dan surah pendek pilihan Anda."},
  {"Ikuti Gerakan", "Sensor tetap membaca gerakan Anda. Saat gerakan berganti, bacaan dan suara ikut berganti."},
  {"Bacaan Tiap Gerakan", "Ruku', i'tidal, sujud, duduk, dan tahiyat memiliki bacaan dan suaranya masing-masing."},
  {"Selesai", "Di akhir, layar menampilkan halaman selesai. Atur volume suara di menu PENGATURAN."},
};

const char* const helpTopicTitle[3] = {"CARA MENGGUNAKAN", "MODE SHOLAT", "MODE BELAJAR"};

int bantuanTopic = 0;      // 0 = Cara Menggunakan, 1 = Mode Sholat, 2 = Mode Belajar
int bantuanSlideIdx = 0;   // slide yang sedang dibaca

const HelpSlide* getHelpSlides(int topic, int &count) {
  if (topic == 0) { count = sizeof(helpCara) / sizeof(helpCara[0]);     return helpCara; }
  if (topic == 1) { count = sizeof(helpSholat) / sizeof(helpSholat[0]); return helpSholat; }
  count = sizeof(helpBelajar) / sizeof(helpBelajar[0]);
  return helpBelajar;
}

void drawBantuanMenu() {
  currentState = STATE_BANTUAN_MENU;
  tft.fillScreen(COLOR_BG);

  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton();
  drawCenteredText("BANTUAN", 70, 250, 10, 2, COLOR_WHITE, COLOR_CARD);

  const char* labels[3] = {"1. CARA MENGGUNAKAN", "2. MODE SHOLAT", "3. MODE BELAJAR"};
  for (int i = 0; i < 3; i++) {
    int y = 48 + (i * 50);                       // 48, 98, 148 (tinggi 42)
    tft.fillRoundRect(30, y, 260, 42, 8, COLOR_CARD);
    tft.drawRoundRect(30, y, 260, 42, 8, COLOR_TEAL);
    tft.setTextColor(COLOR_CYAN, COLOR_CARD);
    tft.setTextSize(2);
    tft.setCursor(46, y + 13);
    tft.print(labels[i]);
  }

  // Tombol kembali ke menu utama
  tft.fillRoundRect(30, 200, 260, 34, 8, COLOR_BLUE_BTN);
  tft.drawRoundRect(30, 200, 260, 34, 8, COLOR_WHITE);
  drawCenteredText("KE MENU UTAMA", 30, 260, 209, 2, COLOR_WHITE, COLOR_BLUE_BTN);
}

void drawBantuanSlide(bool fullRedraw) {
  currentState = STATE_BANTUAN_SLIDE;

  int count = 0;
  const HelpSlide* slides = getHelpSlides(bantuanTopic, count);
  if (bantuanSlideIdx < 0) bantuanSlideIdx = 0;
  if (bantuanSlideIdx > count - 1) bantuanSlideIdx = count - 1;
  bool isLast = (bantuanSlideIdx == count - 1);
  bool isFirst = (bantuanSlideIdx == 0);

  if (fullRedraw) {
    tft.fillScreen(COLOR_BG);
    tft.fillRect(0, 0, 320, 35, COLOR_CARD);
    drawBackButton();
    drawCenteredText(helpTopicTitle[bantuanTopic], 70, 250, 10, 2, COLOR_WHITE, COLOR_CARD);
  }

  // Kartu isi slide
  tft.fillRoundRect(10, 42, 300, 152, 8, COLOR_CARD);
  tft.drawRoundRect(10, 42, 300, 152, 8, COLOR_TEAL);

  tft.setTextColor(COLOR_CYAN, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(20, 52);
  tft.print(slides[bantuanSlideIdx].head);
  tft.drawFastHLine(20, 72, 280, COLOR_TEAL);

  drawWrappedText(slides[bantuanSlideIdx].body, 20, 78, 280, 110, COLOR_WHITE, COLOR_CARD, 2);

  // Bar bawah: SEBELUM | x/y | LANJUT atau SELESAI
  tft.fillRect(0, 196, 320, 44, COLOR_BG);

  uint16_t prevColor = isFirst ? COLOR_DISABLED : COLOR_BLUE_BTN;
  tft.fillRoundRect(8, 200, 104, 34, 8, prevColor);
  tft.drawRoundRect(8, 200, 104, 34, 8, COLOR_WHITE);
  drawCenteredText("SEBELUM", 8, 104, 209, 2, COLOR_WHITE, prevColor);

  String idxText = String(bantuanSlideIdx + 1) + "/" + String(count);
  drawCenteredText(idxText, 112, 96, 209, 2, COLOR_TEXT_DIM, COLOR_BG);

  uint16_t nextColor = isLast ? COLOR_GREEN : COLOR_BLUE_BTN;
  tft.fillRoundRect(208, 200, 104, 34, 8, nextColor);
  tft.drawRoundRect(208, 200, 104, 34, 8, COLOR_WHITE);
  drawCenteredText(isLast ? "SELESAI" : "LANJUT >", 208, 104, 209, 2, COLOR_WHITE, nextColor);
}

// MENU PENGATURAN VOLUME
// ===== KECERAHAN LAYAR (PWM PADA PIN BACKLIGHT TFT_LED) =====
#define BL_PWM_FREQ 5000
#define BL_PWM_RES  8      // 0 - 255
#define BL_PWM_CH   0      // dipakai pada ESP32 Arduino core 2.x

void applyBrightness() {
  int duty = map(globalBrightness, 0, 100, 0, 255);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcWrite(TFT_LED, duty);
#else
  ledcWrite(BL_PWM_CH, duty);
#endif
}

void initBrightness() {
  static bool sudahInit = false;
  if (sudahInit) return;
  sudahInit = true;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttach(TFT_LED, BL_PWM_FREQ, BL_PWM_RES);
#else
  ledcSetup(BL_PWM_CH, BL_PWM_FREQ, BL_PWM_RES);
  ledcAttachPin(TFT_LED, BL_PWM_CH);
#endif
  applyBrightness();
}

// Satu kartu pengaturan: judul, tombol [-] nilai [+], dan keterangan kecil
void drawSettingCard(int cardY, const char* title, const char* valueText, const char* hint) {
  tft.fillRoundRect(20, cardY, 280, 90, 8, COLOR_CARD);
  tft.drawRoundRect(20, cardY, 280, 90, 8, COLOR_TEAL);
  drawCenteredText(title, 20, 280, cardY + 8, 2, COLOR_WHITE, COLOR_CARD);

  int by = cardY + 28;   // baris tombol (tinggi 44)

  // Tombol Minus (-)
  tft.fillRoundRect(40, by, 60, 44, 6, COLOR_RED);
  tft.drawRoundRect(40, by, 60, 44, 6, COLOR_WHITE);
  drawCenteredText("-", 40, 60, by + 10, 3, COLOR_WHITE, COLOR_RED);

  // Tampilan nilai
  tft.fillRect(110, by, 100, 44, COLOR_BG);
  tft.drawRect(110, by, 100, 44, COLOR_TEAL);
  drawCenteredText(valueText, 110, 100, by + 10, 3, COLOR_GREEN, COLOR_BG);

  // Tombol Plus (+)
  tft.fillRoundRect(220, by, 60, 44, 6, COLOR_GREEN);
  tft.drawRoundRect(220, by, 60, 44, 6, COLOR_WHITE);
  drawCenteredText("+", 220, 60, by + 10, 3, COLOR_WHITE, COLOR_GREEN);

  drawCenteredText(hint, 20, 280, cardY + 78, 1, COLOR_TEXT_DIM, COLOR_CARD);
}

// MENU PENGATURAN: VOLUME AUDIO + KECERAHAN LAYAR
void drawVolumeMenu() {
  currentState = STATE_VOLUME_SETTING;
  tft.fillScreen(COLOR_BG);

  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton();
  drawCenteredText("PENGATURAN", 70, 250, 10, 2, COLOR_WHITE, COLOR_CARD);

  char buf[8];
  snprintf(buf, sizeof(buf), "%02d", globalVolume);
  drawSettingCard(40, "VOLUME AUDIO", buf, "Rentang Volume: 0 - 30");

  snprintf(buf, sizeof(buf), "%d%%", globalBrightness);
  drawSettingCard(138, "KECERAHAN", buf, "Rentang Kecerahan: 10% - 100%");
}

void drawPilihSholatMenu() {
  currentState = STATE_PILIH_SHOLAT;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(100, 10);
  tft.print("PILIH SHOLAT");

  String nameSholat[5] = {"SUBUH", "DZUHUR", "ASHAR", "MAGHRIB", "ISYA"};
  String detailRakaat[5] = {"2 RAKAAT", "4 RAKAAT", "4 RAKAAT", "3 RAKAAT", "4 RAKAAT"};

  for (int i = 0; i < 5; i++) {
    int yPos = 42 + (i * 38);

    tft.fillRoundRect(20, yPos, 280, 32, 6, COLOR_CARD);
    tft.drawRoundRect(20, yPos, 280, 32, 6, COLOR_TEAL);

    tft.setTextColor(COLOR_CYAN, COLOR_CARD);
    tft.setTextSize(2);
    tft.setCursor(35, yPos + 8);
    tft.print(nameSholat[i]);

    tft.setTextColor(COLOR_WHITE, COLOR_CARD);
    tft.setTextSize(1);
    int detailX = 280 - (detailRakaat[i].length() * 6);
    tft.setCursor(detailX, yPos + 12);
    tft.print(detailRakaat[i]);
  }
}

void drawBelajarPilihSholatMenu() {
  currentState = STATE_BELAJAR_PILIH_SHOLAT;
  isBelajarMode = true;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(80, 10);
  tft.print("BELAJAR SHOLAT");

  String nameSholat[5] = {"SUBUH", "DZUHUR", "ASHAR", "MAGHRIB", "ISYA"};
  String detailRakaat[5] = {"2 RAKAAT", "4 RAKAAT", "4 RAKAAT", "3 RAKAAT", "4 RAKAAT"};

  for (int i = 0; i < 5; i++) {
    int yPos = 42 + (i * 38);

    tft.fillRoundRect(20, yPos, 280, 32, 6, COLOR_CARD);
    tft.drawRoundRect(20, yPos, 280, 32, 6, COLOR_TEAL);

    tft.setTextColor(COLOR_CYAN, COLOR_CARD);
    tft.setTextSize(2);
    tft.setCursor(35, yPos + 8);
    tft.print(nameSholat[i]);

    tft.setTextColor(COLOR_WHITE, COLOR_CARD);
    tft.setTextSize(1);
    int detailX = 280 - (detailRakaat[i].length() * 6);
    tft.setCursor(detailX, yPos + 12);
    tft.print(detailRakaat[i]);
  }
}

void drawPilihSurahMenu() {
  currentState = STATE_BELAJAR_PILIH_SURAH;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 32, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(85, 8);
  tft.print("PILIH 2 SURAH");

  for (int i = 0; i < 7; i++) {
    int col = i % 2;        
    int row = i / 2;        
    
    int xPos = (col == 0) ? 10 : 165;
    int yPos = 38 + (row * 36); 
    int cardW = 145;
    int cardH = 32;

    uint16_t cardBg = selectedSurah[i] ? COLOR_GREEN : COLOR_CARD;
    uint16_t borderCol = selectedSurah[i] ? COLOR_WHITE : COLOR_TEAL;

    tft.fillRoundRect(xPos, yPos, cardW, cardH, 5, cardBg);
    tft.drawRoundRect(xPos, yPos, cardW, cardH, 5, borderCol);

    tft.setTextColor(selectedSurah[i] ? COLOR_WHITE : COLOR_CYAN, cardBg);
    tft.setTextSize(1);
    tft.setCursor(xPos + 8, yPos + 12);
    tft.print(listSurah[i]);

    if (selectedSurah[i]) {
      tft.setCursor(xPos + cardW - 20, yPos + 12);
      tft.print("[V]");
    }
  }

  int footerY = 188;
  tft.fillRoundRect(10, footerY, 170, 42, 6, COLOR_CARD);
  tft.drawRoundRect(10, footerY, 170, 42, 6, COLOR_TEAL);
  
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(18, footerY + 8);
  tft.print("Status Pemilihan:");

  tft.setTextColor(selectedSurahCount == 2 ? COLOR_GREEN : COLOR_ACCENT, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(18, footerY + 24);
  if (selectedSurahCount == 2) {
    tft.print("Siap Lanjut (2/2)");
  } else {
    tft.printf("Pilih %d Surah Lagi", 2 - selectedSurahCount);
  }

  uint16_t btnColor = (selectedSurahCount == 2) ? COLOR_GREEN : COLOR_DISABLED;
  tft.fillRoundRect(190, footerY, 120, 42, 6, btnColor);
  tft.drawRoundRect(190, footerY, 120, 42, 6, COLOR_WHITE);
  
  tft.setTextColor(COLOR_WHITE, btnColor);
  tft.setTextSize(2);
  tft.setCursor(205, footerY + 13);
  tft.print("LANJUT");
}

void drawPilihJenisBacaanMenu() {
  currentState = STATE_PILIH_JENIS_BACAAN;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(80, 10);
  tft.print("JENIS BACAAN");

  isTextArab = false;   // Hanya teks Latin (pilihan Arab dihapus)

  drawCenteredText("Tampilan huruf untuk bacaan:", 0, 320, 52, 1, COLOR_TEXT_DIM, COLOR_BG);

  tft.fillRoundRect(30, 80, 260, 80, 8, COLOR_GREEN);
  tft.drawRoundRect(30, 80, 260, 80, 8, COLOR_WHITE);
  drawCenteredText("TEKS LATIN", 30, 260, 99, 2, COLOR_WHITE, COLOR_GREEN);
  drawCenteredText("(Transliterasi Indonesia)", 30, 260, 130, 1, COLOR_CYAN, COLOR_GREEN);

  tft.fillRoundRect(190, 192, 120, 38, 6, COLOR_BLUE_BTN);
  tft.drawRoundRect(190, 192, 120, 38, 6, COLOR_WHITE);
  
  tft.setTextColor(COLOR_WHITE, COLOR_BLUE_BTN);
  tft.setTextSize(2);
  tft.setCursor(205, 203);
  tft.print("LANJUT");
}

void drawCalibrationScreen(String textStatus) {
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(105, 10);
  tft.print("KALIBRASI");

  tft.fillRoundRect(15, 60, 290, 130, 8, COLOR_CARD);
  tft.drawRoundRect(15, 60, 290, 130, 8, COLOR_TEAL);

  tft.setTextColor(COLOR_CYAN, COLOR_CARD);
  tft.setTextSize(2);

  if (textStatus.length() > 18) {
    String baris1 = "Mohon Diam di";
    String baris2 = "Posisi Standby!";
    
    int x1 = (320 - (baris1.length() * 12)) / 2;
    int x2 = (320 - (baris2.length() * 12)) / 2;

    tft.setCursor(x1, 100);
    tft.print(baris1);
    tft.setCursor(x2, 130);
    tft.print(baris2);
  } else {
    int xPos = (320 - (textStatus.length() * 12)) / 2;
    if (xPos < 20) xPos = 20;
    tft.setCursor(xPos, 115);
    tft.print(textStatus);
  }
}

void drawPrayerDetectionUI() {
  currentState = STATE_DETEKSI_SHOLAT;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  String headerText = selectedPrayerName;
  int headerX = (320 - (headerText.length() * 12)) / 2;
  tft.setCursor(headerX, 9);
  tft.print(headerText);

  tft.fillRoundRect(10, 42, 300, 52, 8, COLOR_CARD);
  tft.drawRoundRect(10, 42, 300, 52, 8, COLOR_TEAL);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(20, 48);
  tft.print("GERAKAN SAAT INI:");

  String motionText = "";
  switch (currentMotion) {
    case MOTION_STAND:   motionText = "BERDIRI"; break;
    case MOTION_RUKUK:   motionText = "RUKUK"; break;
    case MOTION_ITIDAL:  motionText = "I'TIDAL"; break;
    case MOTION_SUJUD1:  motionText = "SUJUD"; break;
    case MOTION_DUDUK:   motionText = "DUDUK"; break;
    case MOTION_SUJUD2:  motionText = "SUJUD2"; break;
    case MOTION_TAHIYAT: 
      motionText = (currentRakaat == 2 && targetRakaat > 2) ? "TAHIYAT AWAL" : "TAHIYAT AKHIR"; 
      break;
  }
  
  tft.setTextColor(COLOR_GREEN, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(20, 66);
  tft.print(motionText);

  tft.fillRoundRect(10, 100, 145, 80, 8, COLOR_CARD);
  tft.drawRoundRect(10, 100, 145, 80, 8, COLOR_TEAL);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(20, 108);
  tft.print("RAKAAT:");

  tft.setTextColor(COLOR_CYAN, COLOR_CARD);
  tft.setTextSize(3);
  tft.setCursor(20, 130);
  tft.printf("%d/%d", currentRakaat, targetRakaat);

  tft.fillRoundRect(165, 100, 145, 80, 8, COLOR_CARD);
  tft.drawRoundRect(165, 100, 145, 80, 8, COLOR_TEAL);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(175, 108);
  tft.print("TOTAL SUJUD:");

  tft.setTextColor(COLOR_ACCENT, COLOR_CARD);
  tft.setTextSize(3);
  tft.setCursor(175, 130);
  tft.printf("%d/%d", currentSujud, totalTargetSujud);

  tft.fillRoundRect(10, 190, 300, 40, 8, COLOR_RED);
  tft.drawRoundRect(10, 190, 300, 40, 8, COLOR_WHITE);
  tft.setTextColor(COLOR_WHITE, COLOR_RED);
  tft.setTextSize(2);
  tft.setCursor(120, 201);
  tft.print("BATAL");
}

// =====================================================================
// ANIMASI GERAKAN SHOLAT PADA LAYAR MODE BELAJAR
// Gambar orang (tampak samping) digerakkan halus antar pose, di-render ke kanvas
// di memori lalu dikirim sekali ke TFT (tanpa kedip). Hanya tampilan, tidak
// mempengaruhi sensor, audio, maupun logika state.
// =====================================================================
#define ANIM_X 214
#define ANIM_Y 42
#define ANIM_W 96
#define ANIM_H 112

#define ANIM_TAKBIR 0
#define ANIM_QIYAM  1
#define ANIM_RUKUK  2
#define ANIM_ITIDAL 3
#define ANIM_SUJUD  4
#define ANIM_DUDUK  5
#define ANIM_TAHIYAT 6

//                              hipX hipY thigh shin torso head handX handY bend finger
const AnimPose POSE_DOWN      = {44,   64,  0,    0,   0,    0,   45,   66,   1,   0};  // berdiri, tangan di sisi
const AnimPose POSE_SEDEKAP   = {44,   64,  0,    0,   0,    0,   55,   49,   1,   0};  // berdiri, tangan bersedekap
const AnimPose POSE_SEDEKAP_B = {44,   64,  0,    0,   2,    2,   55,   49,   1,   0};  // sedekap (sedikit bergoyang)
const AnimPose POSE_TAKBIR    = {44,   64,  0,    0,   0,    0,   57,   27,   1,   0};  // tangan diangkat sejajar telinga
const AnimPose POSE_RUKUK     = {38,   64,  12,  -4,  84,   95,   42,   83,   1,   0};  // ruku', punggung datar
const AnimPose POSE_KNEEL     = {45.5f,78.3f,10, -88,  0,    0,   55,   70,   1,   0};  // turun berlutut
const AnimPose POSE_SUJUD     = {43,   79,  20, -88, 110,  135,   85,  102,  -1,   0};  // sujud
const AnimPose POSE_DUDUK     = {33,   91,  80, -85,   5,    5,   51,   90,   1,   0};  // duduk iftirasy
const AnimPose POSE_TAHIYAT   = {33,   91,  80, -85,   5,    5,   51,   90,   1,   1};  // duduk + telunjuk

//                                 pose            gerak(ms) tahan(ms)
const AnimKey animKeysTakbir[] = { {POSE_TAKBIR,    700, 1000},
                                   {POSE_SEDEKAP,   700, 1000},
                                   {POSE_DOWN,      600,  300} };                 // berulang
const AnimKey animKeysQiyam[]  = { {POSE_SEDEKAP,   900,    0},
                                   {POSE_SEDEKAP_B, 1400,   0},
                                   {POSE_SEDEKAP,   1400,   0} };                 // berulang (napas halus)
const AnimKey animKeysRukuk[]  = { {POSE_RUKUK,     900,    0} };
const AnimKey animKeysItidal[] = { {POSE_TAKBIR,    900,  600},                   // bangkit & angkat tangan
                                   {POSE_DOWN,      700,    0} };                 // tangan turun
const AnimKey animKeysSujud[]  = { {POSE_KNEEL,     700,    0},
                                   {POSE_SUJUD,     800,    0} };
const AnimKey animKeysDuduk[]  = { {POSE_DUDUK,     900,    0} };
const AnimKey animKeysTahiyat[]= { {POSE_TAHIYAT,   900,    0} };

GFXcanvas16* animCanvas = nullptr;
AnimPose animCur = POSE_DOWN;
AnimPose animFrom = POSE_DOWN;
int animId = -1;
const AnimKey* animKeys = nullptr;
int animKeyCount = 0;
int animKeyIdx = 0;
bool animLoop = false;
bool animDone = true;
unsigned long animSegStart = 0;
unsigned long animLastDraw = 0;
unsigned long animLastSeen = 0;

void lerpPose(const AnimPose &a, const AnimPose &b, float t, AnimPose &o) {
  o.hipX   = a.hipX   + (b.hipX   - a.hipX)   * t;
  o.hipY   = a.hipY   + (b.hipY   - a.hipY)   * t;
  o.thigh  = a.thigh  + (b.thigh  - a.thigh)  * t;
  o.shin   = a.shin   + (b.shin   - a.shin)   * t;
  o.torso  = a.torso  + (b.torso  - a.torso)  * t;
  o.head   = a.head   + (b.head   - a.head)   * t;
  o.handX  = a.handX  + (b.handX  - a.handX)  * t;
  o.handY  = a.handY  + (b.handY  - a.handY)  * t;
  o.bend   = a.bend   + (b.bend   - a.bend)   * t;
  o.finger = a.finger + (b.finger - a.finger) * t;
}

// Garis tebal pada kanvas (th = 1..3)
void animThickLine(float x0, float y0, float x1, float y1, uint16_t col, int th) {
  int ax = lroundf(x0), ay = lroundf(y0), bx = lroundf(x1), by = lroundf(y1);
  animCanvas->drawLine(ax, ay, bx, by, col);
  if (th >= 2) {
    animCanvas->drawLine(ax + 1, ay, bx + 1, by, col);
    animCanvas->drawLine(ax, ay + 1, bx, by + 1, col);
  }
  if (th >= 3) {
    animCanvas->drawLine(ax - 1, ay, bx - 1, by, col);
    animCanvas->drawLine(ax, ay - 1, bx, by - 1, col);
  }
}

// Menghitung posisi siku (2 ruas lengan) agar tangan mencapai target
void solveArm(float sx, float sy, float hx, float hy, float bend, float &ex, float &ey, float &ox, float &oy) {
  const float LU = 15.0f, LF = 15.0f;
  float dx = hx - sx, dy = hy - sy;
  float d0 = sqrtf(dx * dx + dy * dy);
  if (d0 < 0.01f) { dx = 1; dy = 0; d0 = 1; }
  float ux = dx / d0, uy = dy / d0;
  float d = d0;
  if (d > LU + LF - 0.5f) d = LU + LF - 0.5f;
  if (d < 4.0f) d = 4.0f;
  float a = (LU * LU - LF * LF + d * d) / (2.0f * d);
  float h2 = LU * LU - a * a;
  float h = (h2 > 0) ? sqrtf(h2) : 0;
  ex = sx + ux * a - uy * h * bend;
  ey = sy + uy * a + ux * h * bend;
  ox = sx + ux * d;
  oy = sy + uy * d;
}

void renderAnimPose(const AnimPose &p) {
  if (animCanvas == nullptr) return;
  const float D2R = 0.0174533f;
  const float LT = 27.0f, HD = 11.0f, LTH = 20.0f, LSH = 20.0f;

  float hx = p.hipX, hy = p.hipY;
  float kx = hx + LTH * sinf(p.thigh * D2R), ky = hy + LTH * cosf(p.thigh * D2R);
  float ax = kx + LSH * sinf(p.shin * D2R),  ay = ky + LSH * cosf(p.shin * D2R);
  float sx = hx + LT * sinf(p.torso * D2R),  sy = hy - LT * cosf(p.torso * D2R);
  float cx = sx + HD * sinf(p.head * D2R),   cy = sy - HD * cosf(p.head * D2R);
  float ex, ey, ox, oy;
  solveArm(sx, sy, p.handX, p.handY, p.bend, ex, ey, ox, oy);

  animCanvas->fillScreen(COLOR_CARD);
  animCanvas->drawFastHLine(0, 105, ANIM_W, COLOR_TEAL);   // lantai

  // kaki
  animThickLine(hx, hy, kx, ky, COLOR_WHITE, 3);
  animThickLine(kx, ky, ax, ay, COLOR_WHITE, 3);
  animThickLine(ax, ay, ax + 6, ay, COLOR_WHITE, 2);
  // badan, leher, kepala
  animThickLine(hx, hy, sx, sy, COLOR_CYAN, 3);
  animThickLine(sx, sy, cx, cy, COLOR_CYAN, 2);
  animCanvas->fillCircle(lroundf(cx), lroundf(cy), 7, COLOR_WHITE);
  // lengan & tangan
  animThickLine(sx, sy, ex, ey, COLOR_ACCENT, 2);
  animThickLine(ex, ey, ox, oy, COLOR_ACCENT, 2);
  animCanvas->fillCircle(lroundf(ox), lroundf(oy), 2, COLOR_WHITE);
  if (p.finger > 0.5f) animThickLine(ox, oy, ox + 9, oy - 3, COLOR_ACCENT, 2);   // telunjuk tahiyat
}

void pushAnimFrame() {
  if (animCanvas == nullptr) return;
  tft.startWrite();
  tft.setAddrWindow(ANIM_X, ANIM_Y, ANIM_W, ANIM_H);
  tft.writePixels(animCanvas->getBuffer(), ANIM_W * ANIM_H);
  tft.endWrite();
}

// Kartu animasi di sisi kanan layar (kartu + nama gerakan + gambar saat ini)
void drawBelajarAnimationPanel() {
  const char* labels[7] = {"TAKBIR", "QIYAM", "RUKU'", "I'TIDAL", "SUJUD", "DUDUK", "TAHIYAT"};
  tft.fillRoundRect(212, 38, 100, 145, 8, COLOR_CARD);
  tft.drawRoundRect(212, 38, 100, 145, 8, COLOR_TEAL);
  if (animId >= 0 && animId < 7) {
    drawCenteredText(labels[animId], 214, 96, 162, 2, COLOR_GREEN, COLOR_CARD);
  }
  renderAnimPose(animCur);
  pushAnimFrame();
}

// Pilih animasi sesuai gerakan & tahap bacaan saat ini
int getBelajarAnimId() {
  switch (currentMotion) {
    case MOTION_STAND:   return (belajarAudioStep == 1) ? ANIM_TAKBIR : ANIM_QIYAM;
    case MOTION_RUKUK:   return ANIM_RUKUK;
    case MOTION_ITIDAL:  return ANIM_ITIDAL;
    case MOTION_SUJUD1:
    case MOTION_SUJUD2:  return ANIM_SUJUD;
    case MOTION_DUDUK:   return ANIM_DUDUK;
    case MOTION_TAHIYAT: return ANIM_TAHIYAT;
  }
  return ANIM_QIYAM;
}

void setBelajarAnimation(int id) {
  if (animCanvas == nullptr) {
    animCanvas = new GFXcanvas16(ANIM_W, ANIM_H);
    if (animCanvas->getBuffer() == nullptr) { delete animCanvas; animCanvas = nullptr; }   // memori tidak cukup
  }

  unsigned long now = millis();
  if (now - animLastSeen > 2000) {      // sesi belajar baru -> mulai dari berdiri
    animCur = POSE_DOWN;
    animId = -1;
  }
  animLastSeen = now;

  if (id != animId) {                   // gerakan berganti -> mulai animasi baru dari pose saat ini
    switch (id) {
      case ANIM_TAKBIR:  animKeys = animKeysTakbir;  animKeyCount = sizeof(animKeysTakbir)  / sizeof(AnimKey); animLoop = true;  break;
      case ANIM_QIYAM:   animKeys = animKeysQiyam;   animKeyCount = sizeof(animKeysQiyam)   / sizeof(AnimKey); animLoop = true;  break;
      case ANIM_RUKUK:   animKeys = animKeysRukuk;   animKeyCount = sizeof(animKeysRukuk)   / sizeof(AnimKey); animLoop = false; break;
      case ANIM_ITIDAL:  animKeys = animKeysItidal;  animKeyCount = sizeof(animKeysItidal)  / sizeof(AnimKey); animLoop = false; break;
      case ANIM_SUJUD:   animKeys = animKeysSujud;   animKeyCount = sizeof(animKeysSujud)   / sizeof(AnimKey); animLoop = false; break;
      case ANIM_DUDUK:   animKeys = animKeysDuduk;   animKeyCount = sizeof(animKeysDuduk)   / sizeof(AnimKey); animLoop = false; break;
      default:           animKeys = animKeysTahiyat; animKeyCount = sizeof(animKeysTahiyat) / sizeof(AnimKey); animLoop = false; break;
    }
    animId = id;
    animFrom = animCur;
    animKeyIdx = 0;
    animSegStart = now;
    animDone = false;
    animLastDraw = 0;
  }
  drawBelajarAnimationPanel();
}

// Dipanggil terus-menerus saat layar simulasi aktif (lihat checkAudioSequenceLoop)
void updateBelajarAnimation() {
  unsigned long now = millis();
  animLastSeen = now;
  if (animId < 0 || animDone || animCanvas == nullptr || animKeys == nullptr) return;
  if (now - animLastDraw < 50) return;      // maks +/- 20 fps
  animLastDraw = now;

  const AnimKey &k = animKeys[animKeyIdx];
  unsigned long el = now - animSegStart;
  if (el < k.moveMs) {
    float t = (float)el / (float)k.moveMs;
    t = t * t * (3.0f - 2.0f * t);          // gerak halus (ease in-out)
    lerpPose(animFrom, k.pose, t, animCur);
  } else {
    animCur = k.pose;
    if (el >= (unsigned long)k.moveMs + k.holdMs) {
      animFrom = k.pose;
      animSegStart = now;
      animKeyIdx++;
      if (animKeyIdx >= animKeyCount) {
        if (animLoop) animKeyIdx = 0;
        else { animKeyIdx = animKeyCount - 1; animDone = true; }
      }
    }
  }
  renderAnimPose(animCur);
  pushAnimFrame();
}

// Satu kotak status (label kecil di atas, nilai besar di tengah)
void drawStatusBox(int x, int w, const char* label, const String &value,
                   uint8_t valSize, uint16_t valColor, uint16_t borderColor) {
  const int boxY = 188, boxH = 44;
  tft.fillRoundRect(x, boxY, w, boxH, 6, COLOR_CARD_ACC);
  tft.drawRoundRect(x, boxY, w, boxH, 6, borderColor);
  drawCenteredText(label, x, w, boxY + 5, 1, COLOR_TEXT_DIM, COLOR_CARD_ACC);
  int valY = boxY + 16 + ((valSize == 3) ? 2 : 6);
  drawCenteredText(value, x, w, valY, valSize, valColor, COLOR_CARD_ACC);
}

void drawBelajarSimulationUI() {
  currentState = STATE_BELAJAR_SIMULASI;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 32, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  String headerText = selectedPrayerName + " (" + String(currentRakaat) + "/" + String(targetRakaat) + ")";
  int headerX = (320 - (headerText.length() * 12)) / 2;
  if (headerX < 70) headerX = 70;
  tft.setCursor(headerX, 8);
  tft.print(headerText);

  tft.fillRoundRect(8, 38, 200, 145, 8, COLOR_CARD);
  tft.drawRoundRect(8, 38, 200, 145, 8, COLOR_TEAL);

  tft.setTextColor(COLOR_CYAN, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(16, 44);
  tft.print("BACAAN: ");
  tft.setTextColor(COLOR_GREEN, COLOR_CARD);
  tft.print(currentBacaanTitle);

  tft.drawFastHLine(14, 56, 188, COLOR_TEAL);

  String teksShow = isTextArab ? currentBacaanContent.arab : currentBacaanContent.latin;
  drawWrappedText(teksShow, 16, 60, 184, 118, COLOR_WHITE, COLOR_CARD, 1);

  // Panel status bawah: 3 kotak terpisah, tulisan SUJUD diperbesar
  String sujudStr = String(currentSujud) + "/" + String(totalTargetSujud);
  String folderStr = String("#") + (currentAudioFolder < 10 ? "0" : "") + String(currentAudioFolder);
  String trackStr  = String("#") + (currentAudioTrack  < 10 ? "0" : "") + String(currentAudioTrack);
  drawStatusBox(8,   80,  "FOLDER", folderStr, 2, COLOR_ACCENT, COLOR_TEAL);
  drawStatusBox(96,  80,  "TRACK",  trackStr,  2, COLOR_CYAN,   COLOR_TEAL);
  drawStatusBox(184, 128, "SUJUD",  sujudStr,  3, COLOR_GREEN,  COLOR_GREEN);

  // Animasi gerakan sholat (sisi kanan)
  setBelajarAnimation(getBelajarAnimId());
}

void drawHasilSholatUI() {
  currentState = STATE_HASIL_SHOLAT;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(120, 10);
  tft.print("HASIL");

  tft.setTextColor(COLOR_GREEN, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(60, 65);
  tft.print("SHOLAT SELESAI!");

  tft.setTextColor(COLOR_WHITE, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(60, 105);
  tft.printf("Sholat      : %s", selectedPrayerName.c_str());
  tft.setCursor(60, 125);
  tft.printf("Total Rakaat: %d Rakaat", targetRakaat);
  tft.setCursor(60, 145);
  tft.printf("Total Sujud : %d Sujud", currentSujud);

  tft.fillRoundRect(80, 175, 160, 45, 8, COLOR_BLUE_BTN);
  tft.setTextColor(COLOR_WHITE, COLOR_BLUE_BTN);
  tft.setTextSize(2);
  tft.setCursor(115, 188);
  tft.print("SELESAI");
}

void drawBelajarSelesaiUI() {
  currentState = STATE_BELAJAR_SELESAI;
  tft.fillScreen(COLOR_BG);
  
  tft.fillRect(0, 0, 320, 35, COLOR_CARD);
  drawBackButton(); 

  tft.setTextColor(COLOR_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(75, 10);
  tft.print("BELAJAR SELESAI");

  tft.setTextColor(COLOR_GREEN, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(45, 70);
  tft.print("SIMULASI SELESAI!");

  tft.setTextColor(COLOR_WHITE, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(45, 110);
  tft.printf("Sholat   : %s (%d Rakaat)", selectedPrayerName.c_str(), targetRakaat);
  tft.setCursor(45, 130);
  tft.printf("Surah 1  : %s", listSurah[selectedSurahIndices[0]].c_str());
  tft.setCursor(45, 150);
  tft.printf("Surah 2  : %s", listSurah[selectedSurahIndices[1]].c_str());

  tft.fillRoundRect(80, 180, 160, 42, 8, COLOR_BLUE_BTN);
  tft.setTextColor(COLOR_WHITE, COLOR_BLUE_BTN);
  tft.setTextSize(2);
  tft.setCursor(105, 192);
  tft.print("KE MENU");
}

// ===== AUDIO KETERANGAN (MODE BELAJAR) =====
// Atur teks layar untuk track keterangan / salam yang diputar otomatis
void setTeksUntukTrack(int trackNo) {
  const char* judul = nullptr;
  switch (trackNo) {
    case TRK_KET_RUKU:         judul = "Ket. Ruku'"; break;
    case TRK_KET_ITIDAL:       judul = "Ket. I'tidal"; break;
    case TRK_KET_SUJUD:        judul = "Ket. Sujud"; break;
    case TRK_KET_DUDUK:        judul = "Ket. Duduk 2 Sujud"; break;
    case TRK_KET_SUJUD2:       judul = "Ket. Sujud Kedua"; break;
    case TRK_KET_TAHIYAT_AWAL: judul = "Ket. Tahiyat Awal"; break;
    case TRK_KET_TAHIYAT_AKHIR:judul = "Ket. Tahiyat Akhir"; break;
    case TRK_KET_SALAM:        judul = "Ket. Salam"; break;
    case TRK_KET_PENUTUP:      judul = "Ket. Penutup"; break;
  }
  if (judul != nullptr) {
    currentBacaanTitle = judul;
    currentBacaanContent = {
      "Dengarkan penjelasan gerakan berikutnya.",
      "Dengarkan penjelasan gerakan berikutnya."
    };
  } else if (trackNo == TRK_SALAM) {
    currentBacaanTitle = "Salam";
    currentBacaanContent = {
      "Assalamu'alaikum wa rahmatullah. (menoleh ke kanan, lalu ke kiri)",
      "السَّلَامُ عَلَيْكُمْ وَرَحْمَةُ اللَّهِ"
    };
  }
}

// Putar satu track folder 01 (keterangan / bacaan lanjutan) sambil memperbarui layar
void playKeterangan(int trackNo) {
  currentAudioFolder = 1;
  currentAudioTrack = trackNo;
  setTeksUntukTrack(trackNo);
  drawBelajarSimulationUI();
  startSequenceAudio(currentAudioFolder, currentAudioTrack);
}

// Putar track berikutnya dari antrean (dipanggil setelah audio sebelumnya selesai)
void playPendingNext() {
  if (pendingPos >= pendingLen) return;
  playKeterangan(pendingTracks[pendingPos++]);
}

// EKSEKUSI BACAAN & AUDIO DARI DETEKSI GERAKAN SENSOR
void updateBelajarAudioAndText() {
  clearPendingAudio(); // antrean lama dibuang tiap ganti gerakan
  switch (currentMotion) {
    case MOTION_STAND:
      if (belajarAudioStep == 0) {
        if (currentRakaat <= 2) {
          // RAKAAT 1 & 2: Takbiratul Ihram -> Al-Fatihah -> Surah Pendek
          belajarAudioStep = 1; 
          isProcessingSequence = true; // Kunci sekuensial audio
          
          currentAudioFolder = 1;
          currentAudioTrack = 1; // Track 0001
          
          currentBacaanTitle = "Takbiratul Ihram";
          currentBacaanContent.latin = "Allahu Akbar.";
          currentBacaanContent.arab = "اللهُ أَكْبَرُ كَبِيرًا وَالْحَمْدُ لِلَّهِ كَثِيرًا وَسُبْحَانَ اللهِ بُكْرَةً وَأَصِيلاً...";
          
          drawBelajarSimulationUI();
          startSequenceAudio(currentAudioFolder, currentAudioTrack);
        } else {
          // RAKAAT 3 & 4: Hanya Al-Fatihah (Track 0002)
          belajarAudioStep = 2; 
          isProcessingSequence = true; 
          
          currentAudioFolder = 1;
          currentAudioTrack = 2; // Track 0002 (Al-Fatihah)
          
          currentBacaanTitle = "Surah Al-Fatihah";
          currentBacaanContent = surahFatihah;
          
          drawBelajarSimulationUI();
          startSequenceAudio(currentAudioFolder, currentAudioTrack);
        }
      }
      break;

    case MOTION_RUKUK:
      if (lastPlayedFolder != 1 || lastPlayedTrack != TRK_RUKU) {
        belajarAudioStep = 0; 
        isProcessingSequence = false;
        currentAudioFolder = 1;
        currentAudioTrack = TRK_RUKU;
        setPendingAudio(TRK_KET_ITIDAL); // setelah doa ruku: keterangan menuju i'tidal
        currentBacaanTitle = "Bacaan Ruku'";
        currentBacaanContent = {
          "Subhana rabbiyal 'azhimi wa bihamdih. (3x)",
          "سُبْحَانَ رَبِّيَ الْعَظِيمِ وَبِحَمْدِهِ (3x)"
        };
        drawBelajarSimulationUI();
        startSequenceAudio(currentAudioFolder, currentAudioTrack);
      }
      break;

    case MOTION_ITIDAL:
      if (lastPlayedFolder != 1 || lastPlayedTrack != TRK_ITIDAL_FIRST) {
        belajarAudioStep = 0;
        isProcessingSequence = false;
        currentAudioFolder = 1;
        currentAudioTrack = TRK_ITIDAL_FIRST;
        if (TRK_ITIDAL_FIRST == TRK_ITIDAL) setPendingAudio(TRK_KET_SUJUD);
        else setPendingAudio(TRK_ITIDAL, TRK_KET_SUJUD); // 0006 -> 0007 -> keterangan sujud
        currentBacaanTitle = "Bacaan I'tidal";
        currentBacaanContent = {
          "Sami'allahu liman hamidah.\nRabbana lakal-hamdu mil'us-samawati wa mil'ul-ardi wa mil'u ma syi'ta min syai'in ba'du.",
          "سَمِعَ اللَّهُ لِمَنْ حَمِدَهُ\nرَبَّنَا لَكَ الْحَمْدُ مِلْءُ السَّمَاوَاتِ وَمِلْءُ الْأَرْضِ وَمِلْءُ مَا شِئْتَ مِنْ شَيْءٍ بَعْدُ"
        };
        drawBelajarSimulationUI();
        startSequenceAudio(currentAudioFolder, currentAudioTrack);
      }
      break;

    case MOTION_SUJUD1:
    case MOTION_SUJUD2:
      if (lastPlayedFolder != 1 || lastPlayedTrack != TRK_SUJUD) {
        belajarAudioStep = 0;
        isProcessingSequence = false;
        currentAudioFolder = 1;
        currentAudioTrack = TRK_SUJUD;
        if (currentMotion == MOTION_SUJUD1) {
          setPendingAudio(TRK_KET_DUDUK);            // lanjut: duduk di antara 2 sujud
        } else if (currentRakaat == 2 && targetRakaat > 2) {
          setPendingAudio(TRK_KET_TAHIYAT_AWAL);     // lanjut: tahiyat awal
        } else if (currentRakaat >= targetRakaat) {
          setPendingAudio(TRK_KET_TAHIYAT_AKHIR);    // lanjut: tahiyat akhir
        }                                            // rakaat 1/3: berdiri, tanpa keterangan
        currentBacaanTitle = "Bacaan Sujud";
        currentBacaanContent = {
          "Subhana rabbiyal a'la wa bihamdih. (3x)",
          "سُبْحَانَ رَبِّيَ الْأَعْلَىٰ وَبِحَمْدِهِ (3x)"
        };
        drawBelajarSimulationUI();
        startSequenceAudio(currentAudioFolder, currentAudioTrack);
      }
      break;

    case MOTION_DUDUK:
      if (lastPlayedFolder != 1 || lastPlayedTrack != TRK_DUDUK) {
        belajarAudioStep = 0;
        isProcessingSequence = false;
        currentAudioFolder = 1;
        currentAudioTrack = TRK_DUDUK;
        setPendingAudio(TRK_KET_SUJUD2);
        currentBacaanTitle = "Duduk di Antara 2 Sujud";
        currentBacaanContent = {
          "Rabbighfirlii warhamnii wajburnii warfa'nii warzuqnii wahdinii wa 'aafinii wa'fu 'annii.",
          "رَبِّ اغْفِرْ لِي وَارْحَمْنِي وَاجْبُرْنِي وَارْفَعْنِي وَارْزُقْنِي وَاهْدِ نِي وَعَافِني وَاعْفُ عَنِّي"
        };
        drawBelajarSimulationUI();
        startSequenceAudio(currentAudioFolder, currentAudioTrack);
      }
      break;

    case MOTION_TAHIYAT:
      {
        int targetTrack = (currentRakaat == 2 && targetRakaat > 2) ? TRK_TAHIYAT_AWAL : TRK_TAHIYAT_AKHIR;
        if (lastPlayedFolder != 1 || lastPlayedTrack != targetTrack) {
          belajarAudioStep = 0;
          isProcessingSequence = false;
          currentAudioFolder = 1;
          currentAudioTrack = targetTrack;
          if (targetTrack == TRK_TAHIYAT_AKHIR) setPendingAudio(TRK_KET_SALAM, TRK_SALAM, TRK_KET_PENUTUP);
          currentBacaanTitle = (targetTrack == TRK_TAHIYAT_AWAL) ? "Tahiyat Awal" : "Tahiyat Akhir";
          currentBacaanContent = {
            "At-tahiyyatur-mubarakatus-salawatut-tayyibatu lillah.\nAs-salamu 'alaika ayyuhan-nabiyyu wa rahmatullahi wa barakatuh.\nAs-salamu 'alaina wa 'ala 'ibadillahis-salihin.\nAsyhadu alla ilaha illallah, wa asyhadu anna Muhammadar Rasullullah...",
            "التَّحِيَّاتُ الْمُبَارَكَاتُ الصَّلَوَاتُ الطَّيِّبَاتُ لِلَّهِ..."
          };
          drawBelajarSimulationUI();
          startSequenceAudio(currentAudioFolder, currentAudioTrack);
        }
      }
      break;
  }
}

// LOGIKA OTOMATIS SEQUENCER AUDIO SAAT POSISI BERDIRI
void checkAudioSequenceLoop() {
  // Gerakkan animasi gerakan sholat (hanya tampilan; dipanggil terus saat mode belajar aktif)
  if (currentState == STATE_BELAJAR_SIMULASI) updateBelajarAnimation();

  if (currentState != STATE_BELAJAR_SIMULASI) return;

  // Selain berdiri: setelah audio gerakan selesai, putar antrean keterangan/bacaan lanjutan
  if (currentMotion != MOTION_STAND) {
    if (pendingPos < pendingLen && !isAudioPlaying()) playPendingNext();
    return;
  }

  // Jika DFPlayer sedang aktif memutar audio, tunggu sampai selesai
  if (isAudioPlaying()) {
    return; 
  }

  // Jika audio telah selesai diputar dan pengunci sekuensial aktif
  if (isProcessingSequence) {
    if (currentRakaat <= 2) {

      // LANGKAH 1: Selesai Audio 0001 (Takbir) -> Lanjut Audio 0002 (Al-Fatihah)
      if (belajarAudioStep == 1) {
        belajarAudioStep = 2;
        currentAudioFolder = 1;
        currentAudioTrack = 2;

        currentBacaanTitle = "Surah Al-Fatihah";
        currentBacaanContent = surahFatihah;

        drawBelajarSimulationUI();
        startSequenceAudio(currentAudioFolder, currentAudioTrack);
      } 
      // LANGKAH 2: Selesai Audio 0002 (Al-Fatihah) -> Lanjut Audio Surah Pendek (Folder 02)
      else if (belajarAudioStep == 2) {
        belajarAudioStep = 3;
        
        int surahIndex = selectedSurahIndices[currentRakaat - 1]; 
        BacaanText surahDetail = getSurahContent(surahIndex);

        currentAudioFolder = 2;              
        currentAudioTrack = surahIndex + 1; // Track sesuai index pilihan user

        currentBacaanTitle = listSurah[surahIndex];
        currentBacaanContent = surahDetail;

        drawBelajarSimulationUI();
        startSequenceAudio(currentAudioFolder, currentAudioTrack);
      }
      // LANGKAH 3: Selesai Surah Pendek -> Putar KETERANGAN sebelum ruku (track 0003)
      else if (belajarAudioStep == 3) {
        belajarAudioStep = 4;
        playKeterangan(TRK_KET_RUKU);
      }
      // LANGKAH 4: Selesai keterangan -> Buka kunci agar sensor dapat membaca gerakan rukuk
      else if (belajarAudioStep == 4) {
        belajarAudioStep = 5;
        isProcessingSequence = false;
      }

    } else {
      // RAKAAT 3 & 4: Selesai Al-Fatihah (Track 0002) -> Putar KETERANGAN sebelum ruku (track 0003)
      if (belajarAudioStep == 2) {
        belajarAudioStep = 4;
        playKeterangan(TRK_KET_RUKU);
      }
      // Selesai keterangan -> Buka kunci
      else if (belajarAudioStep == 4) {
        belajarAudioStep = 5;
        isProcessingSequence = false;
      }
    }
  }
}

void handleTouchEvents(int tx, int ty) {
  if (tx >= 8 && tx <= 70 && ty >= 0 && ty <= 35) {
    switch (currentState) {
      case STATE_PILIH_SHOLAT:
      case STATE_BELAJAR_PILIH_SHOLAT:
      case STATE_VOLUME_SETTING:
      case STATE_HASIL_SHOLAT:
      case STATE_BELAJAR_SELESAI:
        resetAudioState();
        drawMainMenu();
        return;

      case STATE_BELAJAR_PILIH_SURAH:
        drawBelajarPilihSholatMenu();
        return;

      case STATE_PILIH_JENIS_BACAAN:
        drawPilihSurahMenu();
        return;

      case STATE_KALIBRASI:
        drawPilihSholatMenu();
        return;

      case STATE_BELAJAR_KALIBRASI:
        drawPilihJenisBacaanMenu();
        return;

      case STATE_DETEKSI_SHOLAT:
        drawPilihSholatMenu();
        return;

      case STATE_BELAJAR_SIMULASI:
        resetAudioState();
        belajarAudioStep = 0;
        isProcessingSequence = false;
        drawPilihJenisBacaanMenu();
        return;

      case STATE_BANTUAN_MENU:
        drawMainMenu();
        return;

      case STATE_BANTUAN_SLIDE:
        drawBantuanMenu();
        return;

      default:
        break;
    }
  }

  if (currentState == STATE_PEMBUKA) {
    // Tombol MULAI -> Menu Utama
    if (tx >= 80 && tx <= 240 && ty >= 176 && ty <= 234) {
      drawMainMenu();
    }
  }
  else if (currentState == STATE_MAIN_MENU) {
    if (tx >= 30 && tx <= 290) {
      if (ty >= 44 && ty <= 84) {
        isBelajarMode = false;
        drawPilihSholatMenu();
      } else if (ty >= 90 && ty <= 130) {
        isBelajarMode = true;
        drawBelajarPilihSholatMenu();
      } else if (ty >= 136 && ty <= 176) {
        drawVolumeMenu();
      } else if (ty >= 182 && ty <= 222) {
        drawBantuanMenu();
      }
    }
  }
  else if (currentState == STATE_BANTUAN_MENU) {
    if (tx >= 30 && tx <= 290) {
      for (int i = 0; i < 3; i++) {
        int y = 48 + (i * 50);
        if (ty >= y && ty <= (y + 42)) {
          bantuanTopic = i;
          bantuanSlideIdx = 0;
          drawBantuanSlide(true);
          return;
        }
      }
      if (ty >= 200 && ty <= 234) {
        drawMainMenu();   // KE MENU UTAMA
      }
    }
  }
  else if (currentState == STATE_BANTUAN_SLIDE) {
    int count = 0;
    getHelpSlides(bantuanTopic, count);
    if (ty >= 196 && ty <= 238) {
      if (tx >= 8 && tx <= 112) {
        // SEBELUM (tidak aktif di slide pertama)
        if (bantuanSlideIdx > 0) {
          bantuanSlideIdx--;
          drawBantuanSlide(false);
        }
      } else if (tx >= 208 && tx <= 312) {
        if (bantuanSlideIdx < count - 1) {
          bantuanSlideIdx++;          // LANJUT
          drawBantuanSlide(false);
        } else {
          drawBantuanMenu();          // SELESAI -> kembali ke daftar bantuan
        }
      }
    }
  }

  else if (currentState == STATE_VOLUME_SETTING) {
    // ---- VOLUME (baris tombol y = 68..112) ----
    if (ty >= 64 && ty <= 116) {
      if (tx >= 40 && tx <= 100) {            // Minus
        if (globalVolume > 0) {
          globalVolume--;
          myDFPlayer.volume(globalVolume);
          drawVolumeMenu();
        }
      } else if (tx >= 220 && tx <= 280) {    // Plus
        if (globalVolume < 30) {
          globalVolume++;
          myDFPlayer.volume(globalVolume);
          drawVolumeMenu();
        }
      }
    }
    // ---- KECERAHAN (baris tombol y = 166..210) ----
    else if (ty >= 162 && ty <= 214) {
      if (tx >= 40 && tx <= 100) {            // Minus
        if (globalBrightness > 10) {
          globalBrightness -= 10;
          applyBrightness();
          drawVolumeMenu();
        }
      } else if (tx >= 220 && tx <= 280) {    // Plus
        if (globalBrightness < 100) {
          globalBrightness += 10;
          applyBrightness();
          drawVolumeMenu();
        }
      }
    }
  }
  else if (currentState == STATE_PILIH_SHOLAT) {
    if (tx >= 20 && tx <= 300) {
      for (int i = 0; i < 5; i++) {
        int yPos = 42 + (i * 38);
        if (ty >= yPos && ty <= (yPos + 32)) {
          if (i == 0)      { selectedPrayerName = "SUBUH";   targetRakaat = 2; }
          else if (i == 1) { selectedPrayerName = "DZUHUR";  targetRakaat = 4; }
          else if (i == 2) { selectedPrayerName = "ASHAR";   targetRakaat = 4; }
          else if (i == 3) { selectedPrayerName = "MAGHRIB"; targetRakaat = 3; }
          else if (i == 4) { selectedPrayerName = "ISYA";    targetRakaat = 4; }

          currentRakaat = 1;
          currentSujud = 0;
          totalTargetSujud = targetRakaat * 2;
          currentMotion = MOTION_STAND;
          belajarAudioStep = 0;
          isProcessingSequence = false;

          runCalibrationProcess();
          break;
        }
      }
    }
  }
  else if (currentState == STATE_BELAJAR_PILIH_SHOLAT) {
    if (tx >= 20 && tx <= 300) {
      for (int i = 0; i < 5; i++) {
        int yPos = 42 + (i * 38);
        if (ty >= yPos && ty <= (yPos + 32)) {
          if (i == 0)      { selectedPrayerName = "SUBUH";   targetRakaat = 2; }
          else if (i == 1) { selectedPrayerName = "DZUHUR";  targetRakaat = 4; }
          else if (i == 2) { selectedPrayerName = "ASHAR";   targetRakaat = 4; }
          else if (i == 3) { selectedPrayerName = "MAGHRIB"; targetRakaat = 3; }
          else if (i == 4) { selectedPrayerName = "ISYA";    targetRakaat = 4; }

          for (int s = 0; s < 7; s++) selectedSurah[s] = false;
          selectedSurahCount = 0;

          drawPilihSurahMenu();
          break;
        }
      }
    }
  }
  else if (currentState == STATE_BELAJAR_PILIH_SURAH) {
    for (int i = 0; i < 7; i++) {
      int col = i % 2;
      int row = i / 2;
      int xPos = (col == 0) ? 10 : 165;
      int yPos = 38 + (row * 36);
      int cardW = 145;
      int cardH = 32;

      if (tx >= xPos && tx <= (xPos + cardW) && ty >= yPos && ty <= (yPos + cardH)) {
        if (selectedSurah[i]) {
          selectedSurah[i] = false;
          selectedSurahCount--;
        } else {
          if (selectedSurahCount < 2) {
            selectedSurah[i] = true;
            selectedSurahCount++;
          }
        }
        drawPilihSurahMenu();
        return;
      }
    }

    if (tx >= 190 && tx <= 310 && ty >= 188 && ty <= 230) {
      if (selectedSurahCount == 2) {
        int idx = 0;
        for (int s = 0; s < 7; s++) {
          if (selectedSurah[s]) {
            selectedSurahIndices[idx++] = s;
          }
        }
        drawPilihJenisBacaanMenu();
      }
    }
  }
  else if (currentState == STATE_PILIH_JENIS_BACAAN) {
    if (tx >= 190 && tx <= 310 && ty >= 192 && ty <= 230) {
      isTextArab = false;
      currentState = STATE_BELAJAR_KALIBRASI;
      belajarAudioStep = 0;
      isProcessingSequence = false;
      resetAudioState();
      runBelajarCalibrationProcess();
    }
  }
  else if (currentState == STATE_DETEKSI_SHOLAT) {
    if (tx >= 10 && tx <= 310 && ty >= 190 && ty <= 230) { 
      resetAudioState();
      drawMainMenu();
    }
  }
  else if (currentState == STATE_BELAJAR_SIMULASI) {
    if (tx >= 10 && tx <= 310 && ty >= 190 && ty <= 230) { 
      resetAudioState();
      belajarAudioStep = 0;
      isProcessingSequence = false;
      drawMainMenu();
    }
  }
  else if (currentState == STATE_HASIL_SHOLAT || currentState == STATE_BELAJAR_SELESAI) {
    if (tx >= 80 && tx <= 240 && ty >= 175) {
      resetAudioState();
      belajarAudioStep = 0;
      isProcessingSequence = false;
      drawMainMenu();
    }
  }
}