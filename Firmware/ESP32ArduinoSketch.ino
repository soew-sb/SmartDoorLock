#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Pin Definitions
#define RELAY_PIN 23       // Relay control pin connected to GPIO 23 on ESP32
#define RED_LED_PIN 18     // Red LED pin for Access Denied
#define GREEN_LED_PIN 19   // Green LED pin for Access Granted
#define PASSWORD "2024"    // Define your password here
#define TIMEOUT_SECONDS 10 // Timeout in seconds for PIN entry

// Supabase Configuration
#define SUPABASE_URL "https://doxwvmmbpymxjjnyyyhk.supabase.co"
#define SUPABASE_API_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImRveHd2bW1icHlteGpqbnl5eWhrIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTcyOTQzNTUzNywiZXhwIjoyMDQ1MDExNTM3fQ.-l7r2WthliKW2BPAjrVwv1Ua-qCT-a5skhHvkN8AIXc"

// WiFi Credentials
const char *ssid_1 = "SOEW";
const char *wifi_password_1 = "yaliwe15";

const char *ssid_2 = "Holiday";
const char *wifi_password_2 = "K0b14[53";

// LCD Setup (Adjust the I2C address if necessary, e.g., 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address, columns, rows

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {13, 12, 14, 27}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {26, 25, 33};     // Connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables for PIN Entry and Timing
String inputCode = "";              // To store the entered code
unsigned long lastKeypressTime = 0; // To track the last keypress time
int attempts = 0;                   // To track the number of failed attempts
int maxAttempts = 3;                // Maximum number of attempts before OTP

// Enum for Door Lock State
enum DoorLockState
{
    STATE_WELCOME,
    STATE_PIN_ENTRY,
    STATE_UNLOCKED,
    STATE_DENIED,
    STATE_TIMEOUT,
    STATE_OTP_ENTRY
};

// State Variables
bool welcomeScreen = true;       // Flag to show welcome screen
bool showBlink = false;          // Flag to indicate blinking "Press 0!"
bool blinkState = false;         // Current state of the blink (on/off)
unsigned long lastBlinkTime = 0; // To track the last blink time
int previousInputLength = 0;     // To track the previous input length

DoorLockState currentState = STATE_WELCOME;
bool stateChanged = true;
unsigned long stateEnteredTime = 0;

// Structure for access records
struct AccessRecord
{
    const char *action;
    bool success;
    const char *pin;
};

String currentOtp = "";

// Global HTTP client variables
WiFiClientSecure *globalClient = nullptr;
HTTPClient globalHttps;
bool isHttpInitialized = false;

void setup()
{
    Serial.begin(115200);
    Wire.begin(21, 22); // Initialize I2C with SDA on GPIO 21 and SCL on GPIO 22

    lcd.init();      // Initialize the LCD
    delay(100);      // Brief delay for LCD to initialize
    lcd.backlight(); // Turn on the backlight

    connectToWiFi();

    // Initialize random number generator
    randomSeed(analogRead(0));

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    // Test LEDs and Relay
    testLEDs();
    testRelay();

    // Test Supabase connection
    testSupabaseConnection();
}

void loop()
{
    char key = keypad.getKey();

    switch (currentState)
    {
    case STATE_WELCOME:
        handleWelcomeState(key);
        break;

    case STATE_PIN_ENTRY:
        handlePinEntryState(key);
        break;

    case STATE_UNLOCKED:
        handleUnlockedState();
        break;

    case STATE_DENIED:
        handleDeniedState();
        break;

    case STATE_TIMEOUT:
        handleTimeoutState();
        break;

    case STATE_OTP_ENTRY:
        handleOtpEntryState(key);
        break;
    }
}

// ------------------ Connectivity Functions ------------------
void connectToWiFi()
{
    const char *ssids[] = {ssid_1, ssid_2};
    const char *passwords[] = {wifi_password_1, wifi_password_2};
    const int maxAttempts = 3;

    for (int i = 0; i < 2; i++)
    {
        int attempt = 0;
        while (attempt < maxAttempts)
        {
            Serial.print("Attempting to connect to SSID: ");
            Serial.println(ssids[i]);
            WiFi.begin(ssids[i], passwords[i]);

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Connecting to");
            lcd.setCursor(0, 1);
            lcd.print(ssids[i]);

            int waitTime = 0;
            while (WiFi.status() != WL_CONNECTED && waitTime < 10)
            {
                Serial.print(".");
                delay(1000);
                waitTime++;
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.print("Connected to ");
                Serial.println(ssids[i]);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Connected to");
                lcd.setCursor(0, 1);
                lcd.print(ssids[i]);
                delay(2000);
                return;
            }
            else
            {
                Serial.println("Connection failed!");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Failed to connect");
                lcd.setCursor(0, 1);
                lcd.print(ssids[i]);
                delay(2000);
            }
            attempt++;
        }
    }

    // If both networks failed, restart the process
    Serial.println("Failed to connect to both networks. Restarting...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Restarting WiFi");
    lcd.setCursor(0, 1);
    lcd.print("connection...");
    delay(2000);
    connectToWiFi();
}

bool initializeHttpClient()
{
    if (isHttpInitialized)
    {
        Serial.println("HTTP client already initialized");
        return true;
    }

    if (!globalClient)
    {
        Serial.println("Creating new HTTP client");

        globalClient = new WiFiClientSecure();

        if (!globalClient)
        {
            Serial.println("Failed to create HTTP client");
            return false;
        }

        globalClient->setInsecure();
    }

    isHttpInitialized = true;

    return true;
}

void cleanupHttpClient()
{
    Serial.println("Cleaning up HTTP client");
    if (globalClient)
    {
        globalHttps.end();
        delete globalClient;
        globalClient = nullptr;
    }
    isHttpInitialized = false;
}

bool beginHttpRequest(String endpoint)
{
    Serial.println("Attempting to send request to: (" + endpoint + ")");

    if (!initializeHttpClient())
        return false;

    if (!globalHttps.begin(*globalClient, String(SUPABASE_URL) + endpoint))
        return false;

    // Set timeout
    globalHttps.setTimeout(5000);

    // Set common headers
    globalHttps.addHeader("apikey", SUPABASE_API_KEY);
    globalHttps.addHeader("Authorization", "Bearer " + String(SUPABASE_API_KEY));
    globalHttps.addHeader("Content-Type", "application/json");
    globalHttps.addHeader("Prefer", "return=minimal");

    return true;
}

// ------------------ State Handler Functions ------------------
void handleWelcomeState(char key)
{
    static bool showBlink = false;
    static bool blinkState = false;
    static unsigned long lastBlinkTime = 0;

    // State Entry Actions
    if (stateChanged)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Welcome Home.");
        lcd.setCursor(0, 1);
        lcd.print("Press 0 to unlock");
        showBlink = false;
        stateChanged = false;
        stateEnteredTime = millis();
    }

    // State Processing
    if (key == '0')
    {
        currentState = STATE_PIN_ENTRY;
        stateChanged = true;
        return;
    }

    if (key && key != '0')
    {
        showBlink = true;
        lastBlinkTime = millis();
    }

    // Blink Handler
    if (showBlink && (millis() - lastBlinkTime > 500))
    {
        lastBlinkTime = millis();
        blinkState = !blinkState;
        lcd.setCursor(0, 1);
        lcd.print(blinkState ? "Press 0!         " : "                ");
    }
}

void handlePinEntryState(char key)
{
    // State Entry Actions
    if (stateChanged)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter PIN:");
        inputCode = "";
        lastKeypressTime = millis();
        stateChanged = false;
        stateEnteredTime = millis();
    }

    // Timeout Check
    if (millis() - lastKeypressTime > TIMEOUT_SECONDS * 1000)
    {
        currentState = STATE_TIMEOUT;
        stateChanged = true;
        return;
    }

    // Key Processing
    if (key)
    {
        lastKeypressTime = millis();

        if (key == '#')
        {
            bool success = (inputCode == PASSWORD);

            if (success)
            {
                attempts = 0;
                currentState = STATE_UNLOCKED;
            }
            else
            {
                attempts++;

                if (attempts >= maxAttempts)
                {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Too many fails!");
                    lcd.setCursor(0, 1);
                    lcd.print("Use OTP instead");
                    delay(2000);
                    bool otpSendStatus = sendOtpRecord();

                    if (otpSendStatus)
                    {
                        currentState = STATE_OTP_ENTRY;
                    }
                    else
                    {
                        logAccessRecord("PIN_ENTRY", success, inputCode);
                        currentState = STATE_DENIED;
                    }

                    stateChanged = true;
                    attempts = 0;
                }
                else
                {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Wrong PIN!");
                    lcd.setCursor(0, 1);
                    lcd.print(String(maxAttempts - attempts) + " tries left");
                    delay(2000);
                    lcd.clear();
                }
            }

            stateChanged = true;
            return;
        }

        if (key == '*')
        {
            inputCode = "";
            lcd.setCursor(0, 1);
            lcd.print("                ");
            return;
        }

        if (inputCode.length() < 4)
        {
            inputCode += key;
            lcd.setCursor(0, 1);
            lcd.print("                ");
            lcd.setCursor(0, 1);
            for (int i = 0; i < inputCode.length(); i++)
            {
                lcd.print("*");
            }
        }
    }
}

void handleUnlockedState()
{
    // State Entry Actions
    if (stateChanged)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");
        digitalWrite(GREEN_LED_PIN, HIGH);
        digitalWrite(RELAY_PIN, HIGH);
        stateChanged = false;
        stateEnteredTime = millis();
        return;
    }

    // State Exit After 10 Seconds
    if (millis() - stateEnteredTime > 10000)
    {
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(GREEN_LED_PIN, LOW);
        currentState = STATE_WELCOME;
        stateChanged = true;
        return;
    }
}

void handleDeniedState()
{
    // State Entry Actions
    if (stateChanged)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Denied");
        digitalWrite(RED_LED_PIN, HIGH);
        stateChanged = false;
        stateEnteredTime = millis();
        return;
    }

    // State Exit After 2 Seconds
    if (millis() - stateEnteredTime > 2000)
    {
        digitalWrite(RED_LED_PIN, LOW);
        currentState = STATE_WELCOME;
        stateChanged = true;
        return;
    }
}

void handleTimeoutState()
{
    // State Entry Actions
    if (stateChanged)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timeout!");
        digitalWrite(RED_LED_PIN, HIGH);
        logAccessRecord("TIMEOUT", false, inputCode);

        stateChanged = false;
        stateEnteredTime = millis();
        return;
    }

    // State Exit After 2 Seconds
    if (millis() - stateEnteredTime > 2000)
    {
        digitalWrite(RED_LED_PIN, LOW);
        currentState = STATE_WELCOME;
        stateChanged = true;
        return;
    }
}

void handleOtpEntryState(char key)
{
    static String inputOtp = "";
    static bool showingError = false;
    static unsigned long errorStartTime = 0;
    const unsigned long ERROR_DISPLAY_TIME = 2000;

    // State Entry Actions
    if (stateChanged)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter OTP:");
        inputOtp = "";
        lastKeypressTime = millis();
        stateChanged = false;
        stateEnteredTime = millis();
        showingError = false;
    }

    // Handle error display timeout
    if (showingError && (millis() - errorStartTime >= ERROR_DISPLAY_TIME))
    {
        showingError = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter OTP:");
        if (inputOtp.length() > 0)
        {
            lcd.setCursor(0, 1);
            for (unsigned int i = 0; i < inputOtp.length(); i++)
            {
                lcd.print("*");
            }
        }
    }

    // Key Processing
    if (!showingError && key)
    {
        lastKeypressTime = millis();

        if (key == '#')
        {
            if (inputOtp.length() > 0)
            {
                verifyOtp(inputOtp);
            }
            return;
        }

        if (key == '*')
        {
            inputOtp = "";
            lcd.setCursor(0, 1);
            lcd.print("                ");
            return;
        }

        if (inputOtp.length() < 5 && isDigit(key))
        {
            inputOtp += key;
            lcd.setCursor(0, 1);
            lcd.print("                ");
            lcd.setCursor(0, 1);
            for (unsigned int i = 0; i < inputOtp.length(); i++)
            {
                lcd.print("*");
            }
        }
    }
}

// ------------------ Hardware Helper Functions ------------------
void displayMessage(const char *line1, const char *line2 = "", int delayMs = 0)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    if (strlen(line2) > 0)
    {
        lcd.setCursor(0, 1);
        lcd.print(line2);
    }
    if (delayMs > 0)
    {
        delay(delayMs);
    }
}

void testLEDs()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Testing LEDs");
    digitalWrite(RED_LED_PIN, HIGH);
    delay(1000);

    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(1000);

    digitalWrite(GREEN_LED_PIN, LOW);
}

void testRelay()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Testing Relay");
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    digitalWrite(RELAY_PIN, LOW);
}

// ------------------ Supabase Helper Functions ------------------
void testSupabaseConnection()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Checking Cloud");

    initializeHttpClient();
    beginHttpRequest("/rest/v1/");

    int httpCode = globalHttps.GET();

    Serial.printf("Supabase test: %d\n", httpCode);
    Serial.println(globalHttps.getString());

    if (httpCode == 200)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cloud Connected");
        delay(2000);
    }
    else
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cloud Error");
        delay(2000);
    }

    cleanupHttpClient();
}

void logAccessRecord(String action, bool success, String pin)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi not connected");
        delay(2000);
        return;
    }

    if (!beginHttpRequest("/rest/v1/access_records"))
    {
        Serial.println("[HTTPS] Unable to connect");
        return;
    }

    String payload = "{";
    payload += "\"action\":\"" + String(action) + "\",";
    payload += "\"success\":" + String(success ? "true" : "false") + ",";
    payload += "\"pin\":\"" + String(pin) + "\",";
    payload += "\"device_id\":\"ESP32_001\"";
    payload += "}";

    int httpCode = globalHttps.POST(payload);

    if (httpCode > 0)
    {
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

        String response = globalHttps.getString();

        Serial.println("Response: " + response);
    }

    else
    {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", globalHttps.errorToString(httpCode));
    }

    cleanupHttpClient();

    return;
}

bool sendOtpRecord()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected");
        return false;
    }

    currentOtp = String(random(10000, 99999));

    if (!beginHttpRequest("/rest/v1/otp_records"))
    {
        Serial.println("[HTTPS] Unable to connect");
        return false;
    }

    String payload = "{";
    payload += "\"otp\":\"" + currentOtp + "\",";
    payload += "\"status\":\"pending\",";
    payload += "\"device_id\":\"ESP32_001\"";
    payload += "}";

    int httpCode = globalHttps.POST(payload);

    if (httpCode > 0)
    {
        Serial.printf("OTP sent successfully, code: %d\n", httpCode);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("OTP Generated");
        delay(3000);
        cleanupHttpClient();

        return true;
    }
    else
    {
        Serial.printf("Failed to send OTP, code: %d\n", httpCode);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("OTP Send Failed");
        delay(2000);

        currentState = STATE_DENIED;
        stateChanged = true;

        cleanupHttpClient();

        return false;
    }
}

void verifyOtp(String inputOtp)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        displayMessage("No WiFi!", "", 2000);
        Serial.println("WiFi not connected");

        currentState = STATE_DENIED;
        stateChanged = true;

        return;
    }

    if (!beginHttpRequest("/rest/v1/otp_records?otp=eq." + inputOtp + "&status=eq.pending"))
    {
        displayMessage("Connection Error", "", 2000);

        currentState = STATE_DENIED;
        stateChanged = true;

        return;
    }

    int httpCode = globalHttps.GET();
    String response = globalHttps.getString();
    bool otpValid = response.length() > 2;

    if (httpCode < 0)
    {
        Serial.printf("HTTP Error: %s\n", globalHttps.errorToString(httpCode).c_str());
    }

    if (httpCode != 200)
    {
        Serial.printf("HTTP Status Error: %d\n", httpCode);
    }

    if (httpCode != 201 && httpCode != 200)
    {
        Serial.printf("Failed to verify OTP, code: %d\n", httpCode);
        displayMessage("Verify Error", "", 2000);
        updateOtpStatus(inputOtp, "failed");

        currentState = STATE_DENIED;
        stateChanged = true;

        return;
    }

    else if (otpValid)
    {
        updateOtpStatus(inputOtp, "verified");
        logAccessRecord("OTP_ENTRY", true, inputOtp);
        displayMessage("OTP Verified!", ("PIN: " + String(PASSWORD)).c_str(), 5000);
        displayMessage("Access Granted", "", 2000);

        currentState = STATE_UNLOCKED;
        stateChanged = true;

        return;
    }

    else
    {
        displayMessage("Invalid OTP!", "", 2000);

        logAccessRecord("OTP_ENTRY", false, inputOtp);
        currentState = STATE_DENIED;
        stateChanged = true;

        return;
    }
}

void updateOtpStatus(String otp, const char *status)
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    if (!beginHttpRequest(String("/rest/v1/otp_records?otp=eq." + otp)))
    {
        Serial.println("[HTTPS] Unable to connect");
        return;
    }

    String payload = "{\"status\":\"" + String(status) + "\"}";
    globalHttps.PATCH(payload);
    cleanupHttpClient();
}
