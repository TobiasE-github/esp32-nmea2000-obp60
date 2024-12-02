#ifdef BOARD_OBP60S3

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "GwXDRMappings.h"

/*
  Fluid level view

    0: "Fuel",
    1: "Water",
    2: "Gray Water",
    3: "Live Well",
    4: "Oil",
    5: "Black Water",
    6: "Fuel Gasoline",
    14: "Error",
    15: "Unavailable"

TODO 
  - Check fluid type against connected XDR-value

*/

struct Point {
    double x;
    double y;
};

Point rotatePoint(const Point& origin, const Point& p, double angle) {
    // rotate poind around origin by degrees
    Point rotated;
    double phi = angle * M_PI / 180.0;
    double dx = p.x - origin.x;
    double dy = p.y - origin.y;
    rotated.x = origin.x + cos(phi) * dx - sin(phi) * dy;
    rotated.y = origin.y + sin(phi) * dx + cos(phi) * dy;
    return rotated;
}

std::vector<Point> rotatePoints(const Point& origin, const std::vector<Point>& pts, double angle) {
    std::vector<Point> rotatedPoints;
    for (const auto& p : pts) {
         rotatedPoints.push_back(rotatePoint(origin, p, angle));
    }
     return rotatedPoints;
}

void fillPoly4(const std::vector<Point>& p4, int color) {
    getdisplay().fillTriangle(p4[0].x, p4[0].y, p4[1].x, p4[1].y, p4[2].x, p4[2].y, color);
    getdisplay().fillTriangle(p4[0].x, p4[0].y, p4[2].x, p4[2].y, p4[3].x, p4[3].y, color);
}

void drawTextCentered(int16_t tx, int16_t ty, String text) {
    int16_t x, y;
    uint16_t w, h;
    getdisplay().getTextBounds(text, 0, 0, &x, &y, &w, &h);
    getdisplay().setCursor(tx - w / 2, ty + h / 2);
    getdisplay().print(text);
}

#define fuel_width 16
#define fuel_height 16
static unsigned char fuel_bits[] = {
   0xfc, 0x03, 0xfe, 0x67, 0x06, 0x86, 0x06, 0x86, 0x06, 0xe6, 0x06, 0xe6,
   0x06, 0xe6, 0x06, 0xe6, 0xfe, 0x47, 0xfe, 0x47, 0xfe, 0x4f, 0xfe, 0x57,
   0xfe, 0x57, 0xfe, 0x77, 0xfe, 0x07, 0xff, 0x0f };

#define water_width 16
#define water_height 16
static unsigned char water_bits[] = {
   0x00, 0x00, 0x80, 0x01, 0xfc, 0x3f, 0xfc, 0x3f, 0x80, 0x01, 0xc0, 0x03,
   0xc0, 0xc3, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xc7,
   0x87, 0x01, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00 };

#define waste_width 16
#define waste_height 16
static unsigned char waste_bits[] = {
   0xf0, 0x00, 0x9f, 0x00, 0x80, 0x07, 0x80, 0x08, 0x80, 0x10, 0x9f, 0x23,
   0xf0, 0x24, 0x00, 0x64, 0x10, 0x25, 0x40, 0xa4, 0x08, 0x66, 0xff, 0xff,
   0x00, 0x00, 0xbc, 0x6e, 0x00, 0x00, 0xf0, 0x19 };

#define oil_width 16
#define oil_height 16
static unsigned char oil_bits[] = {
   0x00, 0x01, 0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xc0, 0x07, 0xe0, 0x0f,
   0xe0, 0x0f, 0xf0, 0x1f, 0xf0, 0x1f, 0xf8, 0x3f, 0xd8, 0x3f, 0xd8, 0x3f,
   0xb8, 0x3f, 0x70, 0x1e, 0xe0, 0x0f, 0xc0, 0x07 };

#define gasoline_width 16
#define gasoline_height 16
static unsigned char gasoline_bits[] = {
   0x0f, 0x00, 0xdf, 0x7f, 0xf8, 0xff, 0x70, 0xe0, 0xf8, 0xff, 0xf8, 0xff,
   0x98, 0xcf, 0x38, 0xe7, 0x78, 0xf0, 0xf8, 0xfa, 0xf8, 0xfa, 0x78, 0xf0,
   0x38, 0xe7, 0x98, 0xcf, 0xf8, 0xff, 0xf0, 0x7f };

class PageFluid : public Page{
    bool keylock = false;               // Keylock

    public:
    PageFluid(CommonData &common){
        common.logger->logDebug(GwLog::LOG,"Show PageFluid");
    }

    virtual int handleKey(int key){
        if(key == 11){                  // Code for keylock
            keylock = !keylock;         // Toggle keylock
            return 0;                   // Commit the key
        }
        return key;
    }

    virtual void displayPage(CommonData &commonData, PageData &pageData){
        GwConfigHandler *config = commonData.config;
        GwLog *logger=commonData.logger;

        // Get config data
        String flashLED = config->getString(config->flashLED);
        String displaycolor = config->getString(config->displaycolor);
        String backlightMode = config->getString(config->backlight);

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false);
        }

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageFluid");

        GwApi::BoatValue *bvalue1 = pageData.values[0];
        String name1 = bvalue1->getName();
        double value1 = bvalue1->value;
        bool valid1 = bvalue1->valid;

        int fluidtype = config->getInt("page" + String(commonData.data.actpage) + "fluid", 0);

        // Draw page
        //***********************************************************

        // Set background color and text color
        int textcolor = GxEPD_BLACK;
        int pixelcolor = GxEPD_BLACK;
        int bgcolor = GxEPD_WHITE;
        if(displaycolor != "Normal"){
            textcolor = GxEPD_WHITE;
            pixelcolor = GxEPD_WHITE;
            bgcolor = GxEPD_BLACK;
        }
        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height());

        // descriptions
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        getdisplay().setCursor(20, 60);
        getdisplay().print("Fluid");

        getdisplay().setCursor(300, 60);
        getdisplay().print(xdrDelete(name1).substring(0, 6));

        // analog instrument
        // scale from -120 to 120

        // center
        Point c = {200, 150};
        uint8_t r = 110;

        // circular frame
        getdisplay().drawCircle(c.x, c.y, r+5, pixelcolor);
        getdisplay().fillCircle(c.x, c.y, r+2, pixelcolor);
        getdisplay().fillCircle(c.x, c.y, r-1, bgcolor);
        // center of pointer as dot 
        getdisplay().fillCircle(c.x, c.y, 8, pixelcolor);

        // value down centered
        char buffer[6];
        if (bvalue1->valid) {
            snprintf(buffer, 6, "%3.0f%%", bvalue1->value);
        } else {
            strcpy(buffer, "---");
        }
        drawTextCentered(c.x, c.y + r - 20, String(buffer));

        // draw symbol (as bitmap)
        switch (fluidtype) {
            case 0:
                getdisplay().drawXBitmap(c.x-8, c.y-50, fuel_bits, fuel_width, fuel_height, pixelcolor);
                break;
            case 1:
                getdisplay().drawXBitmap(c.x-8, c.y-50, water_bits, water_width, water_height, pixelcolor);
                break;
            case 4:
                getdisplay().drawXBitmap(c.x-8, c.y-50, oil_bits, oil_width, oil_height, pixelcolor);
                break;
            case 5:
                getdisplay().drawXBitmap(c.x-8, c.y-50, waste_bits, waste_width, waste_height, pixelcolor);
                break;
            case 6:
                getdisplay().drawXBitmap(c.x-8, c.y-50, gasoline_bits, gasoline_width, gasoline_height, pixelcolor);
                break;
        }

        Point p, pr;

        // scale texts
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        p = {c.x, c.y - r + 30};
        drawTextCentered(p.x, p.y, "1/2");
        pr = rotatePoint(c, p, -60);
        drawTextCentered(pr.x, pr.y, "1/4");
        pr = rotatePoint(c, p, 60);
        drawTextCentered(pr.x, pr.y, "3/4");

        // empty and full
        getdisplay().setFont(&Ubuntu_Bold12pt7b);
        p = rotatePoint(c, {c.x, c.y - r + 30}, -130);
        drawTextCentered(p.x, p.y, "E");
        p = rotatePoint(c, {c.x, c.y - r + 30}, 130);
        drawTextCentered(p.x, p.y, "F");

        // lines
        std::vector<Point> pts = {
            {c.x - 2, c.y - r},
            {c.x + 2, c.y - r},
            {c.x + 2, c.y - (r - 16)},
            {c.x - 2, c.y - (r - 16)} 
        };
        fillPoly4(rotatePoints(c, pts, -120), pixelcolor);
        fillPoly4(rotatePoints(c, pts, -60), pixelcolor);
        fillPoly4(rotatePoints(c, pts, 0), pixelcolor);
        fillPoly4(rotatePoints(c, pts, 60), pixelcolor);
        fillPoly4(rotatePoints(c, pts, 120), pixelcolor);

        // dots
        // rotate 0 to 360 in 12 degree steps
        for (int angle = -120; angle < 120; angle += 12) {
            if (angle == -120 or angle == -60 or angle == 0 or angle == 60) {
                continue;
            }
            p = rotatePoint(c, {c.x, c.y - r + 10}, angle);
            getdisplay().fillCircle(p.x, p.y, 3, pixelcolor);
        }

        // pointer
        if (bvalue1->valid) {
            pts = {
                {c.x - 1, c.y - (r - 20)},
                {c.x + 1, c.y - (r - 20)},
                {c.x + 6, c.y + 15},
                {c.x - 6, c.y + 15}
            };
            fillPoly4(rotatePoints(c, pts, -120 + bvalue1->value * 2.4), pixelcolor);
            // Pointer axis is white
            getdisplay().fillCircle(c.x, c.y, 6, bgcolor);
        }
 
        // Key Layout
        getdisplay().setFont(&Ubuntu_Bold8pt7b);
        if(keylock == false){
            getdisplay().setCursor(130, 296);
            getdisplay().print("[  <<<<  " + String(commonData.data.actpage) + "/" + String(commonData.data.maxpage) + "  >>>>  ]");
            if(String(backlightMode) == "Control by Key"){                  // Key for illumination
                getdisplay().setCursor(343, 296);
                getdisplay().print("[ILUM]");
            }
        }
        else{
            getdisplay().setCursor(130, 296);
            getdisplay().print(" [    Keylock active    ]");
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)

    };
};

static Page* createPage(CommonData &common){
    return new PageFluid(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageFluid(
    "Fluid",                // Page name
    createPage,             // Action
    1,                      // Number of bus values depends on selection in Web configuration
    true                    // Show display header on/off
);

#endif