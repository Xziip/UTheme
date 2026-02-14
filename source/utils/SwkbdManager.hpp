#pragma once

#include <nn/swkbd.h>
#include <coreinit/filesystem.h>
#include <string>

class SwkbdManager {
public:
    static SwkbdManager& GetInstance();
    
    // 初始化和清理（程序启动/退出时调用）
    bool Init();
    void Shutdown();
    
    // 显示键盘并获取输入（阻塞式）
    // 返回 true 表示用户确认，false 表示取消
    bool ShowKeyboard(std::string& outText, const std::string& hintText = "", const std::string& initialText = "", int maxLength = 128);
    
    bool IsInitialized() const { return mInitialized; }
    
private:
    SwkbdManager() = default;
    ~SwkbdManager() = default;
    SwkbdManager(const SwkbdManager&) = delete;
    SwkbdManager& operator=(const SwkbdManager&) = delete;
    
    FSClient mFsClient;
    void* mWorkMemory = nullptr;
    bool mInitialized = false;
};
