#include "lvgl.h"
#include "src/widgets/label/lv_label_private.h"
#include <algorithm>
#include <string>
#include <cassert>
#include <cstdio>
#define CONFIG_USE_MULTILINE_CHAT_MESSAGE 0
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define LV_HOR_RES 240
struct Theme { int spacing(int n) {return n;} lv_color_t background_color(){return lv_color_white();} lv_color_t text_color(){return lv_color_black();} };
struct DisplayLockGuard { DisplayLockGuard(void*){} };
enum DeviceState {kDeviceStateIdle, kDeviceStateListening, kDeviceStateSpeaking, kDeviceStateWifiConfiguring};
struct Application {
 DeviceState state=kDeviceStateIdle;
 static Application& GetInstance(){static Application a;return a;}
 DeviceState GetDeviceState(){return state;}
};
struct LvglDisplay {void SetStatus(const char*){} void UpdateStatusBar(bool){} };
struct LcdDisplay : LvglDisplay {
 lv_obj_t *top_bar_=nullptr,*status_bar_=nullptr;
 void UpdateStatusBarVisibility();
 void SetStatus(const char* status);
 void UpdateStatusBar(bool update_all=false);

 lv_obj_t *bottom_bar_=nullptr,*chat_message_label_=nullptr;
 bool setup_ui_called_=true,hide_subtitle_=false;
 void SetupUI() {
 auto screen=lv_screen_active(); Theme t; auto lvgl_theme=&t;
 auto text_font=LV_FONT_DEFAULT;
    /* Top layer: Bottom bar - fixed height at bottom */
    bottom_bar_ = lv_obj_create(screen);
    lv_obj_set_size(bottom_bar_, LV_HOR_RES, text_font->line_height + lvgl_theme->spacing(8));
    lv_obj_set_style_radius(bottom_bar_, 0, 0);
    lv_obj_set_style_bg_color(bottom_bar_, lvgl_theme->background_color(), 0);
    lv_obj_set_style_text_color(bottom_bar_, lvgl_theme->text_color(), 0);
    lv_obj_set_style_pad_all(bottom_bar_, 0, 0);
    lv_obj_set_style_pad_left(bottom_bar_, lvgl_theme->spacing(4), 0);
    lv_obj_set_style_pad_right(bottom_bar_, lvgl_theme->spacing(4), 0);
    lv_obj_set_style_border_width(bottom_bar_, 0, 0);
    lv_obj_set_scrollbar_mode(bottom_bar_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(bottom_bar_, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* chat_message_label_ placed in bottom_bar_, single-line horizontal scroll */
    chat_message_label_ = lv_label_create(bottom_bar_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES - lvgl_theme->spacing(8));
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(chat_message_label_, lvgl_theme->text_color(), 0);
    lv_obj_align(chat_message_label_, LV_ALIGN_CENTER, 0, 0);

    // Start scrolling after a delay (short text won't scroll)
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_delay(&a, 1000);
    lv_anim_set_reverse_delay(&a, 1000);
    lv_anim_set_repeat_delay(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_obj_set_style_anim(chat_message_label_, &a, LV_PART_MAIN);
    lv_obj_set_style_anim_duration(chat_message_label_, lv_anim_speed_clamped(60, 300, 60000),
                                   LV_PART_MAIN);
    lv_obj_add_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);  // Hide until there is content

 }
 void SetChatMessage(const char*,const char*);
};
void LcdDisplay::SetChatMessage(const char* role, const char* content) {
    if (!setup_ui_called_) {
        ESP_LOGW(TAG, "SetChatMessage('%s', '%s') called before SetupUI() - message will be lost!",
                 role, content);
    }
    DisplayLockGuard lock(this);
    if (chat_message_label_ == nullptr) {
        if (setup_ui_called_) {
            ESP_LOGW(TAG,
                     "SetChatMessage('%s', '%s') failed: chat_message_label_ is nullptr (SetupUI() "
                     "was called but label not created)",
                     role, content);
        }
        return;
    }
    lv_anim_delete(chat_message_label_, nullptr);
#if CONFIG_USE_MULTILINE_CHAT_MESSAGE
    lv_label_set_text(chat_message_label_, content != nullptr ? content : "");
#else
    // Explicit line breaks would make LVGL scroll vertically even in scroll mode.
    std::string single_line = content != nullptr ? content : "";
    std::replace(single_line.begin(), single_line.end(), '\r', ' ');
    std::replace(single_line.begin(), single_line.end(), '\n', ' ');
    lv_label_set_text(chat_message_label_, single_line.c_str());
#endif
    // Show bottom_bar_ only when there is content (and subtitle is not globally hidden)
    if (bottom_bar_ != nullptr) {
        if (content == nullptr || content[0] == '\0') {
            lv_obj_add_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);
        } else if (!hide_subtitle_) {
            lv_obj_remove_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);
        }
    }
#if CONFIG_USE_MULTILINE_CHAT_MESSAGE
    // Re-align bottom_bar_ after text change so it stays anchored to the bottom
    // as its height adapts to the wrapped content.
    if (bottom_bar_ != nullptr) {
        lv_obj_align(bottom_bar_, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
#endif
}


void LcdDisplay::UpdateStatusBarVisibility() {
    DisplayLockGuard lock(this);
    const bool idle = Application::GetInstance().GetDeviceState() == kDeviceStateIdle;
    // Hide both parent layers so clock updates and notification timers cannot
    // bring back individual labels while the device is idle.
    for (auto bar : {top_bar_, status_bar_}) {
        if (bar == nullptr) {
            continue;
        }
        if (idle) {
            lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void LcdDisplay::SetStatus(const char* status) {
    LvglDisplay::SetStatus(status);
    UpdateStatusBarVisibility();
}

void LcdDisplay::UpdateStatusBar(bool update_all) {
    UpdateStatusBarVisibility();
    LvglDisplay::UpdateStatusBar(update_all);
}

static void advance(unsigned ms) { for(unsigned i=0;i<ms;i+=10) {lv_tick_inc(10);lv_timer_handler();} }
int main() {
 lv_init();auto d=lv_display_create(240,240);static uint16_t buf[240*40];
 lv_display_set_buffers(d,buf,nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(d,[](lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);});
 LcdDisplay display;display.SetupUI();
 auto obj=display.chat_message_label_;auto label=(lv_label_t*)obj;
 display.SetChatMessage("assistant","你好包子");advance(100);
 assert(label->text_size.x<=lv_obj_get_content_width(obj));
 assert(lv_obj_get_style_text_align(obj,LV_PART_MAIN)==LV_TEXT_ALIGN_CENTER);
 advance(5000);assert(label->offset.x==0 && label->offset.y==0);
 lv_point_t letter;lv_label_get_letter_pos(obj,0,&letter);
 assert(letter.x>0);puts("PASS short Chinese text centered and stationary");
 display.SetChatMessage("assistant","你好包子你好包子你好包子你好包子你好包子你好包子");advance(200);
 assert(label->text_size.x>lv_obj_get_content_width(obj));
 advance(2000);assert(label->offset.x<0 && label->offset.y==0);
 int min_x=label->offset.x;bool returned=false;
 for(int i=0;i<3000;i++){advance(10);assert(label->offset.y==0);if(label->offset.x<min_x)min_x=label->offset.x;else if(label->offset.x>min_x && min_x<0)returned=true;}
 assert(returned);puts("PASS long Chinese text scrolls horizontally in both directions");
 display.SetChatMessage("assistant","你好\r\n包子");advance(2000);
 assert(std::string(lv_label_get_text(obj))=="你好  包子");
 assert(label->offset.x==0 && label->offset.y==0);puts("PASS multiline input remains a centered single line after long text");
 display.SetChatMessage("assistant",nullptr);advance(100);
 assert(lv_obj_has_flag(display.bottom_bar_,LV_OBJ_FLAG_HIDDEN));
 assert(std::string(lv_label_get_text(obj)).empty());puts("PASS null/empty message clears and hides subtitle");

 display.UpdateStatusBar(); // Safe before parent bars exist.
 display.top_bar_=lv_obj_create(lv_screen_active());
 display.status_bar_=lv_obj_create(lv_screen_active());
 auto status=lv_label_create(display.status_bar_);
 display.SetStatus("待命");
 assert(lv_obj_has_flag(display.top_bar_,LV_OBJ_FLAG_HIDDEN));
 assert(lv_obj_has_flag(display.status_bar_,LV_OBJ_FLAG_HIDDEN));
 lv_obj_remove_flag(status,LV_OBJ_FLAG_HIDDEN); // Simulate notification timeout.
 assert(!lv_obj_is_visible(status));
 display.SetStatus("12:00");display.UpdateStatusBar();
 assert(!lv_obj_is_visible(status));
 puts("PASS idle hides both top layers despite clock/notification updates");
 for(auto state : {kDeviceStateListening,kDeviceStateSpeaking,kDeviceStateWifiConfiguring}) {
  Application::GetInstance().state=state;display.SetStatus("active");display.UpdateStatusBar();advance(50);
  assert(lv_obj_is_visible(display.top_bar_));assert(lv_obj_is_visible(status));
  Application::GetInstance().state=kDeviceStateIdle;display.UpdateStatusBar();
  assert(!lv_obj_is_visible(display.top_bar_));assert(!lv_obj_is_visible(status));
 }
 puts("PASS leaving idle restores status bar and re-entering idle hides it");
 lv_deinit();
}
