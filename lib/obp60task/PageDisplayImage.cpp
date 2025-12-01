#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "BoatDataCalibration.h"

class PageDisplayImage : public Page
{
    public:
    PageDisplayImage(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageDisplayImage");
    }

    virtual int handleKey(int key){
        // Code for keylock
        if(key == 11){
            commonData->keylock = !commonData->keylock;
            return 0;                   // Commit the key
        }
        return key;
    }

    int displayPage(PageData &pageData){
        GwConfigHandler *config = commonData->config;
        GwLog *logger = commonData->logger;

        // Old values for hold function
        static String svalue1old = "";
        static String unit1old = "";

        // Get config data
        String lengthformat = config->getString(config->lengthFormat);
        // bool simulation = config->getBool(config->useSimuData);
        bool holdvalues = config->getBool(config->holdvalues);
        String flashLED = config->getString(config->flashLED);
        String backlightMode = config->getString(config->backlight);
        
        // Get boat values
        GwApi::BoatValue *bvalue1 = pageData.values[0]; // First element in list (only one value by PageOneValue)
        String name1 = xdrDelete(bvalue1->getName());   // Value name
        name1 = name1.substring(0, 6);                  // String length limit for value name
        calibrationData.calibrateInstance(bvalue1, logger); // Check if boat data value is to be calibrated
        double value1 = bvalue1->value;                 // Value as double in SI unit
        bool valid1 = bvalue1->valid;                   // Valid information
        String svalue1 = formatValue(bvalue1, *commonData).svalue;    // Formatted value as string including unit conversion and switching decimal places
        String unit1 = formatValue(bvalue1, *commonData).unit;        // Unit of value

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        if (bvalue1 == NULL) return PAGE_OK; // WTF why this statement?
        LOG_DEBUG(GwLog::LOG,"Drawing at PageDisplayImage, %s: %f", name1.c_str(), value1);

        // Draw page
        //***********************************************************
void loop() {
    // Rotate the picture in 1° steps
    int angle = loopCounter % 360;

    // Server settings 
    String server = "norbert-walter.dnshome.de";
    int port = 80;

    // URL to OBP Maps Converter
    // For more details see: https://github.com/norbert-walter/maps-converter
    String url = String("http://") + server + ":" + port +  // OBP Server
                 String("/get_image_json?") +               // Service: Output B&W picture as JSON (Base64 + gzip)
                 "zoom=15" +        // Zoom level: 15
                 "&lat=53.9028" +   // Latitude
                 "&lon=11.4441" +   // Longitude
                 "&mrot=" + angle + // Rotation angle navigation map
                 "&mtype=9" +       // Free Nautical Charts with depth
                 "&dtype=1" +       // Dithering type: Threshold dithering
                 "&width=400" +     // With navigation map
                 "&height=250" +    // Height navigation map
                 "&cutout=0" +      // No picture cutouts
                 "&tab=0" +         // No tab size
                 "&border=2" +      // Border line size: 2 pixel
                 "&symbol=2" +      // Symbol: Triangle
                 "&srot=" + angle + // Symbol rotation angle
                 "&ssize=15" +      // Symbole size: 15 pixel
                 "&grid=1"          // Show grid: On
                 ;         

  //http://norbert-walter.dnshome.de:80/get_image_json?&zoom=15&lat=53.9028&lon=11.4441&mrot=0&mtype=9&dtype=1&width=400&height=250&cutout=0&tab=0&border=2&symbol=2&srot=0&ssize=15&grid=1
    
    // If a network connection to URL
    if (net.fetchAndDecompressJson(url)) {        // Connect to URL, read gzip answare and deflate JSON content
        auto& json = net.json();                  // Parse JSON content
        int numPix = json["number_pixels"] | 0;   // Read number of picture pixels
        String b64 = json["picture_base64"] | ""; // Read the Base64 bit steram content (picture)
        static uint8_t imageData[400 * 300];      // Set picture buffer
        size_t decodedSize = 0;                   // Reset decoded size of Basse64 bit stream content

        decoder.decodeBase64(b64, imageData, sizeof(imageData), decodedSize); // Decode Base64 bit stream content

        // Render image
        // After 60 images full refresh
        if((loopCounter % 60) == 0){
          display.setFullWindow(); 
        }
        // Otherwise partial refresh
        else{
          display.setPartialWindow(0, 25, display.width(), display.height());
        }
        display.fillScreen(GxEPD_WHITE);  // Clear screen
        display.drawBitmap(0, 25, imageData, display.width(), display.height(), GxEPD_BLACK); // Show picture with Y offset 25 pixel
        display.nextPage();
        }

    loopCounter++;  // Counter value + 1
    Serial.printf("Free Heap: %u Bytes\n", ESP.getFreeHeap());
}
        /// Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        // Show name
        getdisplay().setTextColor(commonData->fgcolor);
        getdisplay().setFont(&Ubuntu_Bold32pt8b);
        getdisplay().setCursor(20, 100);
        getdisplay().print(name1);                           // Page name

        // Show unit
        getdisplay().setFont(&Ubuntu_Bold20pt8b);
        getdisplay().setCursor(270, 100);
        if(holdvalues == false){
            getdisplay().print(unit1);                       // Unit
        }
        else{
            getdisplay().print(unit1old);
        }

        // Switch font if format for any values
        if(bvalue1->getFormat() == "formatLatitude" || bvalue1->getFormat() == "formatLongitude"){
            getdisplay().setFont(&Ubuntu_Bold20pt8b);
            getdisplay().setCursor(20, 180);
        }
        else if(bvalue1->getFormat() == "formatTime" || bvalue1->getFormat() == "formatDate"){
            getdisplay().setFont(&Ubuntu_Bold32pt8b);
            getdisplay().setCursor(20, 200);
        }
        else{
            getdisplay().setFont(&DSEG7Classic_BoldItalic60pt7b);
            getdisplay().setCursor(20, 240);
        }

        // Show bus data
        if(holdvalues == false){
            getdisplay().print(svalue1);                                     // Real value as formated string
        }
        else{
            getdisplay().print(svalue1old);                                  // Old value as formated string
        }
        if(valid1 == true){
            svalue1old = svalue1;                                       // Save the old value
            unit1old = unit1;                                           // Save the old unit
        }

        return PAGE_UPDATE;
    };
};

static Page* createPage(CommonData &common){
    return new PageDisplayImage(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageDisplayImage(
    "DisplayImage",     // Page name
    createPage,     // Action
    1,              // Number of bus values depends on selection in Web configuration
    true            // Show display header on/off
);

#endif
