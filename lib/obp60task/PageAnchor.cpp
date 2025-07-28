#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

#include "Pagedata.h"
#include "OBP60Extensions.h"

/*
  Anchor overview with additional associated data
  This page is in experimental stage so be warned!
  North is up.

  Boatdata used
    DBS - Water depth
    HDT - Boat heading
    AWS - Wind strength; Boat not moving so we assume AWS=TWS and AWD=TWD
    AWD - Wind direction
    LAT/LON - Boat position, current
    HDOP - Position error

  This is the fist page to contain a configuration page with 
  data entry option.
  Also it will make use of the new alarm function.

  Data
    Anchor position lat/lon
    Depth at anchor position
    Chain length used
    Boat position current
    Depth at boat position
    Boat heading
    Wind direction
    Wind strength
    Alarm j/n
    Alarm radius
    GPS position error
    Timestamp while dropping anchor

   Drop / raise function in device OBP40 has to be done inside 
   config mode because of limited number of buttons.

*/

#define anchor_width 16
#define anchor_height 16
static unsigned char anchor_bits[] = {
   0x80, 0x01, 0x40, 0x02, 0x40, 0x02, 0x80, 0x01, 0xf0, 0x0f, 0x80, 0x01,
   0x80, 0x01, 0x88, 0x11, 0x8c, 0x31, 0x8e, 0x71, 0x84, 0x21, 0x86, 0x61,
   0x86, 0x61, 0xfc, 0x3f, 0xf8, 0x1f, 0x80, 0x01 };

class PageAnchor : public Page
{
    private:
    GwConfigHandler *config;
    GwLog *logger;
    bool simulation = false;
    bool holdvalues = false;
    String flashLED;
    String backlightMode;

    bool alarm = false;
    bool alarm_enabled = false;
    uint8_t alarm_range;

    uint8_t chain_length;
    uint8_t chain;

    bool anchor_set = false;
    double anchor_lat;
    double anchor_lon;
    double anchor_depth;
    int anchor_ts; // time stamp anchor dropped

    char mode = 'N'; // (N)ormal, (C)onfig

    void displayModeNormal(PageData &pageData) {

        // Boatvalues: DBS, HDT, AWS, AWD, LAT, LON, HDOP
        GwApi::BoatValue *bv_dbs = pageData.values[0]; // DBS
        String sval_dbs = formatValue(bv_dbs, *commonData).svalue;
        GwApi::BoatValue *bv_hdt = pageData.values[1]; // HDT
        String sval_hdt = formatValue(bv_hdt, *commonData).svalue;
        GwApi::BoatValue *bv_aws = pageData.values[2]; // AWS
        String sval_aws = formatValue(bv_aws, *commonData).svalue;
        GwApi::BoatValue *bv_awd = pageData.values[3]; // AWD
        String sval_awd = formatValue(bv_awd, *commonData).svalue;
        GwApi::BoatValue *bv_lat = pageData.values[4]; // LAT
        String sval_lat = formatValue(bv_lat, *commonData).svalue;
        GwApi::BoatValue *bv_lon = pageData.values[5]; // LON
        String sval_lon = formatValue(bv_lon, *commonData).svalue;
        GwApi::BoatValue *bv_hdop = pageData.values[6]; // HDOP
        String sval_hdop = formatValue(bv_hdop, *commonData).svalue;

        Point c = {200, 150}; // center = anchor position
        uint16_t r = 125;

        Point b = {200, 180}; // boat position while dropping anchor

        std::vector<Point> pts_boat = { // polygon lines
            {b.x - 5, b.y},
            {b.x - 5, b.y - 10},
            {b.x, b.y - 16},
            {b.x + 5, b.y - 10}
        };
        //rotatePoints und dann Linien zeichnen
        // TODO rotate boat according to current heading
        //fillPoly4(rotatePoints(c, pts, RadToDeg(value2)), commonData->fgcolor);

        // Draw wind arrow
        /*
        if self._bd.awa.value:
            p = ((cx, cy - r + 25), (cx - 12, cy - r - 4), (cx, cy - r + 6), (cx + 12, cy - r - 4), (cx, cy - r + 25))
            wind = self.rotate((cx, cy), p, self._bd.awa.value)
            ctx.move_to(*wind[0])
            for point in wind[1:]:
                ctx.line_to(*point)
            ctx.fill()
        */

        // Title and corner value headings
        getdisplay().setTextColor(commonData->fgcolor);
        getdisplay().setFont(&Ubuntu_Bold10pt8b);
        getdisplay().setCursor(8, 48);
        getdisplay().print("Anchor");

        getdisplay().setCursor(8, 200);
        getdisplay().print("Depth");

        drawTextRalign(392, 50, "Chain");
        drawTextRalign(392, 200, "Wind");

        // Corner values
        getdisplay().setFont(&Ubuntu_Bold8pt8b);
        getdisplay().setCursor(2, 70);
        getdisplay().print("Alarm: ");
        getdisplay().print(alarm_enabled ? "On" : "Off");
        
        // Units

        // Values
        getdisplay().setFont(&DSEG7Classic_BoldItalic20pt7b);
        // Depth
        getdisplay().setCursor(8, 250);
        //getdisplay().print(sval_dbs);
        getdisplay().print("6.4");
        // Wind
        getdisplay().setCursor(320, 250);
        //getdisplay().print(sval_aws);
        getdisplay().print("12");

        getdisplay().drawCircle(c.x, c.y, r, commonData->fgcolor);
 
        // draw anchor symbol (as bitmap)
        getdisplay().drawXBitmap(c.x - anchor_width / 2, c.y - anchor_height / 2,
                                 anchor_bits, anchor_width, anchor_height, commonData->fgcolor);

    }

    void displayModeConfig() {
        getdisplay().setTextColor(commonData->fgcolor);
        getdisplay().setFont(&Ubuntu_Bold12pt8b);
        getdisplay().setCursor(8, 48);
        getdisplay().print("Anchor configuration");
    }

    public:
    PageAnchor(CommonData &common){
        commonData = &common;
        config = commonData->config;
        logger = commonData->logger;
        logger->logDebug(GwLog::LOG,"Instantiate PageAnchor");

        // preload configuration data
        simulation = config->getBool(config->useSimuData);
        holdvalues = config->getBool(config->holdvalues);
        flashLED = config->getString(config->flashLED);
        backlightMode = config->getString(config->backlight);
        chainLength = config->getInt(config->chainLength);
       
        chain = 0;
        anchor_set = false;
     }

    void setupKeys(){
        Page::setupKeys();
        commonData->keydata[0].label = "MODE";
        commonData->keydata[1].label = "ALARM";
    }

    int handleKey(int key){
        if (key == 1) { // Switch between normal and config mode
            if (mode == 'N') {
                mode = 'C';
            } else {
                mode = 'N';
            }
            return 0;
        }
        if (key == 2) { // Toggle alarm
            alarm_enabled = !alarm_enabled;
            return 0;
        }
        if (key == 11) { // Code for keylock
            commonData->keylock = !commonData->keylock;
            return 0;
        }
        return key;
    }

    void displayNew(PageData &pageData){
    };

    int displayPage(PageData &pageData){

        // Logging boat values
        LOG_DEBUG(GwLog::LOG,"Drawing at PageAnchor");
        LOG_DEBUG(GwLog::LOG,"Drawing at PageAnchor; Mode=%c", mode);

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

       if (mode == 'N') {
           displayModeNormal(pageData);
       } else if (mode == 'C') {
           displayModeConfig();
       }

        return PAGE_UPDATE;
    };
};

static Page *createPage(CommonData &common){
    return new PageAnchor(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageAnchor(
    "Anchor",   // Page name
    createPage, // Action
    0,          // Number of bus values depends on selection in Web configuration
    {"DBS", "HDT", "AWS", "AWD", "LAT", "LON", "HDOP"}, // Names of bus values undepends on selection in Web configuration (refer GwBoatData.h)
    true        // Show display header on/off
);

#endif
