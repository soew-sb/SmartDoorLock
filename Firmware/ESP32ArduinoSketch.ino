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

// WiFi Credentials
const char *ssid = "Holiday";
const char *password = "K0b14[53";
const char *server = "www.httpbin.org";

WiFiClientSecure client;

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

// Flags for Display States
bool welcomeScreen = true;       // Flag to show welcome screen
bool showBlink = false;          // Flag to indicate blinking "Press 0!"
bool blinkState = false;         // Current state of the blink (on/off)
unsigned long lastBlinkTime = 0; // To track the last blink time
int previousInputLength = 0;     // To track the previous input length

void setup()
{
    Serial.begin(115200);
    Wire.begin(21, 22); // Initialize I2C with SDA on GPIO 21 and SCL on GPIO 22

    lcd.init();      // Initialize the LCD
    delay(100);      // Brief delay for LCD to initialize
    lcd.backlight(); // Turn on the backlight

    // Initialize Relay and LEDs
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Start with the relay off (lock engaged)

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);

    // Initial state: Both LEDs off
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);

    showWelcomeScreen(); // Display the initial welcome screen

    connectToWiFi();
    makeHTTPRequest();
}

void loop()
{
    char key = keypad.getKey();

    if (welcomeScreen)
    {
        handleWelcomeScreen(key);
    }
    else
    {
        handlePINEntry(key);
    }

    // Timeout handling: if no input for TIMEOUT_SECONDS, return to welcome screen
    if (millis() - lastKeypressTime > TIMEOUT_SECONDS * 1000 && inputCode != "")
    {
        handleTimeout();
    }
}

void connectToWiFi()
{
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // attempt to connect to Wifi network:
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        // wait 1 second for re-trying
        delay(1000);
    }

    Serial.print("Connected to ");
    Serial.println(ssid);
}

void makeHTTPRequest()
{
    client.setInsecure();

    Serial.println("\nStarting connection to server...");
    if (!client.connect(server, 443))
    {
        Serial.println("Connection failed!");
    }
    else
    {
        Serial.println("Connected to server!");
        // Make a HTTP request:
        client.println("GET https://httpbin.org/get HTTP/1.0");
        client.println("Host: www.howsmyssl.com");
        client.println("Connection: close");
        client.println();

        while (client.connected())
        {
            String line = client.readStringUntil('\n');
            if (line == "\r")
            {
                Serial.println("headers received");
                break;
            }
        }
        // if there are incoming bytes available
        // from the server, read them and print them:
        while (client.available())
        {
            char c = client.read();
            Serial.write(c);
        }

        client.stop();
    }
}

void handleWelcomeScreen(char key)
{
    // If '0' is pressed immediately, go to PIN entry screen
    if (key == '0')
    {
        welcomeScreen = false;
        showBlink = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter PIN:");
        lastKeypressTime = millis();
    }

    // If any key other than '0' is pressed, start blinking "Press 0!"
    if (key && key != '0')
    {
        showBlink = true;
        lastBlinkTime = millis();
    }

    // Handle blinking "Press 0!" if required
    if (showBlink)
    {
        if (millis() - lastBlinkTime > 500)
        { // Toggle every 500 ms
            lastBlinkTime = millis();
            blinkState = !blinkState;
            lcd.setCursor(0, 1);
            if (blinkState)
            {
                lcd.print("Press 0!         "); // Show "Press 0!"
            }
            else
            {
                lcd.print("               "); // Clear the line
            }
        }

        // Always display "Welcome Home." on the first line
        lcd.setCursor(0, 0);
        lcd.print("Welcome Home.    ");
    }
}

void handlePINEntry(char key)
{
    if (key)
    {
        lastKeypressTime = millis(); // Update last keypress time

        if (key == '#')
        { // '#' acts as the Enter key
            if (inputCode == PASSWORD)
            {
                unlockDoor();
            }
            else
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Access Denied");
                digitalWrite(RED_LED_PIN, HIGH);
                delay(2000);
                digitalWrite(RED_LED_PIN, LOW);
                showWelcomeScreen();
            }
            inputCode = "";
            previousInputLength = 0;
        }
        else if (key == '*')
        { // '*' to clear the input
            inputCode = "";
            previousInputLength = 0;
            lcd.setCursor(0, 1);
            lcd.print("                "); // Clear the second row
        }
        else
        {
            inputCode += key;

            // Clear the entire second row first
            lcd.setCursor(0, 1);
            lcd.print("                ");

            // Display asterisks for the current input
            lcd.setCursor(0, 1);
            for (int i = 0; i < inputCode.length(); i++)
            {
                lcd.print("*");
            }

            previousInputLength = inputCode.length();
        }
    }
}

void handleTimeout()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    digitalWrite(RED_LED_PIN, HIGH); // Turn on red LED for timeout
    delay(2000);
    digitalWrite(RED_LED_PIN, LOW); // Turn off red LED
    showWelcomeScreen();
    inputCode = ""; // Reset input code
}

void showWelcomeScreen()
{
    welcomeScreen = true;
    showBlink = false;
    blinkState = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Welcome Home.    ");
    lcd.setCursor(0, 1);
    lcd.print("Press 0 to unlock");
}

void unlockDoor()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");
    digitalWrite(GREEN_LED_PIN, HIGH); // Turn on green LED
    digitalWrite(RELAY_PIN, HIGH);     // Activate relay (engage solenoid to unlock)
    delay(5000);                       // Keep door unlocked for 5 seconds
    digitalWrite(RELAY_PIN, LOW);      // Lock again
    digitalWrite(GREEN_LED_PIN, LOW);  // Turn off green LED
    showWelcomeScreen();
}
