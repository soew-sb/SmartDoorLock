#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Pin Definitions
#define RELAY_PIN 23       // Relay control pin connected to GPIO 23 on ESP32
#define RED_LED_PIN 18     // Red LED pin for Access Denied
#define GREEN_LED_PIN 19   // Green LED pin for Access Granted
#define PASSWORD "2024"    // Define your password here
#define TIMEOUT_SECONDS 10 // Timeout in seconds for PIN entry

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
}

void loop()
{
    char key = keypad.getKey();

    if (welcomeScreen)
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
            // return;
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
        else
        {
            // If not blinking, show the full welcome message
            lcd.setCursor(0, 0);
            lcd.print("Welcome Home.    ");
            lcd.setCursor(0, 1);
            lcd.print("Press 0 to unlock");
        }

        // return; // Exit the loop to avoid further processing while in welcomeScreen
    }

    // Handle PIN entry
    if (key)
    {
        lastKeypressTime = millis();          // Update last keypress time
        lcd.setCursor(inputCode.length(), 1); // Move to the second row for PIN entry display
        lcd.print("*");

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
                digitalWrite(RED_LED_PIN, HIGH); // Turn on red LED
                delay(2000);
                digitalWrite(RED_LED_PIN, LOW); // Turn off red LED
                showWelcomeScreen();
            }
            inputCode = ""; // Reset the code after entry
        }
        else if (key == '*')
        { // '*' to clear the input
            inputCode = "";
            lcd.setCursor(0, 1);
            lcd.print("                "); // Clear the second row on LCD
            lcd.setCursor(0, 1);
        }
        else
        {
            inputCode += key; // Add key to input code
        }
    }

    // Timeout handling: if no input for TIMEOUT_SECONDS, return to welcome screen
    if (millis() - lastKeypressTime > TIMEOUT_SECONDS * 1000 && inputCode != "")
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
