#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_camera.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

const char *ssid = "your-ssid";
const char *password = "your-password";

const int captureInterval = 5000; // Capture image every 5 seconds
unsigned long lastCaptureTime = 0;

// Create an instance of the server
AsyncWebServer server(80);

// Face detection variables
CascadeClassifier faceCascade;
const int minFaceSize = 30;

bool drowsinessDetected = false;

void setup()
{
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Load the Haar Cascade Classifier for face detection
  if (!faceCascade.load("/haarcascade_frontalface_default.xml"))
  {
    Serial.println("Failed to load face cascade classifier");
    while (1);
  }

  // Serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>ESP32-CAM Sleep Detection</h1>";
    
    if (drowsinessDetected)
    {
      html += "<p>Drowsiness detected! Person is asleep.</p>";
    }
    else
    {
      html += "<p>Person is awake.</p>";
    }

    html += "<img src='/image' style='width:100%;'/>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Serve the captured image
  server.on("/image", HTTP_GET, [](AsyncWebServerRequest *request){
    // Capture an image and send it as response
    captureImage(request->client());
  });

  // Start server
  server.begin();
}

void loop()
{
  // Capture image at regular intervals
  if (millis() - lastCaptureTime >= captureInterval)
  {
    drowsinessDetected = analyzeImage();
    lastCaptureTime = millis();
  }
}

void captureImage(Print &output)
{
  // Use the ESP32-CAM library to capture an image
  output.println("CAPTURE");
  delay(2000); // Allow time for capture
}

bool analyzeImage()
{
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Failed to capture image");
    return false;
  }

  Mat img = Mat(fb->height, fb->width, CV_8UC3, fb->buf);
  Mat gray;
  cvtColor(img, gray, COLOR_BGR2GRAY);

  // Detect faces in the image
  std::vector<Rect> faces;
  faceCascade.detectMultiScale(gray, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(minFaceSize, minFaceSize));

  // Draw rectangles around detected faces
  for (const Rect &face : faces)
  {
    rectangle(img, face, Scalar(255, 0, 0), 2);
  }

  // Release the captured image buffer
  esp_camera_fb_return(fb);

  // Return true if faces are detected, false otherwise
  return !faces.empty();
}
