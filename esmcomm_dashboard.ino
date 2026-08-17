#include <NimBLEDevice.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// ==========================================
//      หน้าจอ & LVGL CONFIGURATIONS
// ==========================================
// บอร์ด ESP32-2432S028 ใน LVGL v9 ต้องระบุขนาดจริงแนวตั้งเพื่อไม่ให้แรมบัฟเฟอร์ภาพแตก (จอล้ม)
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// จองพื้นที่หน่วยความจำสำหรับเป็น Buffer ให้หน้าจอ LVGL วาดภาพ
uint32_t draw_buf[SCREEN_WIDTH * 10]; 

// นิยามโครงสร้างข้อมูลความกว้างพิกเซลจอสำหรับแก้ตระกูลคอมไพเลอร์มองไม่เห็น
typedef struct {
    TFT_eSPI * tft;
} lv_tft_espi_t;

// ------------------------------------------
//  ตัวแปรส่วนกลาง (Pointer) วัตถุกราฟิกบนหน้าจอ
// ------------------------------------------
lv_obj_t *arc_soc;
lv_obj_t *label_soc_text;
lv_obj_t *label_voltage;
lv_obj_t *label_current;
lv_obj_t *label_capacity;
lv_obj_t *label_chg_limit;
lv_obj_t *label_dchg_limit;
lv_obj_t *sw_charge;
lv_obj_t *sw_discharge;
lv_obj_t *sw_autodim;
lv_obj_t *label_cells_footer;
lv_obj_t *label_temp_footer;

// ==========================================
//           BMS CONFIGURATIONS
// ==========================================
static BLEUUID serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    charUUID("0000fff2-0000-1000-8000-00805f9b34fb");

#define BMS_MAC_ADDRESS "ff:ff:11:e7:cd:b3" 
#define BMS_DEVICE_NAME "EC-Lifepo4 4S"

// โครงสร้างข้อมูลเก็บค่าตัวแปรแบตเตอรี่จากระบบบลูทูธ
struct ECBMS_Data {
    float totalVoltage = 0.0;
    float cellDelta = 0.0;
    float maxCellVoltage = 0.0;
    float minCellVoltage = 0.0;
    int maxCellIndex = 0;
    int minCellIndex = 0;
    
    float cell1 = 0.0;
    float cell2 = 0.0;
    float cell3 = 0.0;
    float cell4 = 0.0;
    
    float currentAmps = 0.0;
    int stateOfCharge = 0;
    float mosfetTemp = 0.0;
    float boardTemp = 0.0;
};

ECBMS_Data battery;
String lineAccumulator = ""; // กล่องพักข้อความดิบชั่วคราวเพื่อต่อสายตัวอักษร

// ตัวแปรสถานะเครือข่ายบลูทูธส่วนกลาง
static const NimBLEAdvertisedDevice* bmsDevice = nullptr; 
static NimBLEClient*                 pClient   = nullptr;
static bool doConnect = false;

// แจ้งเตือนคอมไพเลอร์ให้รู้จักฟังก์ชันเรียกกลับของบลูทูธล่วงหน้า
void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

// ==========================================
//          TELEMETRY PARSING SYSTEM
// ==========================================
void processFinishedLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    int colonIndex = line.indexOf(':');
    if (colonIndex != -1) {
        String key = line.substring(0, colonIndex);
        String valStr = line.substring(colonIndex + 1);
        float value = valStr.toFloat();

        // แกะรหัสข้อมูลรายบรรทัดเก็บเข้าคลังตัวแปร
        if      (key == "1")     { battery.cell1 = value; }
        else if (key == "2")     { battery.cell2 = value; }
        else if (key == "3")     { battery.cell3 = value; }
        else if (key == "4")     { battery.cell4 = value; }
        else if (key == "zdy")   { battery.totalVoltage = value; }
        else if (key == "yc")    { battery.cellDelta = value; }
        else if (key == "max")   { battery.maxCellVoltage = value; }
        else if (key == "min")   { battery.minCellVoltage = value; }
        else if (key == "zg")    { battery.maxCellIndex = (int)value; }
        else if (key == "zd")    { battery.minCellIndex = (int)value; }
        else if (key == "bl")    { battery.stateOfCharge = (int)value; }
        else if (key == "dl")    { battery.currentAmps = value; }
        else if (key == "moswd") { battery.mosfetTemp = value; }
        else if (key == "jhwd")  { battery.boardTemp = value; }
    } 
    // เมื่อข้อความส่งมาสิ้นสุดชุดรอบ "jhstop" -> ทำการยิงอัปเดตข้อมูลขึ้นหน้าจอ UI ทันที
    else if (line == "jhstop") {
        Serial.println("\n[BMS] --- อัปเดตข้อมูลแพ็กเก็ตใหม่สำเร็จ ---");
        
        Serial.printf("แรงดันรวม: %.2f V | SoC: %d %%\n", battery.totalVoltage, battery.stateOfCharge);
        Serial.printf("กระแส: %.2f A | Delta เซลล์: %.4f V\n", battery.currentAmps, battery.cellDelta);

        // 1. อัปเดตค่าเกจวงกลม และตัวเลขเปอร์เซ็นต์กลางเกจ (ส่งค่าแบบปลอดภัย ไร้เงา redeclaration)
        lv_arc_set_value(arc_soc, battery.stateOfCharge);
        lv_label_set_text(label_soc_text, (String(battery.stateOfCharge) + "%").c_str()); 

        // 2. อัปเดตตัวเลขแรงดันไฟฟ้าโวลต์รวม (BUS VOLTAGE)
        lv_label_set_text(label_voltage, (String(battery.totalVoltage, 2) + "V").c_str());

        // 3. อัปเดตตัวเลขกระแสไฟฟ้าแอมป์ (BATTERY CURRENT)
        lv_label_set_text(label_current, (String(battery.currentAmps, 1) + " A").c_str());

        // 4. อัปเดตรายละเอียดแรงดันเซลล์ C1 - C4 มุมล่างขวา
        lv_label_set_text(label_cells_footer, 
            ("C1: " + String(battery.cell1, 3) + "V  C2: " + String(battery.cell2, 3) + "V\n" +
             "C3: " + String(battery.cell3, 3) + "V  C4: " + String(battery.cell4, 3) + "V").c_str()
        );

        // 5. อัปเดตค่าความร้อนอุณหภูมิระบบมุมล่างซ้าย
        lv_label_set_text(label_temp_footer, 
            ("MOS: " + String(battery.mosfetTemp, 1) + "°C\n" +
             "BAL: " + String(battery.boardTemp, 1) + "°C").c_str()
        );
    }
}

// ฟังก์ชันดักสายสัญญาณบิตข้อมูลสตรีมมิ่งผ่านท่อบลูทูธ
void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    for (size_t i = 0; i < length; i++) {
        char c = (char)pData[i];
        if (c == '\n') {
            processFinishedLine(lineAccumulator);
            lineAccumulator = ""; 
        } else {
            lineAccumulator += c; 
        }
    }
}

// ==========================================
//          CORE BLE STACK CALLBACKS
// ==========================================
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        Serial.println("[BLE] เชื่อมต่อกับกล่องแบตเตอรี่สำเร็จ!");
    }
    void onDisconnect(NimBLEClient* pClient, int reason) override {
        Serial.printf("[BLE] ช่องสัญญาณขาดหาย (รหัส: %d) บังคับเปิดระบบสแกนใหม่อัตโนมัติ...\n", reason);
        doConnect = false;
        bmsDevice = nullptr;
        NimBLEDevice::getScan()->start(0); 
    }
};

class AdvertisedDeviceCallbacks: public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        String currentAddress = advertisedDevice->getAddress().toString().c_str();
        String currentName    = advertisedDevice->getName().c_str();

        if (currentAddress == BMS_MAC_ADDRESS || currentName == BMS_DEVICE_NAME) {
            Serial.print("[BLE] ค้นพบสัญญาณเป้าหมายตรงล็อกตามรหัส! Address: ");
            Serial.println(currentAddress);
            
            bmsDevice = advertisedDevice;    
            doConnect = true;                // ปลดล็อกเงื่อนไขในลูปหลักเพื่อให้เริ่มต่อบลูทูธ
            NimBLEDevice::getScan()->stop(); // หยุดสถานะสแกนเพื่อเคลียร์กำลังวัตต์บอร์ด
        }
    }
};

// ==========================================
//        CONNECTION ORCHESTRATION PIPELINE
// ==========================================
bool connectToBMS() {
    if (bmsDevice == nullptr) return false;

    if (pClient == nullptr) {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks()); 
    } else {
        pClient->disconnect(); // เคลียร์ท่อข้อมูลที่ติดขัดก่อนเชื่อมใหม่ป้องกันแรมซ้อนทับจนบอร์ดค้าง
    }

    Serial.println("[BLE] Opening connection gateway channel...");
    if (!pClient->connect(bmsDevice)) {
        Serial.println("[BLE] Handshake failed or server busy. Aborting connection.");
        return false;
    }

    NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("[BLE] Service UUID validation error. Disconnecting device.");
        pClient->disconnect();
        return false;
    }

    NimBLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("[BLE] Telemetry characteristic handle missing. Disconnecting device.");
        pClient->disconnect();
        return false;
    }

    if (pRemoteCharacteristic->canNotify()) {
        if (!pRemoteCharacteristic->subscribe(true, notifyCallback)) {
            Serial.println("[BLE] Fault encountered setting up subscription handle.");
            pClient->disconnect();
            return false;
        }
        Serial.println("[BLE] Telemetry registration verified. Awaiting incoming data packages...");
    } else {
        Serial.println("[BLE] Selected characteristic handle does not support active notification flags.");
        pClient->disconnect();
        return false;
    }
    return true;
}

// ==========================================
//          LVGL UI CREATION LAYOUT
// ==========================================
void create_esmcomm_dashboard() {
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_make(10, 17, 26), LV_PART_MAIN);

    // ---- Header Section ----
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "ESMCOMM 1.0");
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 10, 5);

    lv_obj_t * label_ip = lv_label_create(scr);
    lv_label_set_text(label_ip, "IP 192.168.1.42");
    lv_obj_set_style_text_color(label_ip, lv_color_make(46, 204, 113), LV_PART_MAIN); 
    lv_obj_set_style_text_font(label_ip, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(label_ip, LV_ALIGN_TOP_RIGHT, -10, 5);

    // ---- Main Panel Container ----
    lv_obj_t * main_panel = lv_obj_create(scr);
    lv_obj_set_size(main_panel, 310, 185); 
    lv_obj_align(main_panel, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_set_style_bg_color(main_panel, lv_color_make(18, 30, 49), LV_PART_MAIN);
    lv_obj_set_style_border_color(main_panel, lv_color_make(41, 128, 185), LV_PART_MAIN);
    lv_obj_set_style_border_width(main_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(main_panel, 8, LV_PART_MAIN);
    lv_obj_clear_flag(main_panel, lv_obj_flag_t(LV_OBJ_FLAG_SCROLLABLE));

    lv_obj_t * label_panel_header = lv_label_create(main_panel);
    lv_label_set_text(label_panel_header, "SYSTEM OVERVIEW");
    lv_obj_set_style_text_color(label_panel_header, lv_color_make(127, 140, 141), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_panel_header, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(label_panel_header, LV_ALIGN_TOP_LEFT, -5, -5);

    // ---- SoC Arc Gauge ----
    arc_soc = lv_arc_create(main_panel);
    lv_obj_set_size(arc_soc, 70, 70);
    lv_obj_align(arc_soc, LV_ALIGN_TOP_LEFT, -5, 12);
    lv_arc_set_rotation(arc_soc, 135);
    lv_arc_set_bg_angles(arc_soc, 0, 270);
    lv_arc_set_value(arc_soc, 0); 
    lv_obj_set_style_arc_color(arc_soc, lv_color_make(46, 204, 113), LV_PART_INDICATOR); 
    lv_obj_set_style_arc_color(arc_soc, lv_color_make(44, 62, 80), LV_PART_MAIN);       
    lv_obj_remove_style(arc_soc, NULL, LV_PART_KNOB); 

    // ผูกวัตถุ label_soc_text ซ้อนเอาไว้บน arc_soc ตรงตามโครงสร้างที่ถูกต้อง
    label_soc_text = lv_label_create(arc_soc);
    lv_label_set_text(label_soc_text, "0%");
    lv_obj_set_style_text_color(label_soc_text, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_soc_text, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align_to(label_soc_text, arc_soc, LV_ALIGN_CENTER, 0, 0);

    // ---- Bus Voltage Box ----
    lv_obj_t * box_volt = lv_obj_create(main_panel);
    lv_obj_set_size(box_volt, 95, 40);
    lv_obj_align(box_volt, LV_ALIGN_TOP_LEFT, 75, 10);
    lv_obj_set_style_bg_color(box_volt, lv_color_make(24, 44, 70), LV_PART_MAIN);
    lv_obj_set_style_border_color(box_volt, lv_color_make(52, 73, 94), LV_PART_MAIN);
    lv_obj_set_style_border_width(box_volt, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box_volt, 0, LV_PART_MAIN);
    lv_obj_clear_flag(box_volt, lv_obj_flag_t(LV_OBJ_FLAG_SCROLLABLE));

    lv_obj_t * lbl_v_title = lv_label_create(box_volt);
    lv_label_set_text(lbl_v_title, "BUS VOLTAGE");
    lv_obj_set_style_text_color(lbl_v_title, lv_color_make(149, 165, 166), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_v_title, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_v_title, LV_ALIGN_TOP_LEFT, 4, 2);

    label_voltage = lv_label_create(box_volt);
    lv_label_set_text(label_voltage, "0.00V");
    lv_obj_set_style_text_color(label_voltage, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_voltage, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_voltage, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // ---- Battery Current Box ----
    lv_obj_t * box_curr = lv_obj_create(main_panel);
    lv_obj_set_size(box_curr, 115, 40);
    lv_obj_align(box_curr, LV_ALIGN_TOP_RIGHT, 5, 10);
    lv_obj_set_style_bg_color(box_curr, lv_color_make(24, 44, 70), LV_PART_MAIN);
    lv_obj_set_style_border_color(box_curr, lv_color_make(52, 73, 94), LV_PART_MAIN);
    lv_obj_set_style_border_width(box_curr, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box_curr, 0, LV_PART_MAIN);
    lv_obj_clear_flag(box_curr, lv_obj_flag_t(LV_OBJ_FLAG_SCROLLABLE));

    lv_obj_t * lbl_c_title = lv_label_create(box_curr);
    lv_label_set_text(lbl_c_title, "BATTERY CURRENT");
    lv_obj_set_style_text_color(lbl_c_title, lv_color_make(149, 165, 166), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_c_title, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_c_title, LV_ALIGN_TOP_LEFT, 4, 2);

    label_current = lv_label_create(box_curr);
    lv_label_set_text(label_current, "0.0 A");
    lv_obj_set_style_text_color(label_current, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_current, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_current, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // ---- Middle Grid Layout ----
    lv_obj_t * lbl_cap_h = lv_label_create(main_panel);
    lv_label_set_text(lbl_cap_h, "CAPACITY");
    lv_obj_set_style_text_color(lbl_cap_h, lv_color_make(155, 89, 182), LV_PART_MAIN); 
    lv_obj_set_style_text_font(lbl_cap_h, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_cap_h, LV_ALIGN_TOP_LEFT, 75, 56);

    lv_obj_t * lbl_chg_h = lv_label_create(main_panel);
    lv_label_set_text(lbl_chg_h, "CHG");
    lv_obj_set_style_text_color(lbl_chg_h, lv_color_make(46, 204, 113), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_chg_h, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(lbl_chg_h, LV_ALIGN_TOP_LEFT, 165, 56);

    lv_obj_t * lbl_dchg_h = lv_label_create(main_panel);
    lv_label_set_text(lbl_dchg_h, "DCHG");
    lv_obj_set_style_text_color(lbl_dchg_h, lv_color_make(230, 126, 34), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_dchg_h, &lv_font_montserrat_12, LV_PART_MAIN); 
    lv_obj_align(lbl_dchg_h, LV_ALIGN_TOP_RIGHT, -15, 56);

    label_capacity = lv_label_create(main_panel);
    lv_label_set_text(label_capacity, "1248 Ah");
    lv_obj_set_style_text_color(label_capacity, lv_color_make(189, 195, 199), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_capacity, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(label_capacity, LV_ALIGN_TOP_LEFT, 75, 72);

    label_chg_limit = lv_label_create(main_panel);
    lv_label_set_text(label_chg_limit, "100 A");
    lv_obj_set_style_text_color(label_chg_limit, lv_color_make(46, 204, 113), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_chg_limit, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(label_chg_limit, LV_ALIGN_TOP_LEFT, 165, 72);

    label_dchg_limit = lv_label_create(main_panel);
    lv_label_set_text(label_dchg_limit, "120 A");
    lv_obj_set_style_text_color(label_dchg_limit, lv_color_make(230, 126, 34), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_dchg_limit, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(label_dchg_limit, LV_ALIGN_TOP_RIGHT, -15, 72);

    // ---- Control Switches Bar ----
    lv_obj_t * lbl_sw1 = lv_label_create(main_panel);
    lv_label_set_text(lbl_sw1, "CHARGE");
    lv_obj_set_style_text_color(lbl_sw1, lv_color_make(52, 152, 219), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_sw1, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_sw1, LV_ALIGN_TOP_LEFT, -5, 96);

    sw_charge = lv_switch_create(main_panel);
    lv_obj_set_size(sw_charge, 32, 16);
    lv_obj_align(sw_charge, LV_ALIGN_TOP_LEFT, -5, 110);
    lv_obj_add_state(sw_charge, LV_STATE_CHECKED);

    lv_obj_t * lbl_sw2 = lv_label_create(main_panel);
    lv_label_set_text(lbl_sw2, "DISCHARGE");
    lv_obj_set_style_text_color(lbl_sw2, lv_color_make(230, 126, 34), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_sw2, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_sw2, LV_ALIGN_TOP_LEFT, 42, 96);

    sw_discharge = lv_switch_create(main_panel);
    lv_obj_set_size(sw_discharge, 32, 16);
    lv_obj_align(sw_discharge, LV_ALIGN_TOP_LEFT, 45, 110);
    lv_obj_add_state(sw_discharge, LV_STATE_CHECKED);

    lv_obj_t * lbl_sw3 = lv_label_create(main_panel);
    lv_label_set_text(lbl_sw3, "AUTO DIM");
    lv_obj_set_style_text_color(lbl_sw3, lv_color_make(149, 165, 166), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_sw3, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_sw3, LV_ALIGN_TOP_LEFT, 115, 96);

    sw_autodim = lv_switch_create(main_panel);
    lv_obj_set_size(sw_autodim, 32, 16);
    lv_obj_align(sw_autodim, LV_ALIGN_TOP_LEFT, 115, 110);

    // ---- แสดงผลแรงดันรายเซลล์ C1 - C4

    label_cells_footer = lv_label_create(main_panel);
    lv_label_set_text(label_cells_footer, "C1: 0.000V  C2: 0.000V\nC3: 0.000V  C4: 0.000V");
    lv_obj_set_style_text_color(label_cells_footer, lv_color_make(149, 165, 166), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_cells_footer, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(label_cells_footer, LV_ALIGN_BOTTOM_RIGHT, 5, -15);

// ---- แสดงผลค่าระดับความร้อนอุณหภูมิระบบ ----

    label_temp_footer = lv_label_create(main_panel);
    lv_label_set_text(label_temp_footer, "MOS: 0.0°C\nBAL: 0.0°C");
    lv_obj_set_style_text_color(label_temp_footer, lv_color_make(189, 195, 199), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_temp_footer, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(label_temp_footer, LV_ALIGN_BOTTOM_LEFT, -5, -5);

}

    // ==========================================
//             STANDARD ARDUINO LOOPS
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("[System] Booting ESP32-2432S028 Core Engine...");

    // บังคับเปิดไฟหลังจอ (Backlight) ของบอร์ด CYD ให้ทำงานสว่างสูงสุด
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
    TFT_eSPI tft = TFT_eSPI();
    // 1. เริ่มต้นหน้าจอสัมผัสระบบฮาร์ดแวร์ TFT_eSPI
    tft.begin();
    tft.setRotation(1); // บังคับแนวนอนสำหรับจอแสดงผลของบอร์ด CYD
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    // 3. วาดดีไซน์ Layout หน้าจอแดชบอร์ดต้นฉบับ
    create_esmcomm_dashboard();

    Serial.println("[System] เปิดสัญญาณเสาบลูทูธ ESP32...");
    NimBLEDevice::init("ESP32_BMS_Gateway");
    NimBLEDevice::setPower(9); 

    NimBLEScan* pNimBLEScan = NimBLEDevice::getScan();
    pNimBLEScan->setScanCallbacks(new AdvertisedDeviceCallbacks(), false);
    pNimBLEScan->setInterval(100);
    pNimBLEScan->setWindow(100);
    pNimBLEScan->setActiveScan(true); 
    pNimBLEScan->start(0); 
    
    Serial.println("[System] Setup Finished. UI and BLE Threads Operational.");
}

void loop() {
// ตรวจสอบและประมวลผลคำสั่งขอกลไกเชื่อมต่อ
    if (doConnect) {
        doConnect = false;
        if (connectToBMS()) {
            Serial.println("[System] การตั้งท่อ Handshake บลูทูธ BMS เสร็จสมบูรณ์.");
        }
    }

// 🌟 ส่วนสำคัญลดอาการกระพริบ: เปลี่ยนระยะเวลาเพิ่มจังหวะหน่วงอย่างเหมาะสม 🌟
lv_task_handler();
lv_tick_inc(25);   // เพิ่มเวลาหัวใจหลักตามรอบมิลลิวินาทีจริง
delay(25);         // หน่วงรอบที่ 25ms เพื่อให้จอสมูท นิ่งสนิท และลดการทำงานบอร์ดหนักเกินไป
}
 
