/*
  Menu system for online configuration
*/
#include "ConfigMenu.h"

ConfigMenuItem::ConfigMenuItem(String itemtype, String itemlabel) {
    if (! (itemtype == "int" or itemtype == "bool")) {
        valtype = "int";
    } else {
        valtype = itemtype;
    }
    label = itemlabel;
    min = 0;
    max = std::numeric_limits<uint16_t>::max();
}

void ConfigMenuItem::setRange(uint16_t valmin, uint16_t valmax, std::vector<uint16_t> valsteps) {
    min = valmin;
    max = valmax;
    steps = valsteps;
};

bool ConfigMenuItem::setValue(uint16_t newval) {
    if (valtype == "int") {
        if (newval >= min and newval <= max) {
            value = newval;
            return true;
        }
        return false; // out of range
    } else if (valtype == "bool") {
        value = (newval != 0) ? 1 : 0;
        return true;
    }
    return false; // invalid type
};

void ConfigMenuItem::setPos(int8_t newpos) {
    position = newpos;
};

int8_t ConfigMenuItem::getPos() {
    return position;
};

ConfigMenu::ConfigMenu(String menutitle, uint16_t menu_x, uint16_t menu_y) {
    title = menutitle;
    x = menu_x;
    y = menu_y;
};

ConfigMenuItem* ConfigMenu::addItem(String key, String label, String valtype) {
    if (items.find(key) != items.end()) {
        // duplicate keys not allowed
        return nullptr;
    }
    ConfigMenuItem itm(valtype, "Test1");
    return &itm;
    // map.insert(std::pair<String, ConfigMenuItem>(itm));
    // Append key to index
    int8_t ix = items.size();
    index[ix] = key;
    itm.setPos(ix);
};

void ConfigMenu::setItemDimension(uint16_t itemwidth, uint16_t itemheight) {
    w = itemwidth;
    h = itemheight;
};

void ConfigMenu::setItemActive(String key) {
    uint8_t ix = 
    activeitem = ix;
};

ConfigMenuItem* ConfigMenu::getActiveItem() {
    return nullptr;
};

ConfigMenuItem* ConfigMenu::getItemByIndex(uint8_t index) {
    return nullptr;
};

ConfigMenuItem* ConfigMenu::getItemByKey(String Key) {
    return nullptr;
};

uint8_t ConfigMenu::getItemCount() {
    return items.size();
};

Point ConfigMenu::getXY() {
    return {x, y};
}

/*
void getRect();
void getItemRect();
*/



