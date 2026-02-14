#pragma once

#include "../utils/Animation.hpp"
#include <string>
#include <vector>
#include <SDL2/SDL.h>

// BGM播放通知组件 - 支持多个通知同时显示
class BgmNotification {
public:
    BgmNotification();
    
    // 显示当前播放的音乐
    void ShowNowPlaying(const std::string& musicName);
    
    // 显示当前播放的音乐(带艺术家信息)
    void ShowNowPlaying(const std::string& musicName, const std::string& artist);
    
    // 显示错误消息
    void ShowError(const std::string& message, uint64_t displayMs = 5000, const std::string& title = "");
    
    // 显示警告消息
    void ShowWarning(const std::string& message, uint64_t displayMs = 5000, const std::string& title = "");
    
    // 显示提示消息
    void ShowInfo(const std::string& message, uint64_t displayMs = 5000, const std::string& title = "");
    
    // 更新和绘制
    void Update();
    void Draw();
    
    // 状态查询
    bool IsVisible() const { return !mNotifications.empty(); }
    
    // 立即隐藏所有通知
    void Hide();
    
private:
    enum NotificationType {
        TYPE_MUSIC,
        TYPE_ERROR,
        TYPE_WARNING,
        TYPE_INFO
    };
    
    struct Notification {
        NotificationType type;
        std::string title;       // 主标题（可选）
        std::string musicName;
        std::string artist;
        std::string message;
        Animation fadeAnim;
        Animation slideAnim;
        uint64_t showTime;
        uint64_t displayDuration;
        int yPosition;  // 在屏幕上的Y位置
    };
    
    std::vector<Notification> mNotifications;
    
    void AddNotification(NotificationType type, const std::string& text1, const std::string& text2 = "", uint64_t displayMs = 4000, const std::string& title = "");
    void DrawNotification(const Notification& notif, int x, int y, float fadeAlpha, float slideOffset);
    
    static const int NOTIFICATION_WIDTH = 500;
    static const int NOTIFICATION_HEIGHT = 90;
    static const int NOTIFICATION_SPACING = 10;  // 通知之间的间距
};
