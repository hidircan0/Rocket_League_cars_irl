#include <PS4Controller.h>
#include <nvs_flash.h>

// --- PİN TANIMLAMALARI (L298N) ---
const int ENA = 13; // Sol Motor Hız (PWM)
const int IN1 = 12; // Sol Motor İleri
const int IN2 = 14; // Sol Motor Geri

const int IN3 = 27; // Sağ Motor İleri
const int IN4 = 26; // Sağ Motor Geri
const int ENB = 25; // Sağ Motor Hız (PWM)

// --- PWM AYARLARI ---
const int freq = 5000;
const int pwmChannelLeft = 0;
const int pwmChannelRight = 1;
const int resolution = 8;

// --- MOTOR KALİBRASYONU ---
bool invertLeft = false;  
bool invertRight = false; 

// --- SÜRÜŞ DEĞİŞKENLERİ ---
bool cruiseControlEnabled = false;
const int cruiseSpeed = 180; // Sabit hız gücü (0-255 arası)

// --- RENK KİLİDİ ---
bool colorSet = false;

void setup() {
  Serial.begin(115200);

  // Motor Pinleri Çıkış
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
  // PWM Kurulumu
  ledcSetup(pwmChannelLeft, freq, resolution);
  ledcSetup(pwmChannelRight, freq, resolution);
  ledcAttachPin(ENA, pwmChannelLeft);
  ledcAttachPin(ENB, pwmChannelRight);

  nvs_flash_erase();
  nvs_flash_init();
  // PS4 Başlat (Buraya kola yazdığın MAC adresini gir)
// PS4 Başlat (Buraya laptobun MAC adresini küçük harflerle gir)
  PS4.begin("02:02:02:02:02:02"); 
 Serial.println("PS4 Baglantisi Bekleniyor...");
}

void moveMotors(int x, int y, int boost) {
  // Tank sürüş algoritması (Differential Drive)
  int leftSpeed = y + x;
  int rightSpeed = y - x;

  // Nitro (Boost) hesaplaması: Yuvarlak tuşuna basıldığında hızı ikiye katla
  float boostMultiplier = 1.0 + (boost / 255.0); 
  leftSpeed = leftSpeed * boostMultiplier;
  rightSpeed = rightSpeed * boostMultiplier;

  // Hız sınırlarını -255 ile 255 arasına sıkıştır
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Sol Motor Yön
  bool leftForward = leftSpeed >= 0;
  if(invertLeft) leftForward = !leftForward;
  
  if (leftSpeed == 0) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  } else if (leftForward) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  }

  // Sağ Motor Yön68:54:5d4a:40:80:
  bool rightForward = rightSpeed >= 0;
  if(invertRight) rightForward = !rightForward;

  if (rightSpeed == 0) {
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  } else if (rightForward) {
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  }

  // Gücü PWM pinlerine yaz
  ledcWrite(pwmChannelLeft, abs(leftSpeed));
  ledcWrite(pwmChannelRight, abs(rightSpeed));
}

void loop() {
  if (PS4.isConnected()) {
    
    // --- RENK AYARI ---
    if (!colorSet) {
      PS4.setLed(255, 0, 0);   // Mavi Takım (Kırmızı için: 255, 0, 0)
      PS4.sendToController();  // Rengi kola yolla
      colorSet = true;         // Kilidi kapat
    }
    
    // 1. DİREKSİYON: Sadece X eksenini (sağ/sol) sol analogdan alıyoruz
    int stickX = PS4.LStickX();
    if (abs(stickX) < 15) stickX = 0; // Deadzone filtresi
    int mappedX = map(stickX, -128, 127, -255, 255); 

    // 2. GAZ/FREN: R2 (İleri) ve L2 (Geri) tetiklerinin değerlerini alıyoruz (0 - 255 arası gelir)
    int r2Gas = PS4.R2Value(); 
    int l2Brake = PS4.L2Value(); 
    
    // Y eksenini tetiklerin farkı olarak hesapla (R2 basılıysa pozitif, L2 basılıysa negatif olur)
    int mappedY = r2Gas - l2Brake; 

    // 3. BOOST (NİTRO): Yuvarlak (Circle) tuşuna basılıysa tam güç (255) yolla, değilse 0
    int boostValue = PS4.Circle() ? 255 : 0;

    // Hız Sabitleyici (Cruise Control) Mantığı
    if (PS4.event.button_down.triangle) {
      cruiseControlEnabled = !cruiseControlEnabled; // Üçgene basınca aç/kapat
    }

    // Eğer Cruise açıksa ve oyuncu L2'ye (Fren) basarsa Cruise'u iptal et
    if (cruiseControlEnabled) {
      if (l2Brake > 30) { 
        cruiseControlEnabled = false;
      } else {
        mappedY = cruiseSpeed; // İleri hızı sabitle
      }
    }

    // Motorlara komut gönder
    moveMotors(mappedX, mappedY, boostValue);

  } else {
    // Bağlantı koparsa veya kol kapanırsa arabayı anında durdur (Safe-Stop)
    moveMotors(0, 0, 0);
    colorSet = false; // Yeniden bağlandığında rengi tekrar yollayabilmek için kilidi aç
  }
  
  delay(20); 
}