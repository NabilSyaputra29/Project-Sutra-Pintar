#ifndef CONFIG_H
#define CONFIG_H

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <DFRobotDFPlayerMini.h>

// PIN HARDWARE TFT (HSPI)
#define TFT_CS    15  
#define TFT_DC     2  
#define TFT_RST    4  
#define TFT_MOSI  13  
#define TFT_CLK   14  
#define TFT_MISO  12  
#define TFT_LED   21  

// PIN HARDWARE TOUCHSCREEN (VSPI)
#define TOUCH_CS   33 
#define TOUCH_IRQ  36 
#define TOUCH_MOSI 32 
#define TOUCH_CLK  25 
#define TOUCH_MISO 39 

// PIN DFPLAYER MINI BUSY (Gunakan GPIO 26 agar tidak bentrok dengan TFT_RST)
#define PIN_DFPLAYER_BUSY 26 

#define TRIG_PIN_1  18  // Sensor Atas
#define ECHO_PIN_1  19
#define TRIG_PIN_2  22  // Sensor Bawah
#define ECHO_PIN_2  23

// TRACK AUDIO FOLDER 01 (urutan sesuai nama file di SD card)
// KET = audio keterangan (penjelasan sebelum gerakan), selain itu = bacaan
#define TRK_TAKBIR            1   // 0001 Allahu Akbar
#define TRK_FATIHAH           2   // 0002 Al-Fatihah
#define TRK_KET_RUKU          3   // 0003 keterangan sebelum ruku
#define TRK_RUKU              4   // 0004 doa ruku
#define TRK_KET_ITIDAL        5   // 0005 keterangan sebelum i'tidal
#define TRK_ITIDAL_AWAL       6   // 0006 bangun dari ruku (Sami'allahu liman hamidah)
#define TRK_ITIDAL            7   // 0007 doa bangun dari ruku (Rabbana lakal hamd)
#define TRK_KET_SUJUD         8   // 0008 keterangan sebelum sujud
#define TRK_SUJUD             9   // 0009 doa sujud
#define TRK_KET_DUDUK         10  // 0010 keterangan sebelum duduk antara 2 sujud
#define TRK_DUDUK             11  // 0011 doa duduk iftirosy
#define TRK_KET_SUJUD2        12  // 0012 keterangan sebelum sujud kedua
#define TRK_KET_TAHIYAT_AWAL  13  // 0013 keterangan sebelum tahiyat awal
#define TRK_TAHIYAT_AWAL      14  // 0014 doa tahiyat awal
#define TRK_KET_TAHIYAT_AKHIR 15  // 0015 keterangan sebelum tahiyat akhir
#define TRK_TAHIYAT_AKHIR     16  // 0016 doa tahiyat akhir
#define TRK_KET_SALAM         17  // 0017 keterangan sebelum salam
#define TRK_SALAM             18  // 0018 salam selesai sholat
#define TRK_KET_PENUTUP       19  // 0019 keterangan penutup

// 1 = I'tidal memutar track 0006 lalu 0007. 0 = hanya 0007 (perilaku lama).
#define ITIDAL_PUTAR_TRACK6   1
#if ITIDAL_PUTAR_TRACK6
  #define TRK_ITIDAL_FIRST TRK_ITIDAL_AWAL
#else
  #define TRK_ITIDAL_FIRST TRK_ITIDAL
#endif

#define SCREEN_W  320
#define SCREEN_H  240

// KALIBRASI TOUCHSCREEN
#define TS_MINX 320
#define TS_MAXX 3850
#define TS_MINY 350
#define TS_MAXY 3800

// WARNA UI
#define COLOR_BG        0x0821 
#define COLOR_CARD      0x18A4 
#define COLOR_CARD_ACC  0x2128 
#define COLOR_CYAN      0x07FF 
#define COLOR_TEAL      0x03E9 
#define COLOR_WHITE     0xFFFF 
#define COLOR_TEXT_DIM  0x9E79 
#define COLOR_RED       0xE082 
#define COLOR_GREEN     0x2604 
#define COLOR_BLUE_BTN  0x2B9D 
#define COLOR_ACCENT    0xF500 
#define COLOR_DISABLED  0x7BEF 

// ENUM STATE
enum AppState {
  STATE_SPLASH,
  STATE_MAIN_MENU,
  STATE_PILIH_SHOLAT,
  STATE_KALIBRASI,
  STATE_DETEKSI_SHOLAT,
  STATE_HASIL_SHOLAT,
  STATE_BELAJAR_PILIH_SHOLAT,
  STATE_BELAJAR_PILIH_SURAH,   
  STATE_PILIH_JENIS_BACAAN,  
  STATE_BELAJAR_KALIBRASI,
  STATE_BELAJAR_SIMULASI,
  STATE_BELAJAR_SELESAI,
  STATE_VOLUME_SETTING,  // Perbaikan: ditambahkan agar tidak undeclared
  STATE_PEMBUKA,         // Halaman pembuka (setelah loading, sebelum menu utama)
  STATE_BANTUAN_MENU,    // Menu bantuan (3 pilihan)
  STATE_BANTUAN_SLIDE    // Penjelasan bantuan per slide
};

enum PrayerMotion {
  MOTION_STAND,
  MOTION_RUKUK,
  MOTION_ITIDAL,
  MOTION_SUJUD1,
  MOTION_DUDUK,
  MOTION_SUJUD2,
  MOTION_TAHIYAT
};

// STRUKTUR TEXT BACAAN
struct BacaanText {
  String latin;
  String arab;
};

// STRUKTUR SLIDE MENU BANTUAN
// (Harus di Config.h, bukan di .ino, agar dikenali oleh prototype otomatis Arduino IDE)
struct HelpSlide {
  const char* head;   // judul slide (maks 24 karakter)
  const char* body;   // isi slide (maks 6 baris x 23 karakter)
};

// STRUKTUR ANIMASI GERAKAN SHOLAT (MODE BELAJAR)
// Satu pose = posisi pinggul, sudut kaki/badan/kepala, dan target tangan (sisi samping, menghadap kanan)
struct AnimPose {
  float hipX, hipY;       // posisi pinggul pada kanvas animasi
  float thigh, shin;      // sudut paha & betis (derajat, 0 = lurus ke bawah, + = ke depan)
  float torso, head;      // sudut badan & kepala (derajat, 0 = tegak, + = condong ke depan)
  float handX, handY;     // target posisi tangan
  float bend;             // arah tekuk siku (+1 / -1)
  float finger;           // 1 = jari telunjuk diangkat (tahiyat)
};
struct AnimKey {
  AnimPose pose;          // pose tujuan
  uint16_t moveMs;        // lama perpindahan menuju pose ini
  uint16_t holdMs;        // lama menahan pose ini
};

// DEKLARASI GLOBAL
extern SPIClass hspi;
extern SPIClass vspi;
extern Adafruit_ST7789 tft;
extern XPT2046_Touchscreen touch;
extern HardwareSerial dfSerial;
extern DFRobotDFPlayerMini myDFPlayer;

extern AppState currentState;
extern PrayerMotion currentMotion;

extern int touchX, touchY;
extern unsigned long lastTouchTime;
extern int currentVolume;

// VARIABEL SHOLAT & SURAH
extern String selectedPrayerName;
extern int targetRakaat;
extern int currentRakaat;
extern int currentSujud;     
extern int totalTargetSujud; 

extern bool isBelajarMode;
extern bool selectedSurah[7];
extern int selectedSurahIndices[2]; 
extern int selectedSurahCount;
extern bool isTextArab; 

// SINKRONISASI BACAAN & AUDIO SIMULASI
extern int currentAudioFolder;
extern int currentAudioTrack;
extern int belajarAudioStep;
extern String currentBacaanTitle;
extern BacaanText currentBacaanContent;

// ANTREAN AUDIO LANJUTAN MODE BELAJAR (folder 01): diputar berurutan setelah audio gerakan selesai
extern int pendingTracks[4];
extern int pendingLen;
extern int pendingPos;

// TRACKER AUDIO PENAHAN (AGAR TIDAK REPEAT/LONCAT)
extern int lastPlayedFolder;
extern int lastPlayedTrack;

// BASELINE SENSOR & SMOOTHING
extern long baseDistAtas;
extern long baseDistBawah;
extern float smoothedAtas;
extern float smoothedBawah;

// TIMING & COOLDOWN
extern unsigned long standStartTime; 
extern const unsigned long STAND_COOLDOWN_MS;

extern unsigned long rukukStartTime; 
extern const unsigned long RUKUK_MIN_MS;

extern unsigned long sujud2StartTime;
extern const unsigned long SUJUD_MIN_MS;

extern unsigned long tahiyatStartTime;
extern const unsigned long TAHIYAT_MIN_MS;

// PROTOTYPE FUNGSI
void initUltrasonicPins();
long getMedianDistance(int trigPin, int echoPin);
long getRobustBaseline(int trigPin, int echoPin);
void runCalibrationProcess();
void runBelajarCalibrationProcess();
void processPrayerFSM();
void processBelajarSimulationFSM();

void initDFPlayer();
void playAudioGuide(int folderNo, int trackNo);
void stopAudioGuide();
void triggerAudioOnce(int folderNo, int trackNo, bool forcePlay = false);
void resetAudioState();
void checkAudioSequenceLoop();
void clearPendingAudio();
void setPendingAudio(int t1, int t2 = 0, int t3 = 0);
bool isAudioPlaying();

bool getTouchCoordinates(int &outX, int &outY);
void drawBackButton();
void initBrightness();
void applyBrightness();
void drawSplashScreen();
void drawOpeningPage();
void drawBantuanMenu();
void drawBantuanSlide(bool fullRedraw = true);
void drawMainMenu();
void drawPilihSholatMenu();
void drawBelajarPilihSholatMenu();
void drawPilihSurahMenu();
void drawPilihJenisBacaanMenu();
void drawCalibrationScreen(String textStatus = "Mohon Diam di Posisi Standby!");
void drawPrayerDetectionUI();
void drawBelajarSimulationUI();
void drawHasilSholatUI();
void drawBelajarSelesaiUI();

void updateBelajarAudioAndText();
void drawWrappedText(String text, int x, int y, int maxW, int maxH, uint16_t color, uint16_t bg, uint8_t textSize);
void handleTouchEvents(int tx, int ty);

#endif