#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include "Graphics.h"  // for Point and Rect

class ConfigMenuItem {
private:
    String label;
    uint16_t value;
    String unit;
    String valtype; // "int" | "bool"
    uint16_t min;
    uint16_t max;
    std::vector<uint16_t> steps;
    uint16_t step;
    int8_t position; // counted fom 0

public:
    ConfigMenuItem(String itemtype, String itemlabel);
    void setRange(uint16_t valmin, uint16_t valmax, std::vector<uint16_t> steps);
    bool setValue(uint16_t newval);
    void setPos(int8_t newpos);
    int8_t getPos();
};

class ConfigMenu {
private:
    String title;
    std::map <String,ConfigMenuItem> items;
    std::map <uint8_t,String> index;
    int8_t activeitem = -1; // refers to position of item
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;

public:
    ConfigMenu(String title, uint16_t menu_x, uint16_t menu_y);
    ConfigMenuItem* addItem(String key, String label, String valtype);
    void setItemDimension(uint16_t itemwidth, uint16_t itemheight);
    void setItemActive(String key);
    ConfigMenuItem* getActiveItem();
    ConfigMenuItem* getItemByIndex(uint8_t index);
    ConfigMenuItem* getItemByKey(String key);
    uint8_t getItemCount();
    Point getXY();
    /* void getRect();
    void getItemRect(); */
};
