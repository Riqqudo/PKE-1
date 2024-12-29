#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// Pin untuk motor kanan (A)
const int ENA = 23;
const int IN1 = 22;
const int IN2 = 21;

// Pin untuk motor kiri (B)
const int ENB = 4;
const int IN3 = 19;
const int IN4 = 18;

// sensor HC-SR04
const int trigPin = 27;
const int echoPin = 14;

// Define IR sensor pins 
#define IR_LEFT 34 
#define IR_RIGHT 35

#define Kec_Motor 255
#define Kec_Suara 0.034 // kecepatan suara dalam cm/us

long duration;
float distance;
const float jarakBerhenti = 15.0;
const float jarakKecTurun = 30.0;
const float jarakMaksimum = 200.0;

const int freqPWM = 1000;   // 1 kHz
const int PWMResolution = 8; // 8 bit
const int PWM_AChannel = 4; // Saluran PWM untuk motor kanan
const int PWM_BChannel = 5;  // Saluran PWM untuk motor kiri

int mode = 0;  // 0 untuk setting awal. Note: 1 untuk RC dan 2 untuk LineFollower

void setup() {
  Serial.begin(9600); // Inisialisasi Serial untuk debugging
  Serial2.begin(9600, SERIAL_8N1, 17, 16); // TX: GPIO 16, RX: GPIO 17 
  Dabble.begin("PKE-9"); 

  // Pin setup untuk motor dan sensor
  // IR sensor
  pinMode(IR_LEFT, INPUT); 
  pinMode(IR_RIGHT, INPUT);
  // HCSR sensor
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT);
  // Driver L298n
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Setup PWM menggunakan API lama (Board ESP Versi 2.17)
  ledcSetup(PWM_AChannel, freqPWM, PWMResolution);
  ledcSetup(PWM_BChannel, freqPWM, PWMResolution);
  ledcAttachPin(ENA, PWM_AChannel);       // secara implisit set ENA High
  ledcAttachPin(ENB, PWM_BChannel);       // secara implisit set ENB High

  rotateMotor(0, 0); // Berhenti
}

void rotateMotor(int A, int B) {
  // Kontrol motor kanan (A)
  if (A > 0) {                          // Maju
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else if (A < 0) {                   // Mundur
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {                              // OFF
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // Kontrol motor kiri (B)
  if (B > 0) {                          // Maju
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else if (B < 0) {                   // Mundur
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {                              // Off
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  // Set kecepatan PWM
  ledcWrite(PWM_AChannel, abs(B));
  ledcWrite(PWM_BChannel, abs(A));
}

void loop() {
  Dabble.processInput();
  checkDistance();

  // Cek mode yang diterima dari RX
  if (Serial.available()) {
    String input = Serial2.readString();
    input.trim();  // menghilangkan karakter lain
    Serial.println(input);  // Print the received string for debugging
    if (input == "1") {
      mode = 1;  // RC mode
    } else if (input == "2") {
      mode = 2;  // Line follower mode
    }
  }

  // Mode RC
  if (mode == 1) {
    int A = 0;
    int B = 0;

    if (distance <= jarakBerhenti) {
      rotateMotor(0, 0);
      delay(10);
      if (GamePad.isUpPressed()) {
        rotateMotor(0, 0);
        delay(10);
      } else if (GamePad.isRightPressed()) {
        A = -Kec_Motor;
        B =  Kec_Motor;
      } else if (GamePad.isLeftPressed()) {
        A =  Kec_Motor;
        B = -Kec_Motor;
      } else if (GamePad.isDownPressed()) {
        A = -Kec_Motor;
        B = -Kec_Motor;
      }
    } else if (distance <= jarakKecTurun) {
      A = Kec_Motor / 2;
      B = Kec_Motor / 2;
        if (GamePad.isUpPressed()) {
        A = Kec_Motor;
        B = Kec_Motor;
      } else if (GamePad.isRightPressed()) {
        A = -Kec_Motor;
        B =  Kec_Motor;
      } else if (GamePad.isLeftPressed()) {
        A =  Kec_Motor;
        B = -Kec_Motor;
      } else if (GamePad.isDownPressed()) {
        A = -Kec_Motor;
        B = -Kec_Motor;
      }
    } else {
      if (GamePad.isUpPressed()) {
        A = Kec_Motor;
        B = Kec_Motor;
      } else if (GamePad.isRightPressed()) {
        A = -Kec_Motor;
        B =  Kec_Motor;
      } else if (GamePad.isLeftPressed()) {
        A =  Kec_Motor;
        B = -Kec_Motor;
      } else if (GamePad.isDownPressed()) {
        A = -Kec_Motor;
        B = -Kec_Motor;
      } 
    }

    // Jika tidak ada tombol yang ditekan, berhentikan motor
    if (!GamePad.isUpPressed() && !GamePad.isDownPressed() && 
      !GamePad.isRightPressed() && !GamePad.isLeftPressed()) {
      A = 0;
      B = 0; 
      rotateMotor(0,0);   // penulisan lain:)
    }

    rotateMotor(A, B);
  }
  // Mode Line Follower
  else if (mode == 2) {
    int A = 0;
    int B = 0;

    if(distance >= jarakBerhenti){
      int leftSensor = digitalRead(IR_LEFT);
      int rightSensor = digitalRead(IR_RIGHT);

      if (leftSensor == HIGH && rightSensor == LOW) {
        // Belok kanan
        A = -Kec_Motor;
        B =  Kec_Motor;
      } else if (leftSensor == LOW && rightSensor == HIGH) {
        // Belok kiri
        A =  Kec_Motor;
        B = -Kec_Motor;
      } else if (leftSensor == HIGH && rightSensor == HIGH) {
        // Terus jalan
        A = Kec_Motor;
        B = Kec_Motor;
      } else if (leftSensor == LOW && rightSensor == LOW) {
        // Berhenti
        A = 0;
        B = 0;
        rotateMotor(0, 0);    // penulisan lain:)
      }
    } 

  rotateMotor(A, B);
  delay(20);
  }
};

// HCSR membaca jarak objek 
void checkDistance() {
  // Clear trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  // trigPin HIGH selama 10us
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Baca durasi sinyal echo
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * Kec_Suara) / 2;

  // Handle error durasi
  if (distance >= jarakMaksimum) {
    distance = jarakMaksimum;
  }
};
