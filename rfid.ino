#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

const char* ssid = "Kepler";
const char* password = "3141527182";

String serverUrl = "http://192.168.1.121:8000/rfid";

// Queue để gửi UID giữa các task
QueueHandle_t uidQueue;

// ======================= TASK RFID =======================
void TaskRFID(void *pvParameters) {
  while (true) {
    if (!rfid.PICC_IsNewCardPresent()) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    if (!rfid.PICC_ReadCardSerial()) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    String uid = "";

    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    uid.toUpperCase();

    Serial.println("UID: " + uid);

    // Gửi UID vào queue
    xQueueSend(uidQueue, &uid, portMAX_DELAY);

    // Dừng thẻ
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}

// ======================= TASK HTTP =======================
void TaskHTTP(void *pvParameters) {
  String uid;

  while (true) {
    // Chờ nhận UID từ queue
    if (xQueueReceive(uidQueue, &uid, portMAX_DELAY) == pdTRUE) {

      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String data = "uid=" + uid;
        int code = http.POST(data);

        Serial.println("HTTP Code: " + String(code));
        http.end();
      } else {
        Serial.println("WiFi disconnected");
      }
    }
  }
}

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  // Tạo queue (10 phần tử, mỗi phần tử là String)
  uidQueue = xQueueCreate(10, sizeof(String));

  // Tạo task
  xTaskCreatePinnedToCore(TaskRFID, "RFID Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskHTTP, "HTTP Task", 8192, NULL, 1, NULL, 0);
}

// ======================= LOOP =======================
void loop() {
  // Không cần dùng loop nữa
}