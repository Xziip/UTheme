#pragma once

#include <string>
#include <vector>

// 远程通知抓取器 - 从 GitHub 获取通知消息
class RemoteNotification {
public:
    static RemoteNotification& GetInstance();
    
    // 启动时检查通知（异步，不阻塞）
    void CheckOnStartup();
    
    // 手动刷新
    void Refresh();
    
private:
    RemoteNotification();
    ~RemoteNotification() = default;
    
    RemoteNotification(const RemoteNotification&) = delete;
    RemoteNotification& operator=(const RemoteNotification&) = delete;
    
    void FetchAndShow();
    
    struct NotificationCache {
        std::string id;
        std::string version;  // 版本号（仅 update 类型）
        int displayCount;     // 已显示次数
    };
    
    bool ShouldShowMessage(const std::string& id, const std::string& type, 
                           const std::string& version, int maxDisplayCount);
    void MarkAsShown(const std::string& id, const std::string& version);
    std::vector<NotificationCache> LoadCache();
    void SaveCache(const std::vector<NotificationCache>& cache);
    
    static const char* NOTIFICATION_URL;
    static const char* CACHE_FILE;
};
