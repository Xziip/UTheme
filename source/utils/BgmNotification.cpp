#include "BgmNotification.hpp"
#include "../Gfx.hpp"
#include "../utils/LanguageManager.hpp"
#include <coreinit/time.h>

BgmNotification::BgmNotification() {
}

void BgmNotification::ShowNowPlaying(const std::string& musicName) {
    ShowNowPlaying(musicName, "");
}

void BgmNotification::ShowNowPlaying(const std::string& musicName, const std::string& artist) {
    AddNotification(TYPE_MUSIC, musicName, artist);
}

void BgmNotification::ShowError(const std::string& message) {
    AddNotification(TYPE_ERROR, message);
}

void BgmNotification::ShowInfo(const std::string& message) {
    AddNotification(TYPE_INFO, message);
}

void BgmNotification::AddNotification(NotificationType type, const std::string& text1, const std::string& text2) {
    Notification notif;
    notif.type = type;
    notif.showTime = OSGetTime();
    notif.displayDuration = 4000; // 默认4秒
    
    if (type == TYPE_MUSIC) {
        notif.musicName = text1;
        notif.artist = text2;
    } else {
        notif.message = text1;
    }
    
    // 计算Y位置 - 从上往下堆叠
    int baseY = 140; // 基础Y坐标
    if (!mNotifications.empty()) {
        // 找到最底部的通知
        int maxY = baseY;
        for (const auto& n : mNotifications) {
            if (n.yPosition + NOTIFICATION_HEIGHT + NOTIFICATION_SPACING > maxY) {
                maxY = n.yPosition + NOTIFICATION_HEIGHT + NOTIFICATION_SPACING;
            }
        }
        notif.yPosition = maxY;
    } else {
        notif.yPosition = baseY;
    }
    
    // 设置动画 - 从右侧滑入
    notif.slideAnim.SetImmediate(1.0f); // 从屏幕外开始
    notif.slideAnim.SetTarget(0.0f, 400); // 滑入
    
    notif.fadeAnim.SetImmediate(0.0f);
    notif.fadeAnim.SetTarget(1.0f, 300);
    
    mNotifications.push_back(notif);
}

void BgmNotification::Hide() {
    for (auto& notif : mNotifications) {
        if (notif.fadeAnim.GetTarget() > 0.0f) {
            notif.fadeAnim.SetTarget(0.0f, 300);
            notif.slideAnim.SetTarget(1.0f, 400);
        }
    }
}

void BgmNotification::Update() {
    if (mNotifications.empty()) return;
    
    uint64_t currentTime = OSGetTime();
    uint64_t ticksPerSecond = OSGetSystemInfo()->busClockSpeed / 4;
    
    // 更新所有通知
    for (auto it = mNotifications.begin(); it != mNotifications.end(); ) {
        it->fadeAnim.Update();
        it->slideAnim.Update();
        
        // 检查是否已显示足够长时间
        uint64_t elapsedMs = ((currentTime - it->showTime) * 1000) / ticksPerSecond;
        if (elapsedMs > it->displayDuration && it->fadeAnim.GetTarget() > 0.0f) {
            // 开始淡出动画
            it->fadeAnim.SetTarget(0.0f, 300);
            it->slideAnim.SetTarget(1.0f, 400);
        }
        
        // 如果完全淡出,移除通知
        if (it->fadeAnim.GetValue() <= 0.0f && it->fadeAnim.GetTarget() <= 0.0f) {
            it = mNotifications.erase(it);
        } else {
            ++it;
        }
    }
}

void BgmNotification::Draw() {
    if (mNotifications.empty()) return;
    
    for (const auto& notif : mNotifications) {
        float fadeAlpha = notif.fadeAnim.GetValue();
        float slideOffset = notif.slideAnim.GetValue();
        
        if (fadeAlpha <= 0.0f) continue;
        
        // 计算位置 - 从右侧滑入
        int x = Gfx::SCREEN_WIDTH - NOTIFICATION_WIDTH - 40 + (int)(slideOffset * (NOTIFICATION_WIDTH + 100));
        int y = notif.yPosition;
        
        DrawNotification(notif, x, y, fadeAlpha, slideOffset);
    }
}

void BgmNotification::DrawNotification(const Notification& notif, int x, int y, float fadeAlpha, float slideOffset) {
    // 绘制阴影
    SDL_Color shadowColor = Gfx::COLOR_SHADOW;
    shadowColor.a = (Uint8)(100 * fadeAlpha);
    Gfx::DrawRectRounded(x + 5, y + 5, NOTIFICATION_WIDTH, NOTIFICATION_HEIGHT, 16, shadowColor);
    
    // 绘制背景
    SDL_Color bgColor;
    if (notif.type == TYPE_ERROR) {
        bgColor = SDL_Color{50, 20, 20, (Uint8)(240 * fadeAlpha)};
    } else if (notif.type == TYPE_INFO) {
        bgColor = SDL_Color{20, 40, 50, (Uint8)(240 * fadeAlpha)};  // 蓝色调的提示背景
    } else {
        bgColor = SDL_Color{30, 35, 50, (Uint8)(240 * fadeAlpha)};
    }
    Gfx::DrawRectRounded(x, y, NOTIFICATION_WIDTH, NOTIFICATION_HEIGHT, 16, bgColor);
    
    // 绘制左侧装饰条
    SDL_Color accentColor;
    if (notif.type == TYPE_ERROR) {
        accentColor = Gfx::COLOR_ERROR;
    } else if (notif.type == TYPE_INFO) {
        accentColor = Gfx::COLOR_SUCCESS;  // 绿色调的提示条
    } else {
        accentColor = Gfx::COLOR_ACCENT;
    }
    accentColor.a = (Uint8)(255 * fadeAlpha);
    Gfx::DrawRectRounded(x, y, 6, NOTIFICATION_HEIGHT, 12, accentColor);
    
    if (notif.type == TYPE_ERROR) {
        // 错误模式 - 显示错误图标和消息
        SDL_Color iconColor = Gfx::COLOR_ERROR;
        iconColor.a = (Uint8)(255 * fadeAlpha);
        Gfx::DrawIcon(x + 35, y + NOTIFICATION_HEIGHT / 2, 36, iconColor, 0xf06a, Gfx::ALIGN_CENTER);
        
        SDL_Color textColor = Gfx::COLOR_TEXT;
        textColor.a = (Uint8)(255 * fadeAlpha);
        Gfx::Print(x + 70, y + 30, 28, textColor, "BGM Error", Gfx::ALIGN_VERTICAL);
        
        SDL_Color msgColor = Gfx::COLOR_ALT_TEXT;
        msgColor.a = (Uint8)(220 * fadeAlpha);
        Gfx::Print(x + 70, y + 60, 24, msgColor, notif.message.c_str(), Gfx::ALIGN_VERTICAL);
    } else if (notif.type == TYPE_INFO) {
        // 提示模式 - 显示信息图标和消息
        SDL_Color iconColor = Gfx::COLOR_SUCCESS;
        iconColor.a = (Uint8)(255 * fadeAlpha);
        Gfx::DrawIcon(x + 35, y + NOTIFICATION_HEIGHT / 2, 36, iconColor, 0xf05a, Gfx::ALIGN_CENTER);  // info-circle
        
        SDL_Color msgColor = Gfx::COLOR_TEXT;
        msgColor.a = (Uint8)(255 * fadeAlpha);
        Gfx::Print(x + 70, y + NOTIFICATION_HEIGHT / 2, 26, msgColor, notif.message.c_str(), Gfx::ALIGN_VERTICAL);
    } else {
        // 音乐模式 - 显示音乐图标和名称
        SDL_Color iconColor = Gfx::COLOR_ACCENT;
        iconColor.a = (Uint8)(255 * fadeAlpha);
        Gfx::DrawIcon(x + 35, y + NOTIFICATION_HEIGHT / 2, 40, iconColor, 0xf001, Gfx::ALIGN_CENTER);
        
        // "Now Playing" 标签
        SDL_Color labelColor = Gfx::COLOR_ALT_TEXT;
        labelColor.a = (Uint8)(200 * fadeAlpha);
        Gfx::Print(x + 70, y + 18, 22, labelColor, "Now Playing", Gfx::ALIGN_VERTICAL);
        
        // 音乐名称
        SDL_Color nameColor = Gfx::COLOR_TEXT;
        nameColor.a = (Uint8)(255 * fadeAlpha);
        Gfx::Print(x + 70, y + 45, 28, nameColor, notif.musicName.c_str(), Gfx::ALIGN_VERTICAL);
        
        // 艺术家(如果有)
        if (!notif.artist.empty()) {
            SDL_Color artistColor = Gfx::COLOR_ALT_TEXT;
            artistColor.a = (Uint8)(200 * fadeAlpha);
            Gfx::Print(x + 70, y + 70, 22, artistColor, notif.artist.c_str(), Gfx::ALIGN_VERTICAL);
        }
    }
}
