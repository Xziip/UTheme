#include "Screen.hpp"
#include "Gfx.hpp"
#include "common.h"
#include "utils/LanguageManager.hpp"
#include "utils/BgmNotification.hpp"
#include "utils/FileLogger.hpp"
#include "screens/MainScreen.hpp"

// 全局BGM通知实例
BgmNotification Screen::sBgmNotification;

void Screen::DrawTopBar(const char *name) {
    // draw top bar - 增加高度与主界面一致
    Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, 120, Gfx::COLOR_BARS);

    // draw top bar content - 使用UTheme
    Gfx::DrawIcon(60, 60, 60, Gfx::COLOR_ACCENT, 0xf53f, Gfx::ALIGN_VERTICAL);
    Gfx::Print(140, 60, 56, Gfx::COLOR_TEXT, _("app_name"), Gfx::ALIGN_VERTICAL);
    
    // Draw version number with local mode indicator if Mocha is unavailable
    int versionX = 140 + Gfx::GetTextWidth(56, _("app_name")) + 20;
    std::string versionText = APP_VERSION_FULL;
    if (!MainScreen::IsMochaAvailable()) {
        versionText += " (";
        versionText += _("common.local_mode");
        versionText += ")";
    }
    Gfx::Print(versionX, 65, 32, Gfx::COLOR_ALT_TEXT, versionText.c_str(), Gfx::ALIGN_VERTICAL);
    
    // Draw page name on the right if provided
    if (name) {
        Gfx::Print(Gfx::SCREEN_WIDTH - 60, 60, 48, Gfx::COLOR_ALT_TEXT, name, Gfx::ALIGN_VERTICAL | Gfx::ALIGN_RIGHT);
    }
    
    // Draw accent line at bottom
    Gfx::DrawRectFilled(0, 115, Gfx::SCREEN_WIDTH, 5, Gfx::COLOR_ACCENT);
}

void Screen::DrawAnimatedTopBar(const std::string& name, Animation& titleAnim, uint16_t icon) {
    titleAnim.Update();
    float titleProgress = titleAnim.GetValue();
    
    // draw top bar
    Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, 120, Gfx::COLOR_BARS);
    
    int titleY = 25 - (int)((1.0f - titleProgress) * 50);
    SDL_Color titleColor = Gfx::COLOR_TEXT;
    titleColor.a = (Uint8)(255 * titleProgress);
    
    Gfx::DrawIcon(60, titleY + 40, 60, Gfx::COLOR_ACCENT, icon, Gfx::ALIGN_VERTICAL);
    Gfx::Print(140, titleY + 40, 56, titleColor, _("app_name"), Gfx::ALIGN_VERTICAL);
    
    // Draw version number with local mode indicator if Mocha is unavailable
    SDL_Color versionColor = Gfx::COLOR_ALT_TEXT;
    versionColor.a = (Uint8)(200 * titleProgress);
    int versionX = 140 + Gfx::GetTextWidth(56, _("app_name")) + 20;
    std::string versionText = APP_VERSION_FULL;
    if (!MainScreen::IsMochaAvailable()) {
        versionText += " (";
        versionText += _("common.local_mode");
        versionText += ")";
    }
    Gfx::Print(versionX, titleY + 45, 32, versionColor, versionText.c_str(), Gfx::ALIGN_VERTICAL);
    
    // Draw page name
    if (!name.empty()) {
        SDL_Color pageColor = Gfx::COLOR_ALT_TEXT;
        pageColor.a = (Uint8)(220 * titleProgress);
        Gfx::Print(Gfx::SCREEN_WIDTH - 60, titleY + 40, 48, pageColor, name, Gfx::ALIGN_VERTICAL | Gfx::ALIGN_RIGHT);
    }
    
    // Draw animated accent line
    SDL_Color accentColor = Gfx::COLOR_ACCENT;
    accentColor.a = (Uint8)(180 * titleProgress);
    Gfx::DrawRectFilled(0, 115, (int)(Gfx::SCREEN_WIDTH * titleProgress), 5, accentColor);
}

void Screen::DrawBottomBar(const char *leftHint, const char *centerHint, const char *rightHint) {
    // draw bottom bar - 与主界面统一高度和字体
    Gfx::DrawRectFilled(0, Gfx::SCREEN_HEIGHT - 80, Gfx::SCREEN_WIDTH, 80, Gfx::COLOR_BARS);

    // draw bottom bar content - 统一字体大小为40
    if (leftHint) {
        Gfx::Print(60, Gfx::SCREEN_HEIGHT - 40, 40, Gfx::COLOR_TEXT, leftHint, Gfx::ALIGN_VERTICAL);
    }
    if (centerHint) {
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT - 40, 40, Gfx::COLOR_TEXT, centerHint, Gfx::ALIGN_CENTER);
    }
    if (rightHint) {
        Gfx::Print(Gfx::SCREEN_WIDTH - 60, Gfx::SCREEN_HEIGHT - 40, 40, Gfx::COLOR_TEXT, rightHint, Gfx::ALIGN_VERTICAL | Gfx::ALIGN_RIGHT);
    }
}

int Screen::DrawHeader(int x, int y, int w, uint16_t icon, const char *text) {
    const int iconWidth = Gfx::GetIconWidth(50, icon);
    const int width     = iconWidth + 32 + Gfx::GetTextWidth(50, text);
    const int xStart    = x + (w / 2) - (width / 2);

    Gfx::DrawIcon(xStart, y, 50, Gfx::COLOR_TEXT, icon, Gfx::ALIGN_VERTICAL);
    Gfx::Print(xStart + iconWidth + 32, y, 50, Gfx::COLOR_TEXT, text, Gfx::ALIGN_VERTICAL);
    Gfx::DrawRectFilled(x, y + 32, w, 4, Gfx::COLOR_ACCENT);

    return y + 64;
}

int Screen::DrawList(int x, int y, int w, const ScreenList &items) {
    int yOff = y;
    for (const auto &item : items) {
        Gfx::Print(x + 16, yOff, 40, Gfx::COLOR_TEXT, item.first, Gfx::ALIGN_VERTICAL);
        Gfx::Print(x + w - 16, yOff, 40, Gfx::COLOR_TEXT, item.second.string, Gfx::ALIGN_VERTICAL | Gfx::ALIGN_RIGHT, item.second.monospace);
        yOff += std::max(
                Gfx::GetTextHeight(40, item.first),
                Gfx::GetTextHeight(40, item.second.string, item.second.monospace));
    }

    return yOff + 32;
}

void Screen::DrawTouchDebugInfo(const Input &input, bool enabled) {
    if (!enabled) return;
    
    // 计算触摸坐标
    int touchX = 0, touchY = 0;
    if (input.data.touched && input.data.validPointer) {
        float scaleX = 1920.0f / 1280.0f;
        float scaleY = 1080.0f / 720.0f;
        touchX = (int)((input.data.x * scaleX) + 960);
        touchY = (int)(540 - (input.data.y * scaleY));
    }
    
    // 绘制调试信息 (固定在屏幕顶部)
    char touchInfo[256];
    snprintf(touchInfo, sizeof(touchInfo), 
            "Touch Debug - T:%d V:%d LT:%d BD:0x%X Raw:(%d,%d) Scr:(%d,%d)", 
            input.data.touched ? 1 : 0, 
            input.data.validPointer ? 1 : 0,
            input.lastData.touched ? 1 : 0,
            input.data.buttons_d,
            input.data.x, input.data.y,
            touchX, touchY);
    
    SDL_Color debugColor = {255, 255, 0, 255};  // 黄色
    Gfx::Print(20, 100, 22, debugColor, touchInfo, Gfx::ALIGN_VERTICAL);
}

BgmNotification& Screen::GetBgmNotification() {
    return sBgmNotification;
}

void Screen::UpdateBgmNotification() {
    sBgmNotification.Update();
}

void Screen::DrawBgmNotification() {
    sBgmNotification.Draw();
}

// 静态变量初始化
int Screen::sBackButtonX = 1820;
int Screen::sBackButtonY = 100;
bool Screen::sBackButtonDragging = false;bool Screen::sButtonPressed = false;int Screen::sDragStartX = 0;
int Screen::sDragStartY = 0;
int Screen::sDragOffsetX = 0;
int Screen::sDragOffsetY = 0;
int Screen::sHoldFrames = 0;

void Screen::DrawBackButton() {
    const int size = 70;  // 圆形按钮直径
    const int radius = size / 2;  // 圆角半径等于直径的一半形成圆形
    const int iconSize = 36;
    
    // 根据拖动状态调整颜色
    SDL_Color bgColor = Gfx::COLOR_CARD_BG;
    SDL_Color borderColor = Gfx::COLOR_ACCENT;
    SDL_Color iconColor = Gfx::COLOR_TEXT;
    
    if (sBackButtonDragging) {
        // 拖动时高亮
        borderColor.a = 255;
        SDL_Color highlightBg = Gfx::COLOR_ACCENT;
        highlightBg.a = 80;
        bgColor = highlightBg;
        iconColor = Gfx::COLOR_WHITE;
    } else {
        borderColor.a = 180;
    }
    
    // 计算左上角坐标（从中心点）
    int x = sBackButtonX - radius;
    int y = sBackButtonY - radius;
    
    // 绘制圆形背景（使用圆角矩形）
    Gfx::DrawRectRounded(x, y, size, size, radius, bgColor);
    Gfx::DrawRectRoundedOutline(x, y, size, size, radius, 2, borderColor);
    
    // 绘制返回图标 (左箭头)
    Gfx::DrawIcon(sBackButtonX, sBackButtonY, iconSize, iconColor, 0xf060, Gfx::ALIGN_CENTER | Gfx::ALIGN_VERTICAL);
}

bool Screen::UpdateBackButton(const Input &input) {
    const int radius = 35;
    const int holdThreshold = 15;  // 长按15帧(约0.25秒)后开始拖动
    
    if (input.data.touched && input.data.validPointer) {
        // 计算触摸坐标
        int touchX = (int)((input.data.x * 1920.0f / 1280.0f) + 960);
        int touchY = (int)(540 - (input.data.y * 1080.0f / 720.0f));
        
        // 计算到按钮中心的距离
        int dx = touchX - sBackButtonX;
        int dy = touchY - sBackButtonY;
        int distSq = dx * dx + dy * dy;
        bool inButton = distSq <= (radius * radius);
        
        if (!input.lastData.touched) {
            // 新触摸
            if (inButton) {
                sButtonPressed = true;  // 标记按钮被按下
                sHoldFrames = 1;
                sDragStartX = touchX;
                sDragStartY = touchY;
                sDragOffsetX = touchX - sBackButtonX;
                sDragOffsetY = touchY - sBackButtonY;
            } else {
                sButtonPressed = false;
                sHoldFrames = 0;
            }
        } else {
            // 持续触摸
            if (sButtonPressed) {
                sHoldFrames++;
                
                // 长按后开始拖动
                if (sHoldFrames >= holdThreshold) {
                    sBackButtonDragging = true;
                    sBackButtonX = touchX - sDragOffsetX;
                    sBackButtonY = touchY - sDragOffsetY;
                    
                    // 限制在屏幕范围内
                    if (sBackButtonX < radius) sBackButtonX = radius;
                    if (sBackButtonX > (int)Gfx::SCREEN_WIDTH - radius) sBackButtonX = (int)Gfx::SCREEN_WIDTH - radius;
                    if (sBackButtonY < radius) sBackButtonY = radius;
                    if (sBackButtonY > (int)Gfx::SCREEN_HEIGHT - radius) sBackButtonY = (int)Gfx::SCREEN_HEIGHT - radius;
                }
            }
        }
    } else {
        // 触摸结束
        if (sButtonPressed) {
            if (sBackButtonDragging) {
                // 拖动结束，不触发返回
                sBackButtonDragging = false;
                sButtonPressed = false;
                sHoldFrames = 0;
                return false;
            } else {
                // 没有进入拖动状态，触发返回（不管按了多久）
                sButtonPressed = false;
                sHoldFrames = 0;
                return true;
            }
        }
        // 重置状态
        sButtonPressed = false;
        sBackButtonDragging = false;
        sHoldFrames = 0;
    }
    
    return false;
}
