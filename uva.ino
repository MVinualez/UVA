#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "<ssid>";
const char* password = "<passwd>";

// Default parameters
float latitude = 44.8393;           // Latitude for the weather API
float longitude = -0.5754;          // Longitude for the weather API
String apiKey = "5796abbde9106b7da4febfae8c44c232"; // API key for OpenWeatherMap
unsigned long interval = 60000;      // Update interval (in milliseconds)

// FastLED parameters
#define LED_PIN     13               // Pin connected to the LED strip
#define NUM_LEDS    64               // Number of LEDs in the strip
#define MAX_BRIGHTNESS 255           // Maximum brightness of the LEDs
#define LED_TYPE    WS2812B          // Type of LED used
#define COLOR_ORDER GRB              // Color order for the LEDs
CRGB leds[NUM_LEDS];                 // Array to hold LED colors

CRGBPalette16 currentPalette;         // Current color palette
TBlendType    currentBlending;        // Current blending type

// Web server
WebServer server(80);                 // Create a web server on port 80

void setup() {
    Serial.begin(115200);             // Start serial communication at 115200 baud

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Configure LEDs
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(MAX_BRIGHTNESS);
    currentPalette = RainbowColors_p;   // Set initial color palette
    currentBlending = LINEARBLEND;      // Set initial blending type

    // Configure web server routes
    server.on("/", handleRoot);          // Route for the root page
    server.on("/update", handleUpdate);  // Route for updating parameters
    server.begin();                      // Start the web server
    Serial.println("Web server started!");

    float uvi = getUVIndex();           // Get initial UV index
    updateLEDs(-1);                     // Reset LED matrix
    updateLEDs(uvi);                    // Update LEDs based on initial UV index
}

void loop() {
    // Handle web server requests
    server.handleClient();

    // Update UV index and LEDs
    static unsigned long previousMillis = 0; // Store the last time the UV index was updated
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;      // Update the previous time
        float uvi = getUVIndex();           // Get the current UV index
        updateLEDs(-1);                     // Reset LED matrix
        updateLEDs(uvi);                    // Update LEDs based on current UV index
    }
}

// Generate the API URL with the current parameters
String getApiUrl() {
    return "https://api.openweathermap.org/data/2.5/onecall?lat=" + String(latitude) + "&lon=" + String(longitude) + "&units=metric&appid=" + apiKey;
}

// Handle the root route "/"
void handleRoot() {
    String message = "<html><body><h1>Application Settings</h1>";
    message += "<form action=\"/update\" method=\"get\">";
    message += "Latitude: <input type=\"text\" name=\"latitude\" value=\"" + String(latitude) + "\"><br>";
    message += "Longitude: <input type=\"text\" name=\"longitude\" value=\"" + String(longitude) + "\"><br>";
    message += "API Key: <input type=\"text\" name=\"apikey\" value=\"" + apiKey + "\"><br>";
    message += "Interval (ms): <input type=\"text\" name=\"interval\" value=\"" + String(interval) + "\"><br>";
    message += "<input type=\"submit\" value=\"Update\"></form>";
    message += "</body></html>";
    server.send(200, "text/html", message); // Send HTML response
}

// Handle the "/update" route
void handleUpdate() {
    // Update parameters based on the input from the web form
    if (server.hasArg("latitude")) {
        latitude = server.arg("latitude").toFloat();
    }
    if (server.hasArg("longitude")) {
        longitude = server.arg("longitude").toFloat();
    }
    if (server.hasArg("apikey")) {
        apiKey = server.arg("apikey");
    }
    if (server.hasArg("interval")) {
        interval = server.arg("interval").toInt();
    }
    server.send(200, "text/plain", "Parameters updated"); // Send confirmation response
    Serial.println("Parameters updated:");
    Serial.print("Latitude: ");
    Serial.println(latitude);
    Serial.print("Longitude: ");
    Serial.println(longitude);
    Serial.print("API Key: ");
    Serial.println(apiKey);
    Serial.print("Interval: ");
    Serial.println(interval);
}

// Retrieve the UV index from the API
float getUVIndex() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(getApiUrl()); // Start the HTTP request
        Serial.println(getApiUrl()); // Log the API URL

        int httpCode = http.GET(); // Send the HTTP GET request
        if (httpCode > 0) {
            String payload = http.getString(); // Get the response payload
            Serial.println(payload); // Log the payload

            DynamicJsonDocument doc(1024); // Create a JSON document to parse the response
            DeserializationError error = deserializeJson(doc, payload); // Parse the JSON

            if (!error) {
                float uvIndex = doc["current"]["uvi"]; // Get the UV index from the parsed JSON
                Serial.print("UV Level: ");
                Serial.println(uvIndex);
                return uvIndex; // Return the UV index
            } else {
                Serial.println("Error parsing JSON");
                return 0; // Return 0 on error
            }
        } else {
            Serial.println("HTTP request error");
            return 0; // Return 0 on error
        }
        http.end(); // End the HTTP request
    } else {
        Serial.println("WiFi connection error");
        return 0; // Return 0 on WiFi error
    }
}

// Update LEDs based on the UV index
void updateLEDs(float uvi) {
    if (uvi < 0) {
        // Turn off all LEDs if UV is negative
        fillLEDs(CRGB::Black);
    } else if (uvi < 2) {
        // Light up only the diagonals if UV is low
        fillDiagonals(CRGB::Purple);
    } else {
        // If UV is high, light up the entire matrix
        fillLEDs(CRGB::Purple);
    }
}

// Fill the entire LED matrix with a color
void fillLEDs(CRGB color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color; // Set each LED to the specified color
    }
    FastLED.show(); // Update the LED strip
}

// Fill the LEDs in diagonal patterns
void fillDiagonals(CRGB color) {
    // Fill the 8x8 matrix with diagonal LEDs
    for (int i = 0; i < 8; i++) {
        // Main diagonal LED
        leds[i * 8 + i] = color; // Set the main diagonal LED
        // Secondary diagonal LED
        leds[i * 8 + (7 - i)] = color; // Set the secondary diagonal LED
    }
    FastLED.show(); // Update the LED strip
}
