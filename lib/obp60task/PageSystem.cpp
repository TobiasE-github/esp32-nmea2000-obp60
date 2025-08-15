#if defined BOARD_OBP60S3 || defined BOARD_OBP40S3

/*
 * Special system page, called directly with fast key sequence 5,4
 * Out of normal page order.
 * Consists of some sub-pages with following content:
 *   1. Hard and software information
 *   2. System settings
 *   3. NMEA2000 device list
 *   4. SD Card information if available
 */

#include "Pagedata.h"
#include "OBP60Extensions.h"
#include "images/logo64.xbm"
#include <esp32/clk.h>
#include "qrcode.h"

#ifdef BOARD_OBP40S3
#include "dirent.h"
#endif

#define STRINGIZE_IMPL(x) #x
#define STRINGIZE(x) STRINGIZE_IMPL(x)
#define VERSINFO STRINGIZE(GWDEVVERSION)
#define BOARDINFO STRINGIZE(BOARD)
#define PCBINFO STRINGIZE(PCBVERS)
#define DISPLAYINFO STRINGIZE(EPDTYPE)
#define GXEPD2INFO STRINGIZE(GXEPD2VERS)

class PageSystem : public Page
{
private:
    uint64_t chipid;
    bool simulation;
    bool use_sdcard;
    String buzzer_mode;
    uint8_t buzzer_power;
    String cpuspeed;
    String rtc_module;
    String gps_module;
    String env_module;

    String batt_sensor;
    String solar_sensor;
    String gen_sensor;
    String rot_sensor;
    double homelat;
    double homelon;

    char mode = 'N'; // (N)ormal, (S)ettings, (D)evice list, (C)ard

public:
    PageSystem(CommonData &common){
        commonData = &common;
        common.logger->logDebug(GwLog::LOG,"Instantiate PageSystem");
        if (hasFRAM) {
            mode = fram.read(FRAM_SYSTEM_MODE);
            common.logger->logDebug(GwLog::DEBUG, "Loaded mode '%c' from FRAM", mode);
        }
        chipid = ESP.getEfuseMac();
        simulation = common.config->getBool(common.config->useSimuData);
#ifdef BOARD_OBP40S3
        use_sdcard = common.config->getBool(common.config->useSDCard);
#endif
        buzzer_mode = common.config->getString(common.config->buzzerMode);
        buzzer_mode.toLowerCase();
        buzzer_power = common.config->getInt(common.config->buzzerPower);
        cpuspeed = common.config->getString(common.config->cpuSpeed);
        env_module = common.config->getString(common.config->useEnvSensor);
        rtc_module = common.config->getString(common.config->useRTC);
        gps_module = common.config->getString(common.config->useGPS);
        batt_sensor = common.config->getString(common.config->usePowSensor1);
        solar_sensor = common.config->getString(common.config->usePowSensor2);
        gen_sensor = common.config->getString(common.config->usePowSensor3);
        rot_sensor = common.config->getString(common.config->useRotSensor);
        homelat = common.config->getString(common.config->homeLAT).toDouble();
        homelon = common.config->getString(common.config->homeLON).toDouble();
    }

    void setupKeys() {
        commonData->keydata[0].label = "EXIT";
        commonData->keydata[1].label = "MODE";
        commonData->keydata[2].label = "";
        commonData->keydata[3].label = "RST";
        commonData->keydata[4].label = "STBY";
        commonData->keydata[5].label = "ILUM";
    }

    int handleKey(int key) {
        // do *NOT* handle key #1 this handled by obp60task as exit
        // Switch display mode
        commonData->logger->logDebug(GwLog::LOG, "System keyboard handler");
        if (key == 2) {
            if (mode == 'N') {
                mode = 'S';
            } else if (mode == 'S') {
                mode = 'D';
            } else if (mode == 'D') {
                if (hasSDCard) {
                    mode = 'C';
                } else {
                    mode = 'N';
                }
            } else {
                mode = 'N';
            }
            if (hasFRAM) fram.write(FRAM_SYSTEM_MODE, mode);
            return 0;
        }
#ifdef BOARD_OBP60S3
        // grab cursor key to disable page navigation
        if (key == 3) {
            return 0;
        }
        // soft reset
        if (key == 4) {
            ESP.restart();
        }
        // standby / deep sleep
        if (key == 5) {
            commonData->logger->logDebug(GwLog::LOG, "System going into deep sleep mode...");
            deepSleep(*commonData);
        }
        // Code for keylock
        if (key == 11) {
            commonData->keylock = !commonData->keylock;
            return 0;
        }
#endif
#ifdef BOARD_OBP40S3
        // grab cursor keys to disable page navigation
        if (key == 9 or key == 10) {
            return 0;
        }
        // standby / deep sleep
        if (key == 12) {
            commonData->logger->logDebug(GwLog::LOG, "System going into deep sleep mode...");
            deepSleep(*commonData);
        }
#endif
        return key;
    }

    void displayBarcode(String serialno, uint16_t x, uint16_t y, uint16_t s) {
        // Barcode with serial number
        // x, y is top left corner
        // s is pixel size of a single box
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(4)];
#ifdef BOARD_OBP40S3
        String prefix = "OBP40:SN:";
#endif
#ifdef BOARD_OBP60S3
        String prefix = "OBP60:SN:";
#endif
        qrcode_initText(&qrcode, qrcodeData, 4, 0, (prefix + serialno).c_str());
        int16_t x0 = x;
        for (uint8_t j = 0; j < qrcode.size; j++) {
            for (uint8_t i = 0; i < qrcode.size; i++) {
                if (qrcode_getModule(&qrcode, i, j)) {
                    getdisplay().fillRect(x, y, s, s, commonData->fgcolor);
                }
                x += s;
            }
            y += s;
            x = x0;
        }
    }

    int displayPage(PageData &pageData){
        GwConfigHandler *config = commonData->config;
        GwLog *logger = commonData->logger;

        // Get config data
        String flashLED = config->getString(config->flashLED);

        // Optical warning by limit violation (unused)
        if(String(flashLED) == "Limit Violation"){
            setBlinkingLED(false);
            setFlashLED(false); 
        }

        // Logging boat values
        logger->logDebug(GwLog::LOG, "Drawing at PageSystem, Mode=%c", mode);

        // Draw page
        //***********************************************************

        uint16_t x0 = 8;  // left column
        uint16_t y0 = 48; // data table starts here

        // Set display in partial refresh mode
        getdisplay().setPartialWindow(0, 0, getdisplay().width(), getdisplay().height()); // Set partial update

        if (mode == 'N') {

            getdisplay().setFont(&Ubuntu_Bold12pt8b);
            getdisplay().setCursor(8, 48);
            getdisplay().print("System Information");

            getdisplay().drawXBitmap(320, 25, logo64_bits, logo64_width, logo64_height, commonData->fgcolor);

            getdisplay().setFont(&Ubuntu_Bold8pt8b);
            y0 = 155;

            char ssid[13];
            snprintf(ssid, 13, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
            displayBarcode(String(ssid), 320, 200, 2);
            getdisplay().setCursor(8, 70);
            getdisplay().print(String("MCUDEVICE-") + String(ssid));

            getdisplay().setCursor(8, 95);
            getdisplay().print("Firmware version: ");
            getdisplay().setCursor(150, 95);
            getdisplay().print(VERSINFO);

            getdisplay().setCursor(8, 113);
            getdisplay().print("Board version: ");
            getdisplay().setCursor(150, 113);
            getdisplay().print(BOARDINFO);
            getdisplay().print(String(" HW ") + String(PCBINFO));

            getdisplay().setCursor(8, 131);
            getdisplay().print("Display version: ");
            getdisplay().setCursor(150, 131);
            getdisplay().print(DISPLAYINFO);
            getdisplay().print("; GxEPD2 v");
            getdisplay().print(GXEPD2INFO);

            getdisplay().setCursor(8, 265);
#ifdef BOARD_OBP60S3
            getdisplay().print("Press STBY to enter deep sleep mode");
#endif
#ifdef BOARD_OBP40S3
            getdisplay().print("Press wheel to enter deep sleep mode");
#endif

            // Flash memory size
            uint32_t flash_size = ESP.getFlashChipSize();
            getdisplay().setCursor(8, y0);
            getdisplay().print("FLASH:");
            getdisplay().setCursor(90, y0);
            getdisplay().print(String(flash_size / 1024) + String(" kB"));

            // PSRAM memory size
            uint32_t psram_size = ESP.getPsramSize();
            getdisplay().setCursor(8, y0 + 16);
            getdisplay().print("PSRAM:");
            getdisplay().setCursor(90, y0 + 16);
            getdisplay().print(String(psram_size / 1024) + String(" kB"));

            // FRAM available / status
            getdisplay().setCursor(8, y0 + 32);
            getdisplay().print("FRAM:");
            getdisplay().setCursor(90, y0 + 32);
            getdisplay().print(hasFRAM ? "available" : "not found");

#ifdef BOARD_OBP40S3
            // SD-Card
            getdisplay().setCursor(8, y0 + 48);
            getdisplay().print("SD-Card:");
            getdisplay().setCursor(90, y0 + 48);
            if (hasSDCard) {
                uint64_t cardsize = ((uint64_t) sdcard->csd.capacity) * sdcard->csd.sector_size / (1024 * 1024);
                getdisplay().printf("%llu MB", cardsize);
            } else {
                getdisplay().print("off");
            }
#endif

            // Uptime
            int64_t uptime = esp_timer_get_time() / 1000000;
            String uptime_unit;
            if (uptime < 120) {
                uptime_unit = " seconds";
            } else {
                if (uptime < 2 * 3600) {
                    uptime /= 60;
                    uptime_unit = " minutes";
                } else if (uptime < 2 * 3600 * 24) {
                    uptime /= 3600;
                    uptime_unit = " hours";
                } else {
                    uptime /= 86400;
                    uptime_unit = " days";
                }
            }
            getdisplay().setCursor(8, y0 + 80);
            getdisplay().print("Uptime:");
            getdisplay().setCursor(90, y0 + 80);
            getdisplay().print(uptime);
            getdisplay().print(uptime_unit);

            // CPU speed config / active
            getdisplay().setCursor(202, y0);
            getdisplay().print("CPU speed:");
            getdisplay().setCursor(300, y0);
            getdisplay().print(cpuspeed);
            getdisplay().print(" / ");
            int cpu_freq = esp_clk_cpu_freq() / 1000000;
            getdisplay().print(String(cpu_freq));

            // total RAM free
            int Heap_free = esp_get_free_heap_size();
            getdisplay().setCursor(202, y0 + 16);
            getdisplay().print("Total free:");
            getdisplay().setCursor(300, y0 + 16);
            getdisplay().print(String(Heap_free));

            // RAM free for task
            int RAM_free = uxTaskGetStackHighWaterMark(NULL);
            getdisplay().setCursor(202, y0 + 32);
            getdisplay().print("Task free:");
            getdisplay().setCursor(300, y0 + 32);
            getdisplay().print(String(RAM_free));

        } else if (mode == 'S') {
            // Settings
            
            getdisplay().setFont(&Ubuntu_Bold12pt8b);
            getdisplay().setCursor(x0, 48);
            getdisplay().print("System settings");

            getdisplay().setFont(&Ubuntu_Bold8pt8b);
            x0 = 8;
            y0 = 72;

            // left column
            getdisplay().setCursor(x0, y0);
            getdisplay().print("Simulation:");
            getdisplay().setCursor(120, y0);
            getdisplay().print(simulation ? "on" : "off");

            getdisplay().setCursor(x0, y0 + 16);
            getdisplay().print("Environment:");
            getdisplay().setCursor(120, y0 + 16);
            getdisplay().print(env_module);

            getdisplay().setCursor(x0, y0 + 32);
            getdisplay().print("Buzzer:");
            getdisplay().setCursor(120, y0 + 32);
            getdisplay().print(buzzer_mode);

            getdisplay().setCursor(x0, y0 + 64);
            getdisplay().print("GPS:");
            getdisplay().setCursor(120, y0 + 64);
            getdisplay().print(gps_module);

            getdisplay().setCursor(x0, y0 + 80);
            getdisplay().print("RTC:");
            getdisplay().setCursor(120, y0 + 80);
            getdisplay().print(rtc_module);

            getdisplay().setCursor(x0, y0 + 96);
            getdisplay().print("Wifi:");
            getdisplay().setCursor(120, y0 + 96);
            getdisplay().print(commonData->status.wifiApOn ? "on" : "off");

            // Home location
            getdisplay().setCursor(x0, y0 + 128);
            getdisplay().print("Home Lat.:");
            getdisplay().setCursor(120, y0 + 128);
            getdisplay().print(formatLatitude(homelat));
            getdisplay().setCursor(x0, y0 + 144);
            getdisplay().print("Home Lon.:");
            getdisplay().setCursor(120, y0 + 144);
            getdisplay().print(formatLongitude(homelon));

            // right column
            getdisplay().setCursor(202, y0);
            getdisplay().print("Batt. sensor:");
            getdisplay().setCursor(320, y0);
            getdisplay().print(batt_sensor);

            // Solar sensor
            getdisplay().setCursor(202, y0 + 16);
            getdisplay().print("Solar sensor:");
            getdisplay().setCursor(320, y0 + 16);
            getdisplay().print(solar_sensor);

            // Generator sensor
            getdisplay().setCursor(202, y0 + 32);
            getdisplay().print("Gen. sensor:");
            getdisplay().setCursor(320, y0 + 32);
            getdisplay().print(gen_sensor);

            // Gyro sensor

        } else if (mode == 'C') {
            // Card info
            getdisplay().setFont(&Ubuntu_Bold12pt8b);
            getdisplay().setCursor(8, 48);
            getdisplay().print("SD Card info");

            getdisplay().setFont(&Ubuntu_Bold8pt8b);

            x0 = 20;
            y0 = 72;
            getdisplay().setCursor(x0, y0);
#ifdef BOARD_OBP60S3
            // This mode should not be callable by devices without card hardware
            // In case of accidential reaching this, display a friendly message
            getdisplay().print("This mode is not indended to be reached!\n");
            getdisplay().print("There's nothing to see here. Move on.");
#endif
#ifdef BOARD_OBP40S3
            getdisplay().print("Work in progress...");

            /* TODO
               this code should go somewhere else. only for testing purposes here
               identify card as OBP-Card:
                 magic.dat
                 version.dat
                 readme.txt
                 IMAGES/
                 CHARTS/
                 LOGS/
                 DATA/
                hint: file access with fopen, fgets, fread, fclose
            */

            // Simple test for magic file in root
            getdisplay().setCursor(x0, y0 + 32);
            String file_magic = MOUNT_POINT "/magic.dat";
            logger->logDebug(GwLog::LOG, "Test magicfile: %s", file_magic.c_str());
            struct stat st;
            if (stat(file_magic.c_str(), &st) == 0) {
                getdisplay().printf("File %s exists", file_magic.c_str());
            } else {
                getdisplay().printf("File %s not found", file_magic.c_str());
            }

            // Root directory check
            DIR* dir = opendir(MOUNT_POINT);
            int dy = 0;
            if (dir != NULL) {
                logger->logDebug(GwLog::LOG, "Root directory: %s", MOUNT_POINT);
                struct dirent* entry;
                while (((entry = readdir(dir)) != NULL) and (dy < 140)) {
                    getdisplay().setCursor(x0, y0 + 64 + dy);
                    getdisplay().print(entry->d_name);
                    // type 1 is file, type 2 is dir
                    if (entry->d_type == 2) {
                        getdisplay().print("/");
                    }
                    dy += 20;
                    logger->logDebug(GwLog::DEBUG, "    %s type %d", entry->d_name, entry->d_type);
                }
                closedir(dir);
            } else {
                logger->logDebug(GwLog::LOG, "Failed to open root directory");
            }

#endif

        } else {
            // NMEA2000 device list
            getdisplay().setFont(&Ubuntu_Bold12pt8b);
            getdisplay().setCursor(8, 48);
            getdisplay().print("NMEA2000 device list");

            getdisplay().setFont(&Ubuntu_Bold8pt8b);
            getdisplay().setCursor(20, 80);
            getdisplay().print("RxD: ");
            getdisplay().print(String(commonData->status.n2kRx));
            getdisplay().setCursor(20, 100);
            getdisplay().print("TxD: ");
            getdisplay().print(String(commonData->status.n2kTx));
        }

        // Update display
        getdisplay().nextPage();    // Partial update (fast)
        return PAGE_OK;
    };
};

static Page* createPage(CommonData &common){
    return new PageSystem(common);
}

/**
 * with the code below we make this page known to the PageTask
 * we give it a type (name) that can be selected in the config
 * we define which function is to be called
 * and we provide the number of user parameters we expect
 * this will be number of BoatValue pointers in pageData.values
 */
PageDescription registerPageSystem(
    "System",   // Page name
    createPage, // Action
    0,          // No bus values
    true        // Headers are anabled so far
);

#endif
