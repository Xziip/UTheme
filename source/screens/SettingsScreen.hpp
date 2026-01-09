#pragma once
#include "Screen.hpp"
#include "../utils/LanguageManager.hpp"
#include "../utils/Animation.hpp"

class SettingsScreen : public Screen {
public:
    SettingsScreen();
    ~SettingsScreen() override;

    void Draw() override;
    bool Update(Input &input) override;

private:
    enum SettingsItem {
        SETTINGS_LANGUAGE,
        SETTINGS_DOWNLOAD_PATH,
        SETTINGS_BGM_ENABLED,
        SETTINGS_LOGGING_ENABLED,
        SETTINGS_LOGGING_VERBOSE,
        SETTINGS_CLEAR_CACHE,
        
        SETTINGS_COUNT
    };
    
    int mFrameCount = 0;
    int mSelectedItem = SETTINGS_LANGUAGE;
    bool mLanguageDialogOpen = false;
    int mSelectedLanguage = 0;
    int mSelectedColumn = 0;  // 0 = left column, 1 = right column
    int mPrevSelectedItem = SETTINGS_LANGUAGE; // 用于追踪上一个选中项
    Animation mTitleAnim;
    Animation mSelectionAnim; // 选项切换动画
    float mItemAnimProgress[SETTINGS_COUNT]; // 每个选项的动画进度
    
    // 长按连续选择
    int mHoldFrames = 0;
    int mRepeatDelay = 30;  // 初始延迟帧数 (约0.5秒)
    int mRepeatRate = 8;    // 重复间隔帧数 (约0.13秒)
    
    // 语言对话框的长按连续选择
    int mDialogHoldFrames = 0;
    
    // 语言对话框动画
    int mPrevSelectedLanguage = 0;  // 用于追踪上一个选中的语言
    int mPrevSelectedColumn = 0;    // 用于追踪上一个选中的列
    std::vector<float> mLanguageItemAnimProgress;  // 每个语言项的动画进度
    
    // 触屏支持
    struct CardBounds {
        int x, y, w, h;
    };
    std::vector<CardBounds> mLanguageCardBounds;  // 语言卡片边界
    bool mTouchStartedOnLanguageCard = false;
    BackButtonBounds mBackButtonBounds = {0, 0, 0, 0};  // 返回按钮边界
    bool mBackButtonHovered = false;  // 返回按钮悬停状态
    
    // 清除缓存退出计时器
    bool mWaitingForExit = false;
    uint64_t mExitStartTime = 0;
    
    void DrawSettingItem(int x, int y, int w, const std::string& title, 
                        const std::string& description, const std::string& value, 
                        bool selected, float animProgress, uint16_t icon = 0);
    void DrawLanguageDialog();
    bool UpdateLanguageDialog(Input &input);
    void ClearCache();
};
