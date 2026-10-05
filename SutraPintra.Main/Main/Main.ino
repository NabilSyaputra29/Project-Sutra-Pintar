#include "Config.h"

SPIClass hspi = SPIClass(HSPI);
SPIClass vspi = SPIClass(VSPI);

Adafruit_ST7789 tft = Adafruit_ST7789(&hspi, TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

HardwareSerial dfSerial(2); 
DFRobotDFPlayerMini myDFPlayer;

AppState currentState = STATE_SPLASH;
PrayerMotion currentMotion = MOTION_STAND;

int touchX = 0, touchY = 0;
unsigned long lastTouchTime = 0;
int currentVolume = 20;

String selectedPrayerName = "SUBUH";
int targetRakaat = 2;
int currentRakaat = 1;
int currentSujud = 0;
int totalTargetSujud = 4;

// VARIABEL MODE BELAJAR
bool isBelajarMode = false;
bool selectedSurah[7] = {false, false, false, false, false, false, false};
int selectedSurahIndices[2] = {0, 1}; 
int selectedSurahCount = 0;
bool isTextArab = false; 

int currentAudioFolder = 1;
int currentAudioTrack = 1;
String currentBacaanTitle = "";
BacaanText currentBacaanContent = {"", ""};

// VARIABEL PENAHAN AUDIO (PREVENT AUDIO REPEAT/JUMP)
int lastPlayedFolder = -1;
int lastPlayedTrack = -1;

// ANTREAN AUDIO LANJUTAN (keterangan gerakan berikutnya, salam, dst.)
int pendingTracks[4] = {0, 0, 0, 0};
int pendingLen = 0;
int pendingPos = 0;

// BASELINE SENSOR & SMOOTHING
long baseDistAtas = 100;
long baseDistBawah = 100;
float smoothedAtas = 100.0;
float smoothedBawah = 100.0;

// TIMING COOLDOWN
unsigned long standStartTime = 0; 
const unsigned long STAND_COOLDOWN_MS = 6000; 

unsigned long rukukStartTime = 0; 
const unsigned long RUKUK_MIN_MS = 3000;       

unsigned long sujud2StartTime = 0;
const unsigned long SUJUD_MIN_MS = 2500;     

unsigned long tahiyatStartTime = 0;
const unsigned long TAHIYAT_MIN_MS = 5000;   

void setup() {
  Serial.begin(115200);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  pinMode(PIN_DFPLAYER_BUSY, INPUT_PULLUP);

  initUltrasonicPins();

  hspi.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init(240, 320); 
  tft.setRotation(1); 

  vspi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(vspi);
  touch.setRotation(1);

  initDFPlayer(); 

  // Loading animasi -> Halaman pembuka. Menu utama dibuka lewat tombol MULAI (lihat handleTouchEvents)
  drawSplashScreen();
}

void loop() {
  if (getTouchCoordinates(touchX, touchY)) {
    if (millis() - lastTouchTime > 250) {
      handleTouchEvents(touchX, touchY);
      lastTouchTime = millis();
    }
  }

  if (currentState == STATE_DETEKSI_SHOLAT) {
    processPrayerFSM();
  }
  else if (currentState == STATE_BELAJAR_SIMULASI) {
    processBelajarSimulationFSM();
    checkAudioSequenceLoop(); // Memantau perpindahan audio otomatis
  }
}

// Fungsi Penahan Pemutaran Audio
void triggerAudioOnce(int folderNo, int trackNo, bool forcePlay) {
  if (forcePlay || folderNo != lastPlayedFolder || trackNo != lastPlayedTrack) {
    lastPlayedFolder = folderNo;
    lastPlayedTrack = trackNo;
    playAudioGuide(folderNo, trackNo);
  }
}
