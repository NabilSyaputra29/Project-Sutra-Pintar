#include "Config.h"

void initUltrasonicPins() {
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
}

long getMedianDistance(int trigPin, int echoPin) {
  const int numSamples = 5; 
  long samples[numSamples];

  for (int i = 0; i < numSamples; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 25000); 
    if (duration == 0) samples[i] = 400; 
    else samples[i] = (duration * 0.034) / 2;
    delay(3);
  }

  for (int i = 0; i < numSamples - 1; i++) {
    for (int j = i + 1; j < numSamples; j++) {
      if (samples[i] > samples[j]) {
        long temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }
  return samples[numSamples / 2]; 
}

long getRobustBaseline(int trigPin, int echoPin) {
  const int TOTAL_SAMPLES = 15;
  long validSamples[TOTAL_SAMPLES];
  int collected = 0;

  while (collected < TOTAL_SAMPLES) {
    long reading = getMedianDistance(trigPin, echoPin);
    if (reading >= 10 && reading <= 250) {
      validSamples[collected] = reading;
      collected++;
    }
    delay(50);
  }

  for (int i = 0; i < TOTAL_SAMPLES - 1; i++) {
    for (int j = i + 1; j < TOTAL_SAMPLES; j++) {
      if (validSamples[i] > validSamples[j]) {
        long temp = validSamples[i];
        validSamples[i] = validSamples[j];
        validSamples[j] = temp;
      }
    }
  }

  long sum = 0;
  for (int i = 3; i < TOTAL_SAMPLES - 3; i++) {
    sum += validSamples[i];
  }

  return sum / (TOTAL_SAMPLES - 6);
}

void runCalibrationProcess() {
  drawCalibrationScreen("Mohon Diam di Posisi Standby!");
  delay(1500);

  drawCalibrationScreen("Mengukur Baseline...");

  baseDistAtas = getRobustBaseline(TRIG_PIN_1, ECHO_PIN_1);
  baseDistBawah = getRobustBaseline(TRIG_PIN_2, ECHO_PIN_2);

  smoothedAtas = baseDistAtas;
  smoothedBawah = baseDistBawah;

  currentRakaat = 1;
  currentSujud = 0; 
  totalTargetSujud = targetRakaat * 2; 
  currentMotion = MOTION_STAND;
  standStartTime = millis(); 

  drawCalibrationScreen("Kalibrasi Selesai!");
  delay(1000);

  currentState = STATE_DETEKSI_SHOLAT;
  drawPrayerDetectionUI();
}


void processPrayerFSM() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 200) return; 
  lastCheck = millis();

  long rawAtas = getMedianDistance(TRIG_PIN_1, ECHO_PIN_1);
  long rawBawah = getMedianDistance(TRIG_PIN_2, ECHO_PIN_2);

  smoothedAtas = (0.6 * rawAtas) + (0.4 * smoothedAtas);
  smoothedBawah = (0.6 * rawBawah) + (0.4 * smoothedBawah);

  long dAtas = (long)smoothedAtas;
  long dBawah = (long)smoothedBawah;

  PrayerMotion prevMotion = currentMotion;

  switch (currentMotion) {
    case MOTION_STAND:
      if (millis() - standStartTime > STAND_COOLDOWN_MS) {
        if (dAtas < (baseDistAtas * 0.80) && dAtas > (baseDistAtas * 0.30)) {
          currentMotion = MOTION_RUKUK;
          rukukStartTime = millis(); 
        }
      }
      break;

    case MOTION_RUKUK:
      if (millis() - rukukStartTime > RUKUK_MIN_MS) {
        if (dAtas >= (baseDistAtas * 0.85)) {
          currentMotion = MOTION_ITIDAL;
        }
      }
      break;

    case MOTION_ITIDAL:
      if (dBawah < (baseDistBawah * 0.40)) {
        currentMotion = MOTION_SUJUD1;
        currentSujud++; 
      }
      break;

    case MOTION_SUJUD1:
      if (dBawah >= (baseDistBawah * 0.40) && dBawah < (baseDistBawah * 0.70)) {
        currentMotion = MOTION_DUDUK;
      }
      break;

    case MOTION_DUDUK:
      if (dBawah < (baseDistBawah * 0.40)) {
        currentMotion = MOTION_SUJUD2;
        currentSujud++; 
        sujud2StartTime = millis(); 
      }
      break;

    case MOTION_SUJUD2:
      if (millis() - sujud2StartTime > SUJUD_MIN_MS) {
        if (currentRakaat == 2 || currentRakaat >= targetRakaat) {
          if (dBawah >= (baseDistBawah * 0.40)) {
            currentMotion = MOTION_TAHIYAT;
            tahiyatStartTime = millis(); 
          }
        } else {
          if (dBawah >= (baseDistBawah * 0.80)) {
            currentRakaat++;
            currentMotion = MOTION_STAND;
            standStartTime = millis(); 
          }
        }
      }
      break;

    case MOTION_TAHIYAT:
      if (currentRakaat == 2 && targetRakaat > 2) {
        if (millis() - tahiyatStartTime > TAHIYAT_MIN_MS) {
          if (dBawah >= (baseDistBawah * 0.85)) {
            currentRakaat++;
            currentMotion = MOTION_STAND;
            standStartTime = millis(); 
          }
        }
      } else {
        currentState = STATE_HASIL_SHOLAT;
        drawHasilSholatUI();
        return;
      }
      break;
  }

  if (prevMotion != currentMotion) {
    drawPrayerDetectionUI();
  }
}

// =====================================================================
//  MODE BELAJAR - PEMBACAAN GERAKAN (dibuat lebih stabil, anti "loncat")
//  Mode SHOLAT (processPrayerFSM) TIDAK diubah sama sekali.
//  Ambang batas (0.80 / 0.30 / 0.85 / 0.40 / 0.80) sama persis dengan mode sholat.
//
//  Yang ditambahkan khusus mode belajar:
//   1. Pembacaan sensor lebih bersih: timeout dibuang (bukan dianggap 400 cm),
//      median 3 siklus untuk membuang lonjakan sesaat, lalu penghalusan.
//   2. Konfirmasi: syarat pindah gerakan harus terpenuhi BERUNTUN beberapa siklus,
//      bukan hanya 1x baca (satu bacaan nyasar tidak bisa lagi memindah gerakan).
//   3. Waktu tahan minimal tiap gerakan (rukuk, i'tidal, sujud, duduk, tahiyat).
//   4. Kunci berdiri: selama audio takbir/Fatihah/surah/keterangan sebelum rukuk
//      masih berjalan, gerakan rukuk tidak dibaca (sesuai maksud flag
//      isProcessingSequence). Matikan dengan BLJ_KUNCI_BERDIRI 0.
//   5. Urutan gerakan selalu berurutan, tidak mungkin melompat/melewati gerakan.
//   6. Tahiyat akhir: selesai hanya setelah doa, salam, dan penutup selesai diputar.
// =====================================================================

extern bool isProcessingSequence;  // didefinisikan di UI.ino

#define BELAJAR_DEBUG      1       // 1 = tampilkan log sensor di Serial Monitor (115200), 0 = matikan
#define BLJ_KUNCI_BERDIRI  1       // 1 = rukuk tidak dibaca selama audio awal berdiri masih berjalan

static const unsigned long BLJ_CYCLE_MS       = 200;     // jarak antar siklus baca sensor
static const int           BLJ_CONFIRM_CYCLES = 4;       // syarat harus benar beruntun sekian siklus (~0.8 detik)
static const int           BLJ_RUKUK_CONFIRM  = 4;       // rukuk: syarat harus terkumpul 4 siklus (~0.8 detik), toleran 1x goyang
static const float         BLJ_RUKUK_MIN_R    = 0.15f;   // batas bawah rasio rukuk (dulu 0.30; badan dekat sensor tidak lagi gagal)
static const float         BLJ_RUKUK_DELTA    = 0.15f;   // rukuk juga terbaca jika turun segini dari tinggi berdiri asli pengguna
static const unsigned long BLJ_STAND_LOCK_MAX_MS = 150000; // pengaman: kunci berdiri dilepas paksa setelah 2,5 menit (audio macet)
static const int           BLJ_INVALID_MAX    = 5;       // tanpa pantulan sekian siklus beruntun -> dianggap area kosong (400 cm)
static const unsigned long BLJ_ITIDAL_MIN_MS  = 1000;    // tahan minimal i'tidal
static const unsigned long BLJ_SUJUD1_MIN_MS  = 2000;    // tahan minimal sujud pertama
static const unsigned long BLJ_DUDUK_MIN_MS   = 1500;    // tahan minimal duduk di antara 2 sujud
static const unsigned long BLJ_LOCK_MAX_MS    = 600000;  // pengaman: kunci dilepas paksa setelah 10 menit (audio macet)
static const unsigned long BLJ_AUDIO_END_MAX  = 600000;  // pengaman: tahiyat akhir selesai paksa jika audio macet

// --- Pembacaan sensor bawah yang lebih teliti (hanya mode belajar) ---
// Gerakan keluar dari sujud / duduk / tahiyat baru dibaca jika:
//   (a) bacaan + keterangan gerakan itu SUDAH SELESAI diputar, dan
//   (b) posisi benar-benar BERUBAH dari posisi diam sebelumnya (level stabil / "plateau"),
//   (c) nilainya masuk rentang gerakan berikutnya (ambang sama dengan mode sholat).
static const float BLJ_MOVE_DELTA_B   = 0.10f;   // sensor bawah harus berubah minimal segini (rasio) dari posisi diam
static const float BLJ_MOVE_DELTA_A   = 0.15f;   // sensor atas (hanya dipakai tahiyat awal -> berdiri)
static const float BLJ_STABLE_B       = 0.07f;   // selisih maks-min selama ~1 detik agar dianggap "diam" (bawah)
static const float BLJ_STABLE_A       = 0.12f;   // idem untuk sensor atas
static const float BLJ_DUDUK_MAX      = 1.20f;   // batas atas rasio duduk (membuang data "pantulan hilang" = 400 cm)
static const unsigned long BLJ_PLAT_FORCE_MS = 4000;  // jika belum ada posisi diam stabil setelah ini, pakai rata-rata terakhir
#define BLJ_STABLE_WIN 5                          // jumlah siklus untuk menilai "diam" (5 x 200 ms = 1 detik)

// Status penyaring sensor (0 = sensor atas, 1 = sensor bawah)
static long  bljHist[2][3];
static int   bljHistPos[2];
static float bljOut[2];
static int   bljInvalid[2];

// Status konfirmasi gerakan
static bool          bljReadyInit = false;
static float         bljStandRef = 1.0f;  // tinggi berdiri asli pengguna (rasio sensor atas), diperbarui pelan-pelan saat berdiri tegak
static int           bljCnt = 0;          // berapa siklus beruntun syarat terpenuhi
static unsigned long bljEnterMs = 0;      // kapan masuk ke gerakan sekarang
static unsigned long bljLastDbg = 0;

// Baca 1 sensor: 5 sampel, timeout dibuang, ambil median dari sampel yang valid.
// Mengembalikan -1 jika tidak ada pantulan yang valid.
static long bljBacaJarak(int trigPin, int echoPin) {
  const int N = 5;
  long s[N];
  int n = 0;

  for (int i = 0; i < N; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 25000);
    if (duration > 0) {
      long cm = (duration * 17) / 1000;   // sama dengan duration * 0.034 / 2
      if (cm >= 2 && cm <= 400) s[n++] = cm;
    }
    delay(3);
  }

  if (n < 2) return -1;

  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (s[i] > s[j]) { long t = s[i]; s[i] = s[j]; s[j] = t; }
    }
  }
  return s[n / 2];
}

// Saring hasil baca: median 3 siklus (buang lonjakan) lalu penghalusan ringan.
static float bljSaring(int idx, long raw) {
  if (raw < 0) {
    bljInvalid[idx]++;
    if (bljInvalid[idx] < BLJ_INVALID_MAX) return bljOut[idx];  // tahan nilai terakhir
    raw = 400;                                                   // lama tanpa pantulan = kosong/jauh
  } else {
    bljInvalid[idx] = 0;
  }

  bljHist[idx][bljHistPos[idx]] = raw;
  bljHistPos[idx] = (bljHistPos[idx] + 1) % 3;

  long a = bljHist[idx][0], b = bljHist[idx][1], c = bljHist[idx][2];
  long med;
  if ((a >= b && a <= c) || (a <= b && a >= c)) med = a;
  else if ((b >= a && b <= c) || (b <= a && b >= c)) med = b;
  else med = c;

  bljOut[idx] = (0.5f * med) + (0.5f * bljOut[idx]);
  return bljOut[idx];
}

// ---------- POSISI DIAM (PLATEAU) ----------
// Menyimpan level sensor saat pengguna diam (mis. saat sujud / duduk / tahiyat).
// "Bergerak" = bacaan menyimpang dari level diam itu sebesar delta, beruntun beberapa siklus.
// Selama bergerak, level diam DIKUNCI (tidak ikut berubah), jadi pengguna yang sudah berdiri
// sebelum bacaan selesai tetap terbaca bergerak begitu bacaan selesai.
static float bljWin[2][BLJ_STABLE_WIN];
static int   bljWinPos = 0, bljWinCnt = 0;
static float bljPlat[2] = {1.0f, 1.0f};
static bool  bljPlatReady = false;
static bool  bljMoved = false;      // true = sudah ada perubahan posisi dari posisi diam
static int   bljDevCnt = 0;         // siklus beruntun menyimpang dari posisi diam
static int   bljBackCnt = 0;        // siklus beruntun kembali ke posisi diam

// --- KHUSUS SHOLAT SUBUH (targetRakaat == 2); sholat lain tidak terpengaruh ---
static bool  bljSubuh = false;       // true jika sholat yang dipilih = Subuh
static bool  bljSubuhSujudAkhir = false;  // true saat di sujud terakhir Subuh (menuju tahiyat akhir)

static void bljPlatReset() {
  bljWinPos = 0;
  bljWinCnt = 0;
  bljPlatReady = false;
  bljMoved = false;
  bljDevCnt = 0;
  bljBackCnt = 0;
}

// dirB: +1 = syarat gerak adalah naik (menjauh), -1 = turun (mendekat), 0 = dua arah
// useTop: ikutkan sensor atas dalam penilaian gerak (dipakai tahiyat awal -> berdiri)
static void bljUpdatePlateau(float rA, float rB, int dirB, bool useTop, unsigned long inMs) {
  bljWin[0][bljWinPos] = rA;
  bljWin[1][bljWinPos] = rB;
  bljWinPos = (bljWinPos + 1) % BLJ_STABLE_WIN;
  if (bljWinCnt < BLJ_STABLE_WIN) bljWinCnt++;

  bool windowFull = (bljWinCnt >= BLJ_STABLE_WIN);
  bool stabil = false;
  float mA = rA, mB = rB;
  if (windowFull) {
    float minA = bljWin[0][0], maxA = bljWin[0][0], sumA = 0;
    float minB = bljWin[1][0], maxB = bljWin[1][0], sumB = 0;
    for (int i = 0; i < BLJ_STABLE_WIN; i++) {
      float a = bljWin[0][i], b = bljWin[1][i];
      if (a < minA) minA = a;  if (a > maxA) maxA = a;  sumA += a;
      if (b < minB) minB = b;  if (b > maxB) maxB = b;  sumB += b;
    }
    mA = sumA / BLJ_STABLE_WIN;
    mB = sumB / BLJ_STABLE_WIN;
    stabil = ((maxB - minB) < BLJ_STABLE_B) && (!useTop || ((maxA - minA) < BLJ_STABLE_A));
  }

  // Belum punya posisi diam: tunggu pengguna diam ~1 detik (atau paksa setelah BLJ_PLAT_FORCE_MS)
  if (!bljPlatReady) {
    if (windowFull && (stabil || inMs > BLJ_PLAT_FORCE_MS)) {
      bljPlat[0] = mA;
      bljPlat[1] = mB;
      bljPlatReady = true;
      bljDevCnt = 0;
    }
    return;
  }

  float devA = rA - bljPlat[0];
  float devB = rB - bljPlat[1];
  float dB = (dirB > 0) ? devB : (dirB < 0) ? -devB : fabsf(devB);
  bool menyimpang = (dB >= BLJ_MOVE_DELTA_B) || (useTop && fabsf(devA) >= BLJ_MOVE_DELTA_A);
  bool dekat = (fabsf(devB) < (BLJ_MOVE_DELTA_B * 0.5f)) && (!useTop || fabsf(devA) < (BLJ_MOVE_DELTA_A * 0.5f));

  if (!bljMoved) {
    if (menyimpang) {
      bljDevCnt++;
      if (bljDevCnt >= BLJ_CONFIRM_CYCLES) { bljMoved = true; bljBackCnt = 0; }
    } else {
      bljDevCnt = 0;
      // masih diam: ikuti pergeseran kecil secara perlahan
      bool kecil = (fabsf(devB) < BLJ_MOVE_DELTA_B) && (!useTop || fabsf(devA) < BLJ_MOVE_DELTA_A);
      if (stabil && kecil) {
        bljPlat[0] = (0.7f * bljPlat[0]) + (0.3f * mA);
        bljPlat[1] = (0.7f * bljPlat[1]) + (0.3f * mB);
      } else if (bljSubuhSujudAkhir && stabil && devB < 0.0f) {
        // Subuh, sujud terakhir: badan turun lebih dalam dari posisi diam yang tersimpan
        // -> posisi diam ikut turun, supaya saat bangun perubahan tetap terbaca.
        bljPlat[0] = mA;
        bljPlat[1] = mB;
      }
    }
  } else {
    // sempat menyimpang tapi kembali ke posisi diam -> batalkan (bukan gerakan sungguhan)
    if (dekat) {
      bljBackCnt++;
      if (bljBackCnt >= 5) { bljMoved = false; bljDevCnt = 0; bljBackCnt = 0; }
    } else {
      bljBackCnt = 0;
    }
  }
}

// Reset pembaca gerakan mode belajar (dipanggil setelah kalibrasi)
static void bljReset() {
  for (int i = 0; i < 2; i++) {
    long base = (i == 0) ? baseDistAtas : baseDistBawah;
    for (int k = 0; k < 3; k++) bljHist[i][k] = base;
    bljHistPos[i] = 0;
    bljOut[i] = (float)base;
    bljInvalid[i] = 0;
  }
  bljCnt = 0;
  bljStandRef = 1.0f;
  bljEnterMs = millis();
  bljPlatReset();
  bljReadyInit = true;
}

// KALIBRASI KHUSUS MODE BELAJAR
void runBelajarCalibrationProcess() {
  drawCalibrationScreen("Mohon Diam di Posisi Standby!");
  delay(1500);

  drawCalibrationScreen("Mengukur Baseline...");

  baseDistAtas = getRobustBaseline(TRIG_PIN_1, ECHO_PIN_1);
  baseDistBawah = getRobustBaseline(TRIG_PIN_2, ECHO_PIN_2);

  smoothedAtas = baseDistAtas;
  smoothedBawah = baseDistBawah;

  currentRakaat = 1;
  currentSujud = 0; 
  totalTargetSujud = targetRakaat * 2; 
  currentMotion = MOTION_STAND;

  drawCalibrationScreen("Kalibrasi Selesai!");
  delay(1000);

  standStartTime = millis();   // hitung waktu berdiri dari sini (setelah layar kalibrasi)
  bljReset();
  currentState = STATE_BELAJAR_SIMULASI;
  updateBelajarAudioAndText(); // Putar MP3 & tampilkan bacaan pertama
  drawBelajarSimulationUI();
}

// SIMULASI FSM MODE BELAJAR SINKRON SENSOR + AUDIO BACAAN
void processBelajarSimulationFSM() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < BLJ_CYCLE_MS) return; 
  lastCheck = millis();

  if (!bljReadyInit) bljReset();

  // 1. BACA & SARING SENSOR
  float dAtas  = bljSaring(0, bljBacaJarak(TRIG_PIN_1, ECHO_PIN_1));
  float dBawah = bljSaring(1, bljBacaJarak(TRIG_PIN_2, ECHO_PIN_2));
  smoothedAtas = dAtas;
  smoothedBawah = dBawah;

  float rAtas  = dAtas  / (float)baseDistAtas;    // rasio terhadap baseline (ambang sama dengan mode sholat)
  float rBawah = dBawah / (float)baseDistBawah;

  unsigned long now = millis();
  unsigned long inMs = now - bljEnterMs;          // sudah berapa lama di gerakan ini

  // Bacaan + keterangan gerakan ini sudah selesai diputar? (sensor baru "dipercaya" setelah itu)
  bool audioSelesai = (pendingPos >= pendingLen) && !isAudioPlaying();
  bool audioOk = audioSelesai || (inMs > BLJ_LOCK_MAX_MS);
  // Pantulan bawah hilang lama (nilai 400 buatan)? -> jangan dianggap posisi berdiri/duduk
  bool lostB = (bljInvalid[1] >= BLJ_INVALID_MAX);

  // Posisi diam (plateau) untuk gerakan yang keluarnya dibaca lewat perubahan posisi
  bool tahiyatAwal = (currentRakaat == 2 && targetRakaat > 2);
  bljSubuh = (targetRakaat == 2);
  bljSubuhSujudAkhir = bljSubuh && (currentMotion == MOTION_SUJUD2) && (currentRakaat >= targetRakaat);
  if (currentMotion == MOTION_SUJUD1 || currentMotion == MOTION_SUJUD2) bljUpdatePlateau(rAtas, rBawah, +1, false, inMs);
  else if (currentMotion == MOTION_DUDUK)                               bljUpdatePlateau(rAtas, rBawah, -1, false, inMs);
  else if (currentMotion == MOTION_TAHIYAT && tahiyatAwal)              bljUpdatePlateau(rAtas, rBawah,  0, true,  inMs);

  // Perbarui tinggi berdiri asli pengguna (hanya saat berdiri tegak, bukan saat membungkuk)
  if (currentMotion == MOTION_STAND && rAtas >= 0.85f && rAtas <= 1.30f) {
    bljStandRef = (0.9f * bljStandRef) + (0.1f * rAtas);
  }
  // Posisi rukuk: di bawah 0.80 (seperti semula) ATAU turun cukup jauh dari tinggi berdiri asli pengguna
  bool bowNow = (rAtas > BLJ_RUKUK_MIN_R) && ((rAtas < 0.80f) || (rAtas < bljStandRef - BLJ_RUKUK_DELTA));
  if (bljSubuh && currentMotion == MOTION_STAND) {
    // Subuh: rukuk dibaca lebih toleran
    //  - batas rukuk dilonggarkan (< 0.85, atau turun 0.10 dari tinggi berdiri)
    //  - punggung yang miring bisa memantulkan suara menjauh (sensor atas tidak dapat pantulan)
    //    -> setelah 3 siklus tanpa pantulan saat berdiri, dianggap sedang rukuk
    bool bowLonggar = (rAtas > 0.10f) && ((rAtas < 0.85f) || (rAtas < bljStandRef - 0.10f));
    bool tanpaPantulanAtas = (bljInvalid[0] >= 3);
    bowNow = bowNow || bowLonggar || tanpaPantulanAtas;
  }

  // 2. SYARAT KELUAR DARI GERAKAN SEKARANG (ambang sama dengan mode sholat)
  bool syarat = false;
  bool gerbang = true;     // false = perpindahan ditahan walau syarat terkumpul (kunci berdiri)
  int  perluSiklus = BLJ_CONFIRM_CYCLES;

  switch (currentMotion) {
    case MOTION_STAND: {
#if BLJ_KUNCI_BERDIRI
      // Selama audio takbir/Fatihah/surah/keterangan jalan (kunci aktif) -> pindah ke rukuk ditahan
      bool kunciAktif = isProcessingSequence && (inMs < BLJ_STAND_LOCK_MAX_MS);
#else
      bool kunciAktif = false;
#endif
      perluSiklus = bljSubuh ? 3 : BLJ_RUKUK_CONFIRM;   // Subuh: cukup 3 siklus (~0.6 detik)
      // Posisi rukuk tetap DIHITUNG walau kunci aktif; hanya perpindahannya yang ditahan.
      // Jadi begitu keterangan selesai dan pengguna sedang/masih rukuk, langsung terbaca.
      syarat = (now - standStartTime > STAND_COOLDOWN_MS) && bowNow;
      gerbang = !kunciAktif;
      break;
    }
    case MOTION_RUKUK:
      {
        // Bangun dari rukuk: kembali ke tinggi berdiri (batas 0.85 seperti semula, sedikit lebih rendah
        // jika pengguna memang berdiri lebih dekat ke sensor)
        float batasBangun = bljStandRef - 0.08f;
        if (batasBangun > 0.85f) batasBangun = 0.85f;
        syarat = audioOk && (now - rukukStartTime > RUKUK_MIN_MS) && (rAtas >= batasBangun);
      }
      break;

    case MOTION_ITIDAL:
      syarat = audioOk && (inMs > BLJ_ITIDAL_MIN_MS) && (rBawah < 0.40f);
      break;

    case MOTION_SUJUD1:
      // Sujud -> duduk: bacaan sujud + keterangan duduk SELESAI dulu, posisi benar-benar naik dari
      // posisi sujud, dan nilainya di rentang duduk (>= 0.40, tanpa data "pantulan hilang").
      syarat = audioOk && bljMoved && (inMs > BLJ_SUJUD1_MIN_MS)
               && (rBawah >= 0.40f) && (rBawah <= BLJ_DUDUK_MAX) && !lostB;
      break;

    case MOTION_DUDUK:
      // Duduk -> sujud ke-2: bacaan duduk + keterangan sujud ke-2 selesai, posisi turun dari posisi duduk
      syarat = audioOk && bljMoved && (inMs > BLJ_DUDUK_MIN_MS) && (rBawah < 0.40f);
      break;

    case MOTION_SUJUD2:
      // Subuh (sujud terakhir): perubahan posisi dianggap cukup jika sudah naik >= 0.05 dari posisi diam sujud
      if (audioOk && (bljMoved || (bljSubuhSujudAkhir && bljPlatReady && (rBawah - bljPlat[1]) >= 0.05f))
          && (now - sujud2StartTime > SUJUD_MIN_MS)) {
        if (currentRakaat == 2 || currentRakaat >= targetRakaat) syarat = (rBawah >= 0.40f);
        else syarat = (rBawah >= 0.80f) && !lostB;
      }
      break;

    case MOTION_TAHIYAT:
      if (tahiyatAwal) {
        // Tahiyat awal -> berdiri: doa selesai, lalu pengguna benar-benar berubah posisi dari posisi duduk
        syarat = audioOk && bljMoved && (now - tahiyatStartTime > TAHIYAT_MIN_MS)
                 && (rBawah >= 0.85f) && !lostB;
      } else {
        // Tahiyat akhir: tunggu doa + keterangan salam + salam + penutup selesai dulu
        syarat = audioSelesai || (inMs > BLJ_AUDIO_END_MAX);
        perluSiklus = 1;
      }
      break;
  }

  // 3. KONFIRMASI BERUNTUN: syarat harus benar terus selama beberapa siklus
  if (syarat) {
    if (bljCnt < 100) bljCnt++;
  } else if (currentMotion == MOTION_STAND) {
    if (bljCnt > 0) bljCnt--;     // berdiri->rukuk: satu bacaan goyang tidak menghapus semua hitungan
  } else {
    bljCnt = 0;
  }

#if BELAJAR_DEBUG
  if (now - bljLastDbg >= 500) {
    bljLastDbg = now;
    Serial.printf("[BELAJAR] gerak=%d rkt=%d atas=%.0f(%.2f) bawah=%.0f(%.2f) diam=%.2f/%.2f siap=%d bergerak=%d audioSelesai=%d syarat=%d cnt=%d/%d kunci=%d bow=%d ref=%.2f\n",
                  (int)currentMotion, currentRakaat, dAtas, rAtas, dBawah, rBawah,
                  bljPlat[0], bljPlat[1], bljPlatReady, bljMoved, audioSelesai,
                  syarat, bljCnt, perluSiklus, isProcessingSequence, bowNow, bljStandRef);
  }
#endif

  if (bljCnt < perluSiklus || !gerbang) return;

  // 4. PINDAH GERAKAN (selalu berurutan)
  PrayerMotion prevMotion = currentMotion;

  switch (currentMotion) {
    case MOTION_STAND:
      currentMotion = MOTION_RUKUK;
      rukukStartTime = now;
      break;

    case MOTION_RUKUK:
      currentMotion = MOTION_ITIDAL;
      break;

    case MOTION_ITIDAL:
      currentMotion = MOTION_SUJUD1;
      currentSujud++;
      break;

    case MOTION_SUJUD1:
      currentMotion = MOTION_DUDUK;
      break;

    case MOTION_DUDUK:
      currentMotion = MOTION_SUJUD2;
      currentSujud++;
      sujud2StartTime = now;
      break;

    case MOTION_SUJUD2:
      if (currentRakaat == 2 || currentRakaat >= targetRakaat) {
        currentMotion = MOTION_TAHIYAT;
        tahiyatStartTime = now;
      } else {
        currentRakaat++;
        currentMotion = MOTION_STAND;
        standStartTime = now;
      }
      break;

    case MOTION_TAHIYAT:
      if (currentRakaat == 2 && targetRakaat > 2) {
        currentRakaat++;
        currentMotion = MOTION_STAND;
        standStartTime = now;
      } else {
        stopAudioGuide();
        currentState = STATE_BELAJAR_SELESAI;
        drawBelajarSelesaiUI();
        return;
      }
      break;
  }

  // Siapkan pembacaan untuk gerakan baru
  bljCnt = 0;
  bljEnterMs = now;
  bljPlatReset();   // posisi diam gerakan baru dicari ulang

#if BELAJAR_DEBUG
  Serial.printf("[BELAJAR] >>> PINDAH gerakan %d -> %d (rakaat %d, sujud %d)\n",
                (int)prevMotion, (int)currentMotion, currentRakaat, currentSujud);
#endif

  // Jika posisi gerakan berubah, perbarui audio MP3 dan tampilan teks bacaan di layar
  updateBelajarAudioAndText();
  drawBelajarSimulationUI();
}
