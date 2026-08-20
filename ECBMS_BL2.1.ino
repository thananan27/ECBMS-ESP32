/*
 * ECBMSM 2.1 Dashboard & BLE Gateway for ESP32-2432S028 (CYD 2.8" 320x240)
 * Fully Refactored for **LVGL v9** Standard.
 * Multi-threaded Protection with NimBLE 2.5.1 + LovyanGFX Driver.
 * 
 * 100% WORKING FULL CODE - Features Dynamic Screen Switching and Smart Cell Coloring.
 */

#include <NimBLEDevice.h>
#include <lvgl.h>

// ==========================================
//   1. HARDWARE DISPLAY CONFIG (LovyanGFX)
// ==========================================
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX_ESP32_2432S028 : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
public:
    LGFX_ESP32_2432S028() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = VSPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 20000000; 
            cfg.pin_sclk = 14;
            cfg.pin_mosi = 13;
            cfg.pin_miso = 12;
            cfg.pin_dc   = 2;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 15;
            cfg.pin_rst          = -1;
            cfg.panel_width      = 240; 
            cfg.panel_height     = 320;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 2; // บังคับแสดงผลแนวนอน
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};
LGFX_ESP32_2432S028 lcd; 

// ==========================================
//           USER CONFIGURATIONS
// ==========================================
static BLEUUID serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    charUUID("0000fff2-0000-1000-8000-00805f9b34fb");

#define BMS_MAC_ADDRESS "ff:ff:11:e7:cd:b3" 
#define BMS_DEVICE_NAME "EC-Lifepo4 4S"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// ==========================================
//           GLOBAL STRUCTURE DATA
// ==========================================
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

volatile ECBMS_Data battery; 
String lineAccumulator = ""; 

static const NimBLEAdvertisedDevice* bmsDevice = nullptr; 
static NimBLEClient*                 pClient   = nullptr;
static bool doConnect = false;

String  targetMacStr = "";
uint8_t targetMacType = 0; 

static volatile bool dataReadyToUpdate = false; 

// ==========================================
//        LVGL v9 UI WIDGET HANDLES
// ==========================================
lv_obj_t * main_panel = nullptr;  // หน้า 1: Overview
lv_obj_t * cells_panel = nullptr; // หน้า 2: Live Summary (Cells)

lv_obj_t * label_title = nullptr;
lv_obj_t * line_indicator = nullptr; // เส้นใต้บอกสถานะเมนู

// สมาชิกหน้า 1 (Overview)
lv_obj_t * arc_soc = nullptr;
lv_obj_t * label_soc_text = nullptr;
lv_obj_t * label_voltage = nullptr;
lv_obj_t * label_current = nullptr;
lv_obj_t * label_capacity = nullptr;
lv_obj_t * label_chg_limit = nullptr;
lv_obj_t * label_dchg_limit = nullptr;
lv_obj_t * sw_charge = nullptr;
lv_obj_t * sw_discharge = nullptr;
lv_obj_t * sw_autodim = nullptr;

// สมาชิกหน้า 2 (Cells)
lv_obj_t * label_high_val = nullptr;
lv_obj_t * label_low_val = nullptr;
lv_obj_t * label_diff_val = nullptr;
lv_obj_t * box_cell1 = nullptr; lv_obj_t * label_c1_v = nullptr;
lv_obj_t * box_cell2 = nullptr; lv_obj_t * label_c2_v = nullptr;
lv_obj_t * box_cell3 = nullptr; lv_obj_t * label_c3_v = nullptr;
lv_obj_t * box_cell4 = nullptr; lv_obj_t * label_c4_v = nullptr;

// 🚨 จัดตั้งพิกัดขนาดบัฟเฟอร์วิดีโอใหม่ให้สัมพันธ์กับสัดส่วนขอบเขตแนวนอนของ LVGL v9
static uint8_t lv_buf[SCREEN_WIDTH * 16 * sizeof(lv_color_t)]; 

// ==========================================
//      LVGL v9 DISPLAY FLUSH CALLBACK
// ==========================================
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((uint16_t *)px_map, w * h, true);
    lcd.endWrite();

    lv_display_flush_ready(disp);
}

// ==========================================
//      SCREEN NAVIGATION CALLBACK EVENT
// ==========================================
// ฟังก์ชันตรวจจับการใช้นิ้วกดสัมผัสสลับหน้าจอ (Tab Click)
static void menu_click_cb(lv_event_t * e) {
    lv_obj_t * target = (lv_obj_t *)lv_event_get_target(e);
    char * menu_name = lv_label_get_text(target);

    if (strcmp(menu_name, "Overview") == 0) {
        lv_label_set_text(label_title, "ESMCOMM 1.0");
        lv_obj_remove_flag(main_panel, LV_OBJ_FLAG_HIDDEN); // แสดงหน้าหลัก
        lv_obj_add_flag(cells_panel, LV_OBJ_FLAG_HIDDEN);   // ซ่อนหน้าเซลล์
        lv_obj_align(line_indicator, LV_ALIGN_BOTTOM_LEFT, 15, 0); // เลื่อนเส้นใต้มาที่เมนู 1
    } 
    else if (strcmp(menu_name, "Live Summary") == 0) {
        lv_label_set_text(label_title, "PACK 01 DETAIL"); 
        lv_obj_add_flag(main_panel, LV_OBJ_FLAG_HIDDEN);      // ซ่อนหน้าหลัก
        lv_obj_remove_flag(cells_panel, LV_OBJ_FLAG_HIDDEN);  // แสดงหน้าเซลล์
        lv_obj_align(line_indicator, LV_ALIGN_BOTTOM_MID, 0, 0);   // เลื่อนเส้นใต้มาที่เมนู 2
    }
}

// ==========================================
//          LVGL v9 UI CREATION LAYOUT
// ==========================================
void create_unified_dashboard() {
    lv_obj_t * scr = lv_screen_active(); 
    lv_obj_set_style_bg_color(scr, lv_color_make(10, 17, 26), LV_STATE_DEFAULT);

    // ---- 1. แถบหัวกระดานข้อมูลร่วม (Static Header) ----
    label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "ECBMS_BL 2.1");
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 10, 5);

    lv_obj_t * label_ip = lv_label_create(scr);
    lv_label_set_text(label_ip, "IP 192.168.1.42");
    lv_obj_set_style_text_color(label_ip, lv_color_make(46, 204, 113), LV_STATE_DEFAULT); 
    lv_obj_set_style_text_font(label_ip, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_ip, LV_ALIGN_TOP_RIGHT, -10, 5);

    // ==========================================
    //    SCREEN TAB 1: OVERVIEW MAIN PANEL
    // ==========================================
    main_panel = lv_obj_create(scr);
    lv_obj_set_size(main_panel, 310, 185); 
    lv_obj_align(main_panel, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_set_style_bg_color(main_panel, lv_color_make(18, 30, 49), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(main_panel, lv_color_make(41, 128, 185), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(main_panel, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(main_panel, 8, LV_STATE_DEFAULT);
    lv_obj_clear_flag(main_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label_panel_header = lv_label_create(main_panel);
    lv_label_set_text(label_panel_header, "SYSTEM OVERVIEW");
    lv_obj_set_style_text_color(label_panel_header, lv_color_make(127, 140, 141), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_panel_header, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_align(label_panel_header, LV_ALIGN_TOP_LEFT, -5, -5);

    // เกจวงแหวนปรับปรุงขยายใหญ่ขนาด 95 พิกเซล
    arc_soc = lv_arc_create(main_panel);
    lv_obj_set_size(arc_soc, 95, 95); 
    lv_obj_align(arc_soc, LV_ALIGN_TOP_LEFT, -12, 8); 
    lv_arc_set_rotation(arc_soc, 135);
    lv_arc_set_bg_angles(arc_soc, 0, 270);
    lv_arc_set_value(arc_soc, 0); 
    lv_obj_set_style_arc_color(arc_soc, lv_color_make(46, 204, 113), LV_PART_INDICATOR | LV_STATE_DEFAULT); 
    lv_obj_set_style_arc_color(arc_soc, lv_color_make(44, 62, 80), LV_PART_MAIN | LV_STATE_DEFAULT);       
    lv_obj_remove_style(arc_soc, NULL, LV_PART_KNOB); 

    label_soc_text = lv_label_create(main_panel);
    lv_label_set_text(label_soc_text, "0%");
    lv_obj_set_style_text_color(label_soc_text, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_soc_text, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_obj_align_to(label_soc_text, arc_soc, LV_ALIGN_CENTER, 0, 0);

    // กล่อง Bus Voltage Box
    lv_obj_t * box_volt = lv_obj_create(main_panel);
    lv_obj_set_size(box_volt, 95, 40);
    lv_obj_align(box_volt, LV_ALIGN_TOP_LEFT, 80, 10);
    lv_obj_set_style_bg_color(box_volt, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box_volt, lv_color_make(52, 73, 94), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_volt, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box_volt, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box_volt, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * lbl_v_title = lv_label_create(box_volt);
    lv_label_set_text(lbl_v_title, "BUS VOLTAGE");
    lv_obj_set_style_text_color(lbl_v_title, lv_color_make(149, 165, 166), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_v_title, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_align(lbl_v_title, LV_ALIGN_TOP_LEFT, 4, 2);

    label_voltage = lv_label_create(box_volt);
    lv_label_set_text(label_voltage, "0.00V");
    lv_obj_set_style_text_color(label_voltage, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_voltage, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_align(label_voltage, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // กล่อง Battery Current Box
    lv_obj_t * box_curr = lv_obj_create(main_panel);
    lv_obj_set_size(box_curr, 110, 40);
    lv_obj_align(box_curr, LV_ALIGN_TOP_RIGHT, 5, 10);
    lv_obj_set_style_bg_color(box_curr, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box_curr, lv_color_make(52, 73, 94), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_curr, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box_curr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box_curr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * lbl_c_title = lv_label_create(box_curr);
    lv_label_set_text(lbl_c_title, "BATTERY CURRENT");
    lv_obj_set_style_text_color(lbl_c_title, lv_color_make(149, 165, 166), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_c_title, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_align(lbl_c_title, LV_ALIGN_TOP_LEFT, 4, 2);

    label_current = lv_label_create(box_curr);
    lv_label_set_text(label_current, "0.0 A");
    lv_obj_set_style_text_color(label_current, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_current, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_align(label_current, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // บล็อกความจุ
    lv_obj_t * lbl_cap_h = lv_label_create(main_panel);
    lv_label_set_text(lbl_cap_h, "CAP");
    lv_obj_set_style_text_color(lbl_cap_h, lv_color_make(155, 89, 182), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_cap_h, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(lbl_cap_h, LV_ALIGN_TOP_LEFT, 80, 56);

    label_capacity = lv_label_create(main_panel);
    lv_label_set_text(label_capacity, "1248 Ah");
    lv_obj_set_style_text_color(label_capacity, lv_color_make(189, 195, 199), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_capacity, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_capacity, LV_ALIGN_TOP_LEFT, 80, 72);

    lv_obj_t * lbl_chg_h = lv_label_create(main_panel);
    lv_label_set_text(lbl_chg_h, "CHG");
    lv_obj_set_style_text_color(lbl_chg_h, lv_color_make(46, 204, 113), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_chg_h, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(lbl_chg_h, LV_ALIGN_TOP_LEFT, 165, 56);

    label_chg_limit = lv_label_create(main_panel);
    lv_label_set_text(label_chg_limit, "100 A");
    lv_obj_set_style_text_color(label_chg_limit, lv_color_make(46, 204, 113), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_chg_limit, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_chg_limit, LV_ALIGN_TOP_LEFT, 165, 72);

    lv_obj_t * lbl_dchg_h = lv_label_create(main_panel);
    lv_label_set_text(lbl_dchg_h, "DCHG");
    lv_obj_set_style_text_color(lbl_dchg_h, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_dchg_h, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(lbl_dchg_h, LV_ALIGN_TOP_RIGHT, -15, 56);

    label_dchg_limit = lv_label_create(main_panel);
    lv_label_set_text(label_dchg_limit, "120 A");
    lv_obj_set_style_text_color(label_dchg_limit, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_dchg_limit, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_dchg_limit, LV_ALIGN_TOP_RIGHT, -15, 72);

    // ปุ่มสวิตช์ระบบควบคุม
    lv_obj_t * lbl_sw1 = lv_label_create(main_panel); 
    lv_label_set_text(lbl_sw1, "CHG");
    lv_obj_set_style_text_color(lbl_sw1, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_sw1, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_align(lbl_sw1, LV_ALIGN_TOP_LEFT, -5, 110);

    sw_charge = lv_switch_create(main_panel);
    lv_obj_set_size(sw_charge, 32, 16);
    lv_obj_align(sw_charge, LV_ALIGN_TOP_LEFT, -5, 124);
    lv_obj_add_state(sw_charge, LV_STATE_CHECKED);

    lv_obj_t * lbl_sw2 = lv_label_create(main_panel); 
    lv_label_set_text(lbl_sw2, "DIS");
    lv_obj_set_style_text_color(lbl_sw2, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_sw2, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_align(lbl_sw2, LV_ALIGN_TOP_LEFT, 42, 110);

    sw_discharge = lv_switch_create(main_panel);
    lv_obj_set_size(sw_discharge, 32, 16);
    lv_obj_align(sw_discharge, LV_ALIGN_TOP_LEFT, 45, 124);
    lv_obj_add_state(sw_discharge, LV_STATE_CHECKED);

    lv_obj_t * lbl_sw3 = lv_label_create(main_panel); 
    lv_label_set_text(lbl_sw3, "AB");
    lv_obj_set_style_text_color(lbl_sw3, lv_color_make(127, 140, 141), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_sw3, &lv_font_montserrat_10, LV_STATE_DEFAULT);
    lv_obj_align(lbl_sw3, LV_ALIGN_TOP_LEFT, 106, 110);

    sw_autodim = lv_switch_create(main_panel);
    lv_obj_set_size(sw_autodim, 32, 16);
    lv_obj_align(sw_autodim, LV_ALIGN_TOP_LEFT, 108, 124);
    lv_obj_add_state(sw_autodim, LV_STATE_CHECKED);

    // ==========================================
    //    SCREEN TAB 2: CELLS SUMMARY PANEL
    // ==========================================
    cells_panel = lv_obj_create(scr);
    lv_obj_set_size(cells_panel, 310, 185);
    lv_obj_align(cells_panel, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_set_style_bg_color(cells_panel, lv_color_make(18, 30, 49), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cells_panel, lv_color_make(41, 128, 185), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cells_panel, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cells_panel, 8, LV_STATE_DEFAULT);
    lv_obj_clear_flag(cells_panel, LV_OBJ_FLAG_SCROLLABLE);

    // ตั้งค่าซ่อนหน้าเซลล์ไว้เริ่มต้น เพื่อรอทริกเกอร์การกดสลับหน้า
    lv_obj_add_flag(cells_panel, LV_OBJ_FLAG_HIDDEN);

    // รายการวิเคราะห์แรงดันส่วนหัวกน้าหน้า 2 (HIGH, LOW, DIFF)
    lv_obj_t * lbl_high_t = lv_label_create(cells_panel); 
    lv_label_set_text(lbl_high_t, "HIGH");
    lv_obj_set_style_text_color(lbl_high_t, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_high_t, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(lbl_high_t, LV_ALIGN_TOP_LEFT, 0, 0);

    label_high_val = lv_label_create(cells_panel); 
    lv_label_set_text(label_high_val, "0.000 V");
    lv_obj_set_style_text_color(label_high_val, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_high_val, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_high_val, LV_ALIGN_TOP_LEFT, 45, 0);

    lv_obj_t * lbl_low_t = lv_label_create(cells_panel); 
    lv_label_set_text(lbl_low_t, "LOW");
    lv_obj_set_style_text_color(lbl_low_t, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_low_t, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(lbl_low_t, LV_ALIGN_TOP_LEFT, 110, 0);

    label_low_val = lv_label_create(cells_panel); 
    lv_label_set_text(label_low_val, "0.000 V");
    lv_obj_set_style_text_color(label_low_val, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_low_val, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_low_val, LV_ALIGN_TOP_LEFT, 150, 0);

    lv_obj_t * lbl_diff_t = lv_label_create(cells_panel); 
    lv_label_set_text(lbl_diff_t, "DIFF");
    lv_obj_set_style_text_color(lbl_diff_t, lv_color_make(241, 196, 15), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_diff_t, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(lbl_diff_t, LV_ALIGN_TOP_LEFT, 215, 0);

    label_diff_val = lv_label_create(cells_panel); 
    lv_label_set_text(label_diff_val, "0.000 V");
    lv_obj_set_style_text_color(label_diff_val, lv_color_make(241, 196, 15), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_diff_val, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(label_diff_val, LV_ALIGN_TOP_LEFT, 250, 0);

    // การ์ดรายเซลล์ Grid 01 - 04
    int box_w = 68; int box_h = 38; int start_y = 22;

    // การ์ด 01
    box_cell1 = lv_obj_create(cells_panel); 
    lv_obj_set_size(box_cell1, box_w, box_h);
    lv_obj_align(box_cell1, LV_ALIGN_TOP_LEFT, -5, start_y);
    lv_obj_set_style_bg_color(box_cell1, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box_cell1, lv_color_make(52, 73, 94), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_cell1, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box_cell1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box_cell1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t * t1 = lv_label_create(box_cell1); 
    lv_label_set_text(t1, "01");
    lv_obj_set_style_text_color(t1, lv_color_make(149, 165, 166), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(t1, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 4, 2);label_c1_v = lv_label_create(box_cell1); 
    lv_label_set_text(label_c1_v, "-.- V");
    lv_obj_set_style_text_font(label_c1_v, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(label_c1_v, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // การ์ด 02
    box_cell2 = lv_obj_create(cells_panel); 
    lv_obj_set_size(box_cell2, box_w, box_h);
    lv_obj_align(box_cell2, LV_ALIGN_TOP_LEFT, 70, start_y);
    lv_obj_set_style_bg_color(box_cell2, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box_cell2, lv_color_make(52, 73, 94), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_cell2, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box_cell2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box_cell2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t * t2 = lv_label_create(box_cell2);
    lv_label_set_text(t2, "02");
    lv_obj_set_style_text_color(t2, lv_color_make(149, 165, 166), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(t2, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(t2, LV_ALIGN_TOP_LEFT, 4, 2);label_c2_v = lv_label_create(box_cell2); 
    lv_label_set_text(label_c2_v, "-.- V");
    lv_obj_set_style_text_font(label_c2_v, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(label_c2_v, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // การ์ด 03
    box_cell3 = lv_obj_create(cells_panel); 
    lv_obj_set_size(box_cell3, box_w, box_h);
    lv_obj_align(box_cell3, LV_ALIGN_TOP_LEFT, 145, start_y);
    lv_obj_set_style_bg_color(box_cell3, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box_cell3, lv_color_make(52, 73, 94), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_cell3, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box_cell3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box_cell3, LV_OBJ_FLAG_SCROLLABLE);lv_obj_t * t3 = lv_label_create(box_cell3); 
    lv_label_set_text(t3, "03");
    lv_obj_set_style_text_color(t3, lv_color_make(149, 165, 166), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(t3, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(t3, LV_ALIGN_TOP_LEFT, 4, 2);
    label_c3_v = lv_label_create(box_cell3); 
    lv_label_set_text(label_c3_v, "-.- V");
    lv_obj_set_style_text_font(label_c3_v, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(label_c3_v, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // การ์ด 04
    box_cell4 = lv_obj_create(cells_panel); 
    lv_obj_set_size(box_cell4, box_w, box_h);
    lv_obj_align(box_cell4, LV_ALIGN_TOP_RIGHT, 5, start_y);
    lv_obj_set_style_bg_color(box_cell4, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box_cell4, lv_color_make(52, 73, 94), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box_cell4, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box_cell4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box_cell4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t * t4 = lv_label_create(box_cell4); 
    lv_label_set_text(t4, "04");
    lv_obj_set_style_text_color(t4, lv_color_make(149, 165, 166), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(t4, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(t4, LV_ALIGN_TOP_LEFT, 4, 2);
    label_c4_v = lv_label_create(box_cell4); 
    lv_label_set_text(label_c4_v, "-.- V");
    lv_obj_set_style_text_font(label_c4_v, &lv_font_montserrat_10, LV_STATE_DEFAULT); 
    lv_obj_align(label_c4_v, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // ---- Static Bottom Tab Navigation Menu Bar ----
    lv_obj_t * tab_bar = lv_obj_create(scr);
    lv_obj_set_size(tab_bar, 320, 25);
    lv_obj_align(tab_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(tab_bar, lv_color_make(10, 17, 26), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(tab_bar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(tab_bar, LV_OBJ_FLAG_SCROLLABLE);

    // ปุ่มสลับหน้า 1 (Overview)
    lv_obj_t * btn_overview = lv_label_create(tab_bar);
    lv_label_set_text(btn_overview, "Overview");
    lv_obj_set_style_text_color(btn_overview, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(btn_overview, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(btn_overview, LV_ALIGN_LEFT_MID, 15, 0);
    lv_obj_add_flag(btn_overview, LV_OBJ_FLAG_CLICKABLE); 
    // เปิดสิทธิ์ให้กดได้
    lv_obj_add_event_cb(btn_overview, menu_click_cb, LV_EVENT_CLICKED, NULL); 
    // ผูก Callback

    // ปุ่มสลับหน้า 2 (Live Summary)
    lv_obj_t * btn_summary = lv_label_create(tab_bar);
    lv_label_set_text(btn_summary, "Live Summary");
    lv_obj_set_style_text_color(btn_summary, lv_color_make(127, 140, 141), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(btn_summary, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(btn_summary, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(btn_summary, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_summary, menu_click_cb, LV_EVENT_CLICKED, NULL);

    // ปุ่มหน้า 3 (Settings)
    lv_obj_t * btn_settings = lv_label_create(tab_bar);
    lv_label_set_text(btn_settings, "Settings");
    lv_obj_set_style_text_color(btn_settings, lv_color_make(127, 140, 141), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(btn_settings, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(btn_settings, LV_ALIGN_RIGHT_MID, -25, 0);

    // เส้นขีดใต้บอกสถานะเมนู
    line_indicator = lv_obj_create(tab_bar);
    lv_obj_set_size(line_indicator, 55, 2);
    lv_obj_align(line_indicator, LV_ALIGN_BOTTOM_LEFT, 15, 0);
    lv_obj_set_style_bg_color(line_indicator, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(line_indicator, 0, LV_STATE_DEFAULT);
}

    // ==========================================
    //      DYNAMIC UI DATA REFRESH BRIDGE
    // ==========================================
    void update_dashboard_from_bms() {
    if (arc_soc == nullptr || label_soc_text == nullptr || label_voltage == nullptr) return;
    // ---- [อัปเดตข้อมูลหน้า 1: Overview] ----
    int current_soc = battery.stateOfCharge;
    if(current_soc < 0) current_soc = 0;
    if(current_soc > 100) current_soc = 100;
    lv_arc_set_value(arc_soc, current_soc);
    String socStr = String(current_soc) + "%";
    lv_label_set_text(label_soc_text, socStr.c_str());
    String voltStr = String(battery.totalVoltage, 2) + "V";
    lv_label_set_text(label_voltage, voltStr.c_str());
    String currentStr = String(battery.currentAmps, 1) + " A";
    lv_label_set_text(label_current, currentStr.c_str());
    if(battery.currentAmps < 0) {lv_obj_set_style_bg_color(box_cell1, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_current, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    } else {
    lv_obj_set_style_text_color(label_current, lv_color_make(46, 204, 113), LV_STATE_DEFAULT);
    }
    // ---- [🚨 อัปเดตข้อมูลหน้า 2: Cells รายก้อนพร้อมย้อมสีอัจฉริยะ] ----
    if (label_high_val != nullptr && label_c1_v != nullptr) {
    lv_label_set_text(label_high_val, (String(battery.maxCellVoltage, 3) + " V").c_str());
    lv_label_set_text(label_low_val, (String(battery.minCellVoltage, 3) + " V").c_str());
    lv_label_set_text(label_diff_val, (String(battery.cellDelta, 3) + " V").c_str());
    lv_label_set_text(label_c1_v, (String(battery.cell1, 3) + " V").c_str());
    lv_label_set_text(label_c2_v, (String(battery.cell2, 3) + " V").c_str());
    lv_label_set_text(label_c3_v, (String(battery.cell3, 3) + " V").c_str());
    lv_label_set_text(label_c4_v, (String(battery.cell4, 3) + " V").c_str());
    // คืนค่าสีการ์ดและฟอนต์เป็นปกติก่อนสแกนใหม่
    lv_obj_set_style_bg_color(box_cell1, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(box_cell2, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(box_cell3, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(box_cell4, lv_color_make(24, 44, 70), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c1_v, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c2_v, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c3_v, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c4_v, lv_color_white(), LV_STATE_DEFAULT);
    // ย้อมการ์ดโวลต์สูงสุด (HIGH) -> สีส้มอมแดง
    if (battery.maxCellIndex == 1) {
    lv_obj_set_style_bg_color(box_cell1, lv_color_make(74, 35, 18), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c1_v, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    } 
    else if (battery.maxCellIndex == 2) {
    lv_obj_set_style_bg_color(box_cell2, lv_color_make(74, 35, 18), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c2_v, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    }
    else if (battery.maxCellIndex == 3) {
    lv_obj_set_style_bg_color(box_cell3, lv_color_make(74, 35, 18), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c3_v, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    }
    else if (battery.maxCellIndex == 4) {
    lv_obj_set_style_bg_color(box_cell4, lv_color_make(74, 35, 18), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_c4_v, lv_color_make(230, 126, 34), LV_STATE_DEFAULT);
    }
    // ย้อมการ์ดโวลต์ต่ำสุด (LOW) -> สีฟ้าสด
    if (battery.minCellIndex == 1) {
        lv_obj_set_style_bg_color(box_cell1, lv_color_make(15, 41, 67), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_c1_v, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    } 
    else if (battery.minCellIndex == 2) {
        lv_obj_set_style_bg_color(box_cell2, lv_color_make(15, 41, 67), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_c2_v, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    }
    else if (battery.minCellIndex == 3) {
        lv_obj_set_style_bg_color(box_cell3, lv_color_make(15, 41, 67), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_c3_v, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    } 
    else if (battery.minCellIndex == 4) {
        lv_obj_set_style_bg_color(box_cell4, lv_color_make(15, 41, 67), LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_c4_v, lv_color_make(52, 152, 219), LV_STATE_DEFAULT);
    }
}
}

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
        valStr.trim();
        float value = valStr.toFloat();

        if      (key == "1")     { battery.cell1 = value; }
        else if (key == "2")     { battery.cell2 = value; }
        else if (key == "3")     { battery.cell3 = value; }
        else if (key == "4")     { battery.cell4 = value; }
        else if (key == "zdy")   { battery.totalVoltage = value; dataReadyToUpdate = true; } // อัปเดตทันทีเมื่อเจอโวลต์
        else if (key == "yc")    { battery.cellDelta = value; }
        else if (key == "max")   { battery.maxCellVoltage = value; }
        else if (key == "min")   { battery.minCellVoltage = value; }
        else if (key == "zg")    { battery.maxCellIndex = (int)value; }
        else if (key == "zd")    { battery.minCellIndex = (int)value; }
        else if (key == "bl")    { battery.stateOfCharge = (int)value; dataReadyToUpdate = true; } // อัปเดตเมื่อเจอเปอร์เซ็นต์
        else if (key == "dl")    { battery.currentAmps = value; dataReadyToUpdate = true; } // อัปเดตเมื่อเจอกระแส
        else if (key == "moswd") { battery.mosfetTemp = value; }
        else if (key == "jhwd")  { battery.boardTemp = value; }
    } 
    else if (line.indexOf("jhstop") != -1 || line == "jhstop") {
        Serial.println("\n[BMS] --- อัปเดตข้อมูลแพ็กเก็ตใหม่สำเร็จ ---");
        Serial.printf("แรงดันรวม: %.2f V | SoC: %d %%\n", battery.totalVoltage, battery.stateOfCharge);
        
        // ส่งสัญญาณบอกลูปหลักว่า แพ็กเก็ตมาครบแล้ว สั่งวาดจอได้เลย!
        dataReadyToUpdate = true; 
    }
}

    void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
        for (size_t i = 0; i < length; i++) {char c = (char)pData[i];
        if (c == '\n' || c == '\r') {
        if (lineAccumulator.length() > 0) {processFinishedLine(lineAccumulator);
        lineAccumulator = "";
    }
} else {
    lineAccumulator += c;
}
}
}

    class ClientCallbacks : public NimBLEClientCallbacks {void onConnect(NimBLEClient* pClient) override {
        Serial.println("[BLE] Connected successfully.");
    }
    void onDisconnect(NimBLEClient* pClient, int reason) override {
        doConnect = false;
        bmsDevice = nullptr;
        NimBLEDevice::getScan()->start(0);
    }
};
    class AdvertisedDeviceCallbacks: public NimBLEScanCallbacks {void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        String currentAddress = advertisedDevice->getAddress().toString().c_str();
        String currentName    = advertisedDevice->getName().c_str();
        if (currentAddress == BMS_MAC_ADDRESS || currentName == BMS_DEVICE_NAME) {
            NimBLEDevice::getScan()->stop();
            targetMacStr = currentAddress;
            targetMacType = advertisedDevice->getAddressType();
            bmsDevice = advertisedDevice;
            doConnect = true;
        }
    }
};

    bool connectToBMS() {
        if (targetMacStr.length() == 0) return false;
        if (pClient != nullptr) {pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;
    }
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks());
        if (!pClient->connect(NimBLEAddress(targetMacStr.c_str(), targetMacType))) {
            NimBLEDevice::deleteClient(pClient); 
            pClient = nullptr; 
            return false;
        }
        delay(500);
        NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService == nullptr) {pClient->disconnect(); 
        NimBLEDevice::deleteClient(pClient); 
        pClient = nullptr; return false;
    }
    pRemoteService->getCharacteristics(true);
    delay(200);
    NimBLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic->canNotify() || pRemoteCharacteristic->canIndicate()) {
        if (!pRemoteCharacteristic->subscribe(true, notifyCallback, false)) {
            pClient->disconnect(); 
            NimBLEDevice::deleteClient(pClient);
             pClient = nullptr; return false;
         }
     }
     return true;
 }

// ==========================================
//             MAIN CORE SETUP
// ==========================================
    void setup() {
    Serial.begin(115200);

    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    lcd.init();
    lcd.setRotation(1);
    lcd.fillScreen(TFT_BLACK);
    lv_init();

    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_buffers(disp, lv_buf, NULL, sizeof(lv_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, my_disp_flush);

    // บูตรวมระเบียบโครงหน้าจอแบบรวมศูนย์รองรับ Tab สลับหน้า
    create_unified_dashboard();

    lv_timer_handler();
    delay(50);

    NimBLEDevice::init("ESP32_BMS_Gateway");
    NimBLEDevice::setPower(5);
    NimBLEScan* pNimBLEScan = NimBLEDevice::getScan();
    pNimBLEScan->setScanCallbacks(new AdvertisedDeviceCallbacks(), false);
    pNimBLEScan->start(0); 

}

    void loop() {
        lv_timer_handler();
        if (dataReadyToUpdate) {dataReadyToUpdate = false;update_dashboard_from_bms();
    }
    if (doConnect) {
        doConnect = false;
        connectToBMS();
    }
    delay(1);
}
