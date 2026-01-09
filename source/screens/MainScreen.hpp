#pragma once

#include "Gfx.hpp"
#include "Screen.hpp"
#include "../utils/Animation.hpp"
#include "../utils/LanguageManager.hpp"
#include <memory>

class MainScreen : public Screen {
public:
    MainScreen() {
        mSpinnerAnim.SetImmediate(0.0f);
        mLoadingAnim.SetImmediate(0.0f);
        mLoadingAnim.SetTarget(1.0f, 800);
        
        // 初始化语言项动画进度数组
        const auto& languages = LanguageManager::getInstance().GetAvailableLanguages();
        mLanguageItemAnimProgress.resize(languages.size(), 0.0f);
        mLanguageCardBounds.resize(languages.size());  // 初始化边界数组
    }

    ~MainScreen() override;

    void Draw() override;

    bool Update(Input &input) override;
    
    // 静态方法检查Mocha是否可用
    static bool IsMochaAvailable() { return sMochaAvailable; }

protected:
    static void DrawStatus(std::string status, SDL_Color color = Gfx::COLOR_TEXT);
    void DrawLoadingSpinner(int x, int y, int size, float progress);

private:
    enum {
        STATE_INIT,
        STATE_LANGUAGE_SELECT,  // 新增:语言选择状态
        STATE_INIT_MOCHA,
        STATE_INIT_FS,
        STATE_LOAD_MENU,
        STATE_IN_MENU,
    } mState           = STATE_INIT;
    bool mStateFailure = false;
    
    // 语言选择
    int mSelectedLanguage = 0;
    int mSelectedColumn = 0;  // 0 = left, 1 = right
    std::vector<float> mLanguageItemAnimProgress;  // 每个语言项的动画进度
    
    // 卡片边界(用于触摸检测)
    struct CardBounds {
        int x, y, w, h;
    };
    std::vector<CardBounds> mLanguageCardBounds;  // 改为动态数组
    bool mTouchStartedOnLanguageCard = false;
    
    // 静态变量,全局标记Mocha是否可用
    static bool sMochaAvailable;

    std::unique_ptr<Screen> mMenuScreen;
    
    Animation mSpinnerAnim;
    Animation mLoadingAnim;
    uint64_t mFrameCount = 0;
    
    static bool mMochaAvailableGlobal;  // 全局标记
};
