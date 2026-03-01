#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <map>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// LCD Setup
LiquidCrystal_PCF8574 lcd(0x27);

// Button and Buzzer Pin Definitions
#define REMOVE_BUTTON 13
#define SCROLL_BUTTON 12
#define BUZZER_PIN 14

// RFID Setup
#define SS_PIN 5
#define RST_PIN 4
MFRC522 mfrc522(SS_PIN, RST_PIN);

// WiFi Credentials
const char* ssid = "";
const char* password = "";

// WebSocket Server
WebSocketsServer webSocket(81);

// Local Product Database
struct Product {
    String name;
    int price;
};

std::map<String, Product> barcodeDatabase = {
    {"8901030987328", {"3 Roses", 10}},
    {"4987176074195", {"Head & Shoulders", 82}},
    {"8901764012709", {"CocaCola", 40}},
    {"8901023028779", {"CINTHOL ORIGINAL", 10}}
};

// Cart Database
std::map<String, int> cart;
int totalPrice = 0;
bool removeMode = false;
int scrollIndex = 0;

// Payment RFID Cards
std::map<String, float> paymentCards = {
    {"91E4D500", 400.0},
    {"5415E000", 100.0}
};

// Beep Function
void beep(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
    noTone(BUZZER_PIN);
}

// Fetch Product from API

    int httpResponseCode = http.GET();
    if (h
        if (!doc["product"].isNull() && !doc["product"]["product_name"].isNull()) {
            productName = doc["product"]["product_name"].as<String>();
            return true;
        }
    }

    http.end();
    return false;
}

// Fetch Product from Local Database
bool fetchP
        return true;
    }
    return false;
}

// Add Product to Cart
bool addToCart(const String &barcode) {
    St;
            lcd.print("Product Not Found!");
            return false;
        }
    }

    cart[productName]++;
    totalPrice += price;
    beep(1000, 200);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(productName);
    lcd.setCursor(0, 1);
    lcd.print("Total: ₹");
    lcd.print(totalPrice);

    // Send updated cart to WebSocket
    String cartData = "{\"product\":\"" + productName + "\", \"price\":" + String(price) + ", \"total\":" + String(totalPrice) + "}";
    webSocket.broadcastTXT(cartData);

    return true;
}

// Remove Product from Cart
bool removeFromCart(const String &barcode) {
    String productName;
    int price = 10;

    if (!fetchProductFromAPI(barcode, productName)) {
        if (!fetchProductFromLocal(barcode, productName, price)) return false;
    }

    if (cart.find(productName) != cart.end()) {
        if (cart[productName] > 1) cart[productName]--;
        else cart.erase(productName);

        totalPrice -= price;
        beep(800, 200);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Removed: ");
        lcd.print(productName);
        lcd.setCursor(0, 1);
        lcd.print("Total: ₹");
        lcd.print(totalPrice);

        // Send update to WebSocket
        String cartData = "{\"remove\":\"" + productName + "\", \"total\":" + String(totalPrice) + "}";
        webSocket.broadcastTXT(cartData);

        return true;
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Not in Cart!");
        beep(500, 300);
        return false;
    }
}

// Scroll Through Cart
void scrollCart() {
    if (cart.empty()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cart is Empty!");
        return;
    }

    beep(500, 100);
    auto it = cart.begin();
    std::advance(it, scrollIndex);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(it->first);
    lcd.setCursor(0, 1);
    lcd.print("Qty: ");
    lcd.print(it->second);

    scrollIndex = (scrollIndex + 1) % cart.size();
}

// WebSocket Event Handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    String barcode = "";

    if (type == WStype_TEXT) {
        payload[length] = '\0';
        barcode = String((char*)payload);
        barcode.trim();
    } else if (type == WStype_BIN) {
        for (size_t i = 0; i < length; i++) {
            barcode += (char)payload[i];
        }
        barcode.trim();
    } else return;

    if (removeMode) {
        removeFromCart(barcode);
        removeMode = false;
    } else {
        addToCart(barcode);
    }
}

// Process Payment
void processPayment() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    if (paymentCards.find(uid) != paymentCards.end()) {
        float &balance = paymentCards[uid];

        if (balance >= totalPrice) {
            balance -= totalPrice;
            

            cart.clear();
            totalPrice = 0;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Scan to Add!");
        }
    }
}

void setup() {
    Serial.begin(115200);




void loop() {
    webSocket.loop();
    processPayment();

    if
    }
}
