#include "Config.h"

void initDFPlayer() {
  // Inisialisasi Serial2 (RX=16, TX=17)
  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  delay(1000); // Beri waktu chip DFPlayer booting

  if (!myDFPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer Error!");
  } else {
    Serial.println("DFPlayer Ready!");
    delay(200);
    myDFPlayer.volume(28); // Setel volume (0 - 30)
    delay(200);
  }
}

void playAudioGuide(int folderNo, int trackNo) {
  Serial.printf("Memutar -> Folder: %d, Track: %d\n", folderNo, trackNo);

  // Jeda singkat sebelum dan sesudah kirim command agar komunikasi serial stabil
  delay(50);
  myDFPlayer.playLargeFolder(folderNo, trackNo);
  delay(50);
}

void stopAudioGuide() {
  myDFPlayer.stop();
}

// ===================== FUNGSI YANG SEBELUMNYA HILANG =====================

// Kosongkan antrean audio lanjutan (mode belajar)
void clearPendingAudio() {
  for (int i = 0; i < 4; i++) pendingTracks[i] = 0;
  pendingLen = 0;
  pendingPos = 0;
}

// Isi antrean audio lanjutan: diputar berurutan setelah audio gerakan selesai.
// Track bernilai 0 dianggap kosong (tidak dimasukkan ke antrean).
void setPendingAudio(int t1, int t2, int t3) {
  clearPendingAudio();
  int list[3] = { t1, t2, t3 };
  for (int i = 0; i < 3; i++) {
    if (list[i] > 0) pendingTracks[pendingLen++] = list[i];
  }
}

// Reset semua status audio (dipanggil saat kembali ke menu / mulai ulang)
void resetAudioState() {
  stopAudioGuide();
  clearPendingAudio();
  lastPlayedFolder = -1;
  lastPlayedTrack = -1;
}