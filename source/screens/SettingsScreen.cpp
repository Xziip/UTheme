#include "SettingsScreen.hpp"
#include "Gfx.hpp"
#include "../utils/LanguageManager.hpp"
#include "../utils/FileLogger.hpp"
#include "../utils/Config.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <functional>
#include <cstring>
#include <sysapp/launch.h>
#include <coreinit/time.h>

SettingsScreen::SettingsScreen()
    : mPrevSelectedItem(SETTINGS_LANGUAGE)
{
    mTitleAnim.Start(0, 1, 500);
    mSelectionAnim.Start(0, 1, 200);
    
    // 初始化所有选项动画进度
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        mItemAnimProgress[i] = 0.0f;
    }
    mItemAnimProgress[mSelectedItem] = 1.0f; // 当前选中项从完全显示开始
    
    // 找到当前语言在列表中的位置
    const auto& languages = Lang().GetAvailableLanguages();
    const std::string& currentLang = Lang().GetCurrentLanguage();
    
    const int itemsPerColumn = (static_cast<int>(languages.size()) + 1) / 2;
    
    // 初始化语言项动画进度数组
    mLanguageItemAnimProgress.resize(languages.size(), 0.0f);
    mLanguageCardBounds.resize(languages.size());  // 初始化边界数组
    
    for (size_t i = 0; i < languages.size(); i++) {
        if (languages[i].code == currentLang) {
            // 计算在双列布局中的位置
            mSelectedColumn = static_cast<int>(i) / itemsPerColumn;
            mSelectedLanguage = static_cast<int>(i) % itemsPerColumn;
            mPrevSelectedColumn = mSelectedColumn;
            mPrevSelectedLanguage = mSelectedLanguage;
            
            // 当前语言项从完全显示开始
            mLanguageItemAnimProgress[i] = 1.0f;
            break;
        }
    }
}

SettingsScreen::~SettingsScreen() {
}

void SettingsScreen::Draw() {
    mFrameCount++;
    
    // 更新选项动画
    mSelectionAnim.Update();
    
    // 更新每个选项的动画进度
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        if (i == mSelectedItem) {
            // 当前选中项:逐渐增加到1.0
            mItemAnimProgress[i] += (1.0f - mItemAnimProgress[i]) * 0.2f;
        } else {
            // 非选中项:逐渐减少到0.0
            mItemAnimProgress[i] *= 0.8f;
        }
    }
    
    // Draw gradient background
    Gfx::DrawGradientV(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_HEIGHT, 
                       Gfx::COLOR_BACKGROUND, Gfx::COLOR_ALT_BACKGROUND);
    
    DrawAnimatedTopBar(_("settings.title"), mTitleAnim, 0xf013);
    
    if (mLanguageDialogOpen) {
        DrawLanguageDialog();
        return;
    }
    
    // 设置项列表
    const int topBarHeight = 120;
    const int itemHeight = 120;
    const int itemSpacing = 20;
    const int listX = 200;
    const int listY = topBarHeight + 60;
    const int listW = Gfx::SCREEN_WIDTH - 400;
    
    // 获取当前语言名称
    const auto& languages = Lang().GetAvailableLanguages();
    std::string currentLanguageName = "Unknown";
    for (const auto& lang : languages) {
        if (lang.code == Lang().GetCurrentLanguage()) {
            currentLanguageName = lang.name;
            break;
        }
    } 
    
    // 绘制设置项
    int currentY = listY;
    
    // 语言设置 - 地球图标
    DrawSettingItem(listX, currentY, listW, 
                   _("settings.language"), 
                   _("settings.language_desc"), 
                   currentLanguageName,
                   mSelectedItem == SETTINGS_LANGUAGE,
                   mItemAnimProgress[SETTINGS_LANGUAGE],
                   0xf0ac);  // fa-globe
    currentY += itemHeight + itemSpacing;
    
    // 下载路径设置 - 文件夹图标
    DrawSettingItem(listX, currentY, listW, 
                   _("settings.download_path"), 
                   _("settings.download_path_desc"), 
                   "SD:/wiiu/themes/",
                   mSelectedItem == SETTINGS_DOWNLOAD_PATH,
                   mItemAnimProgress[SETTINGS_DOWNLOAD_PATH],
                   0xf07c);  // fa-folder-open
    currentY += itemHeight + itemSpacing;
    
    // 背景音乐设置 - 音乐图标
    DrawSettingItem(listX, currentY, listW, 
                   _("settings.bgm_enabled"), 
                   _("settings.bgm_enabled_desc"), 
                   Config::GetInstance().IsBgmEnabled() ? _("common.yes") : _("common.no"),
                   mSelectedItem == SETTINGS_BGM_ENABLED,
                   mItemAnimProgress[SETTINGS_BGM_ENABLED],
                   0xf001);  // fa-music
    currentY += itemHeight + itemSpacing;
    
    // 日志启用设置 - 文件图标
    DrawSettingItem(listX, currentY, listW, 
                   _("settings.logging"), 
                   _("settings.logging_desc"), 
                   Config::GetInstance().IsLoggingEnabled() ? _("common.yes") : _("common.no"),
                   mSelectedItem == SETTINGS_LOGGING_ENABLED,
                   mItemAnimProgress[SETTINGS_LOGGING_ENABLED],
                   0xf15c);  // fa-file-text
    currentY += itemHeight + itemSpacing;
    
    // 详细日志设置 - 列表图标
    DrawSettingItem(listX, currentY, listW, 
                   _("settings.verbose_logging"), 
                   _("settings.verbose_logging_desc"), 
                   Config::GetInstance().IsVerboseLogging() ? _("common.yes") : _("common.no"),
                   mSelectedItem == SETTINGS_LOGGING_VERBOSE,
                   mItemAnimProgress[SETTINGS_LOGGING_VERBOSE],
                   0xf0ae);  // fa-tasks
    currentY += itemHeight + itemSpacing;
    
    // 清除缓存设置 - 垃圾桶图标
    DrawSettingItem(listX, currentY, listW, 
                   _("settings.clear_cache"), 
                   _("settings.clear_cache_desc"), 
                   _("settings.press_to_clear"),
                   mSelectedItem == SETTINGS_CLEAR_CACHE,
                   mItemAnimProgress[SETTINGS_CLEAR_CACHE],
                   0xf1f8);  // fa-trash
    
    // 绘制返回按钮 (右上角)
    mBackButtonBounds = DrawBackButton(Gfx::SCREEN_WIDTH - 240, 140, mBackButtonHovered);
    
    DrawBottomBar(nullptr, (std::string("\ue044 ") + _("input.exit")).c_str(), (std::string("\ue001 ") + _("input.back")).c_str());
}

void SettingsScreen::DrawSettingItem(int x, int y, int w, const std::string& title, 
                                     const std::string& description, const std::string& value, 
                                     bool selected, float animProgress, uint16_t icon) {
    const int itemH = 120; // 匹配Draw()中的itemHeight
    
    // 使用动画进度计算缩放效果
    float scale = 1.0f + (animProgress * 0.03f); // 选中时放大3%
    int scaledW = (int)(w * scale);
    int scaledH = (int)(itemH * scale);
    int offsetX = (w - scaledW) / 2;
    int offsetY = (itemH - scaledH) / 2;
    
    int drawX = x + offsetX;
    int drawY = y + offsetY;
    
    // 绘制背景和选中效果
    SDL_Color bgColor = Gfx::COLOR_CARD_BG;
    if (selected) {
        // 根据动画进度插值背景色
        SDL_Color hoverColor = Gfx::COLOR_CARD_HOVER;
        bgColor.r = (Uint8)(bgColor.r + (hoverColor.r - bgColor.r) * animProgress);
        bgColor.g = (Uint8)(bgColor.g + (hoverColor.g - bgColor.g) * animProgress);
        bgColor.b = (Uint8)(bgColor.b + (hoverColor.b - bgColor.b) * animProgress);
        
        // 绘制阴影
        SDL_Color shadowColor = Gfx::COLOR_SHADOW;
        shadowColor.a = (Uint8)(100 * animProgress);
        Gfx::DrawRectRounded(drawX + 4, drawY + 4, scaledW, scaledH, 12, shadowColor);
        
        // 绘制边框
        SDL_Color borderColor = Gfx::COLOR_ACCENT;
        borderColor.a = (Uint8)(180 * animProgress);
        Gfx::DrawRectRoundedOutline(drawX - 2, drawY - 2, scaledW + 4, scaledH + 4, 14, 3, borderColor);
    }
    
    Gfx::DrawRectRounded(drawX, drawY, scaledW, scaledH, 12, bgColor);
    
    // 绘制图标(如果提供)
    int iconSize = 50;
    int iconX = drawX + 30;
    int textStartX = drawX + 40;  // 默认文本起始位置
    
    if (icon != 0) {
        SDL_Color iconColor = Gfx::COLOR_ICON;
        if (selected) {
            // 选中时图标颜色变为accent色
            SDL_Color accentColor = Gfx::COLOR_ACCENT;
            iconColor.r = (Uint8)(iconColor.r + (accentColor.r - iconColor.r) * animProgress);
            iconColor.g = (Uint8)(iconColor.g + (accentColor.g - iconColor.g) * animProgress);
            iconColor.b = (Uint8)(iconColor.b + (accentColor.b - iconColor.b) * animProgress);
        }
        Gfx::DrawIcon(iconX, drawY + scaledH/2, iconSize, iconColor, icon, Gfx::ALIGN_VERTICAL);
        textStartX = iconX + iconSize + 30;  // 有图标时文本往右移
    }
    
    // 绘制文本
    SDL_Color titleColor = Gfx::COLOR_TEXT;
    SDL_Color descColor = Gfx::COLOR_ALT_TEXT;
    SDL_Color valueColor = Gfx::COLOR_ICON;
    
    if (selected) {
        // 根据动画进度插值文本颜色
        SDL_Color whiteColor = Gfx::COLOR_WHITE;
        titleColor.r = (Uint8)(titleColor.r + (whiteColor.r - titleColor.r) * animProgress);
        titleColor.g = (Uint8)(titleColor.g + (whiteColor.g - titleColor.g) * animProgress);
        titleColor.b = (Uint8)(titleColor.b + (whiteColor.b - titleColor.b) * animProgress);
        
        SDL_Color accentColor = Gfx::COLOR_ACCENT;
        valueColor.r = (Uint8)(valueColor.r + (accentColor.r - valueColor.r) * animProgress);
        valueColor.g = (Uint8)(valueColor.g + (accentColor.g - valueColor.g) * animProgress);
        valueColor.b = (Uint8)(valueColor.b + (accentColor.b - valueColor.b) * animProgress);
    }
    
    int valueX = drawX + scaledW - 40;
    
    // 计算垂直居中位置 - 往下移动
    const int titleSize = 38;  // 从44减小到38
    const int descSize = 28;   // 从32减小到28
    const int titleHeight = Gfx::GetTextHeight(titleSize, title.c_str());
    const int descHeight = Gfx::GetTextHeight(descSize, description.c_str());
    const int totalTextHeight = titleHeight + descHeight + 8; // 8px间距
    const int textStartY = drawY + (scaledH - totalTextHeight) / 2 + 10; // 往下移动10px
    
    Gfx::Print(textStartX, textStartY, titleSize, titleColor, title, Gfx::ALIGN_VERTICAL);
    Gfx::Print(textStartX, textStartY + titleHeight + 8, descSize, descColor, description, Gfx::ALIGN_VERTICAL);
    
    // 计算值文本宽度,为箭头留出空间
    const int valueSize = 36;  // 从40减小到36
    int valueWidth = Gfx::GetTextWidth(valueSize, value.c_str());
    int arrowWidth = 28;       // 从32减小到28
    int spacing = 50; // 箭头和值文本之间的间距
    
    if (selected && animProgress > 0.1f) {
        // 箭头位置:在值文本左侧,带有动画淡入效果
        int arrowX = valueX - valueWidth - spacing;
        SDL_Color arrowColor = Gfx::COLOR_ACCENT;
        arrowColor.a = (Uint8)(255 * animProgress);
        Gfx::DrawIcon(arrowX, drawY + scaledH/2, arrowWidth, arrowColor, 0xf054, Gfx::ALIGN_VERTICAL);
    }
    
    Gfx::Print(valueX, drawY + scaledH/2, valueSize, valueColor, value, Gfx::ALIGN_VERTICAL | Gfx::ALIGN_RIGHT);
}

void SettingsScreen::DrawLanguageDialog() {
    // 更新语言项动画进度
    const auto& languages = Lang().GetAvailableLanguages();
    const int itemsPerColumn = (static_cast<int>(languages.size()) + 1) / 2;
    
    for (size_t i = 0; i < languages.size(); i++) {
        int column = static_cast<int>(i) / itemsPerColumn;
        int rowInColumn = static_cast<int>(i) % itemsPerColumn;
        bool isSelected = (column == mSelectedColumn && rowInColumn == mSelectedLanguage);
        
        if (isSelected) {
            // 当前选中项:逐渐增加到1.0
            mLanguageItemAnimProgress[i] += (1.0f - mLanguageItemAnimProgress[i]) * 0.2f;
        } else {
            // 非选中项:逐渐减少到0.0
            mLanguageItemAnimProgress[i] *= 0.8f;
        }
    }
    
    // 半透明遮罩
    SDL_Color overlay = {0, 0, 0, 180};
    Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_HEIGHT, overlay);
    
    // 对话框 - 更大更宽,减少底部空白
    const int dialogW = 1600;  // 从1400增加到1600
    const int dialogH = 800;   // 减少从900到800,减少底部空白
    const int dialogX = (Gfx::SCREEN_WIDTH - dialogW) / 2;
    const int dialogY = (Gfx::SCREEN_HEIGHT - dialogH) / 2;
    
    // 绘制对话框背景
    SDL_Color shadowColor = Gfx::COLOR_SHADOW;
    shadowColor.a = 120;
    Gfx::DrawRectRounded(dialogX + 8, dialogY + 8, dialogW, dialogH, 20, shadowColor);
    Gfx::DrawRectRounded(dialogX, dialogY, dialogW, dialogH, 20, Gfx::COLOR_CARD_BG);
    
    // 双列布局 - 移除标题,直接显示语言列表
    const int columnW = (dialogW - 120) / 2;  // 两列宽度
    const int columnSpacing = 40;  // 列间距
    const int leftColumnX = dialogX + 40;
    const int rightColumnX = leftColumnX + columnW + columnSpacing;
    const int itemH = 80;          // 从70增加到80
    const int listStartY = dialogY + 60;  // 从130减少到60,顶部留更少空间
    
    for (size_t i = 0; i < languages.size(); i++) {
        // 确定当前项在哪一列
        int column = static_cast<int>(i) / itemsPerColumn;
        int rowInColumn = static_cast<int>(i) % itemsPerColumn;
        
        int itemX = (column == 0) ? leftColumnX : rightColumnX;
        int itemY = listStartY + rowInColumn * itemH;
        
        // 修复选中判断:需要同时匹配列和行
        bool isSelected = (column == mSelectedColumn && rowInColumn == mSelectedLanguage);
        bool isCurrent = (languages[i].code == Lang().GetCurrentLanguage());
        
        // 获取动画进度
        float animProgress = mLanguageItemAnimProgress[i];
        
        // 使用动画进度计算缩放效果
        float scale = 1.0f + (animProgress * 0.02f); // 选中时放大2%
        int scaledW = (int)(columnW * scale);
        int scaledH = (int)((itemH - 10) * scale);
        int offsetX = (columnW - scaledW) / 2;
        int offsetY = ((itemH - 10) - scaledH) / 2;
        
        int drawX = itemX + offsetX;
        int drawY = itemY + 5 + offsetY;
        
        // 绘制选中背景和动画效果
        SDL_Color bgColor = Gfx::COLOR_CARD_BG;
        if (isSelected) {
            // 根据动画进度插值背景色
            SDL_Color selectBg = Gfx::COLOR_ACCENT;
            selectBg.a = (Uint8)(80 + 40 * animProgress);  // 80-120透明度
            
            // 绘制阴影
            SDL_Color shadowColor = Gfx::COLOR_SHADOW;
            shadowColor.a = (Uint8)(60 * animProgress);
            Gfx::DrawRectRounded(drawX + 3, drawY + 3, scaledW, scaledH, 8, shadowColor);
            
            // 绘制边框
            SDL_Color borderColor = Gfx::COLOR_ACCENT;
            borderColor.a = (Uint8)(180 * animProgress);
            Gfx::DrawRectRoundedOutline(drawX - 2, drawY - 2, scaledW + 4, scaledH + 4, 10, 2, borderColor);
            
            // 绘制背景
            Gfx::DrawRectRounded(drawX, drawY, scaledW, scaledH, 8, selectBg);
            
            // 绘制左侧指示条
            SDL_Color indicatorColor = Gfx::COLOR_ACCENT;
            indicatorColor.a = (Uint8)(255 * animProgress);
            Gfx::DrawRectRounded(drawX + 5, drawY + 10, 6, scaledH - 20, 3, indicatorColor);
        } else {
            // 未选中项只绘制基础背景
            Gfx::DrawRectRounded(drawX, drawY, scaledW, scaledH, 8, bgColor);
        }
        
        // 绘制语言名称 - 临时切换字体以正确显示各语言文字
        SDL_Color textColor = Gfx::COLOR_TEXT;
        if (isSelected) {
            // 根据动画进度插值文本颜色
            SDL_Color whiteColor = Gfx::COLOR_WHITE;
            textColor.r = (Uint8)(textColor.r + (whiteColor.r - textColor.r) * animProgress);
            textColor.g = (Uint8)(textColor.g + (whiteColor.g - textColor.g) * animProgress);
            textColor.b = (Uint8)(textColor.b + (whiteColor.b - textColor.b) * animProgress);
        }
        
        // 保存当前字体设置
        bool originalFontSetting = (languages[i].code == "zh-cn" || 
                                    languages[i].code == "zh-tw" || 
                                    languages[i].code == "ja-jp" || 
                                    languages[i].code == "ko-kr");
        
        // 临时切换到该语言需要的字体
        Gfx::SetUseLatinFont(!originalFontSetting);
        
        Gfx::Print(drawX + 30, drawY + scaledH/2, 38, textColor, 
                  languages[i].name, Gfx::ALIGN_VERTICAL);
        
        // 恢复到当前语言的字体设置
        bool currentNeedsCJK = (Lang().GetCurrentLanguage() == "zh-cn" || 
                                Lang().GetCurrentLanguage() == "zh-tw" || 
                                Lang().GetCurrentLanguage() == "ja-jp" || 
                                Lang().GetCurrentLanguage() == "ko-kr");
        Gfx::SetUseLatinFont(!currentNeedsCJK);
        
        // 绘制当前语言标记
        if (isCurrent) {
            SDL_Color checkColor = Gfx::COLOR_SUCCESS;
            if (isSelected) {
                // 选中时检查标记也有动画
                checkColor.a = (Uint8)(255 * (0.6f + 0.4f * animProgress));
            }
            Gfx::DrawIcon(drawX + scaledW - 50, drawY + scaledH/2, 28, checkColor, 
                         0xf00c, Gfx::ALIGN_VERTICAL);
        }
        
        // 保存边界用于触摸检测
        mLanguageCardBounds[i] = {drawX, drawY, scaledW, scaledH};
    }
}

bool SettingsScreen::Update(Input &input) {
    // 检查是否正在等待退出
    if (mWaitingForExit) {
        uint64_t currentTime = OSGetSystemTime();
        uint64_t elapsed = OSTicksToMilliseconds(currentTime - mExitStartTime);
        
        // 等待2秒后退出
        if (elapsed >= 2000) {
            SYSLaunchMenu();
            return false;
        }
        
        // 在等待期间继续返回 true 保持渲染
        return true;
    }
    
    if (mLanguageDialogOpen) {
        return UpdateLanguageDialog(input);
    }
    
    // 检测返回按钮触摸
    if (IsTouchOnBackButton(input, mBackButtonBounds)) {
        return false;  // 返回上一级
    }
    
    // 检测返回按钮悬停状态(用于高亮显示)
    if (input.data.touched && input.data.validPointer) {
        float scaleX = 1920.0f / 1280.0f;
        float scaleY = 1080.0f / 720.0f;
        int touchX = (Gfx::SCREEN_WIDTH / 2) + (int)(input.data.x * scaleX);
        int touchY = (Gfx::SCREEN_HEIGHT / 2) - (int)(input.data.y * scaleY);
        
        mBackButtonHovered = (touchX >= mBackButtonBounds.x && 
                             touchX <= mBackButtonBounds.x + mBackButtonBounds.w &&
                             touchY >= mBackButtonBounds.y && 
                             touchY <= mBackButtonBounds.y + mBackButtonBounds.h);
    } else {
        mBackButtonHovered = false;
    }
    
    // 按B键返回
    if (input.data.buttons_d & Input::BUTTON_B) {
        return false;
    }
    
    // 检测上下按键(D-Pad + 摇杆)
    bool upPressed = (input.data.buttons_d & Input::BUTTON_UP) || (input.data.buttons_d & Input::STICK_L_UP);
    bool downPressed = (input.data.buttons_d & Input::BUTTON_DOWN) || (input.data.buttons_d & Input::STICK_L_DOWN);
    bool upHeld = (input.data.buttons_h & Input::BUTTON_UP) || (input.data.buttons_h & Input::STICK_L_UP);
    bool downHeld = (input.data.buttons_h & Input::BUTTON_DOWN) || (input.data.buttons_h & Input::STICK_L_DOWN);
    
    // 上下选择设置项(支持循环选择)
    bool shouldMoveUp = false;
    bool shouldMoveDown = false;
    
    if (upPressed) {
        shouldMoveUp = true;
        mHoldFrames = 0;
    } else if (upHeld) {
        mHoldFrames++;
        if (mHoldFrames >= mRepeatDelay) {
            if ((mHoldFrames - mRepeatDelay) % mRepeatRate == 0) {
                shouldMoveUp = true;
            }
        }
    } else if (downPressed) {
        shouldMoveDown = true;
        mHoldFrames = 0;
    } else if (downHeld) {
        mHoldFrames++;
        if (mHoldFrames >= mRepeatDelay) {
            if ((mHoldFrames - mRepeatDelay) % mRepeatRate == 0) {
                shouldMoveDown = true;
            }
        }
    } else {
        mHoldFrames = 0;
    }
    
    if (shouldMoveUp) {
        mSelectedItem = (mSelectedItem - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
    } else if (shouldMoveDown) {
        mSelectedItem = (mSelectedItem + 1) % SETTINGS_COUNT;
    }
    
    // A键进入设置项
    if (input.data.buttons_d & Input::BUTTON_A) {
        switch (mSelectedItem) {
            case SETTINGS_LANGUAGE:
                mLanguageDialogOpen = true;
                break;
            case SETTINGS_DOWNLOAD_PATH:
                // TODO: 打开路径设置对话框
                break;
            case SETTINGS_BGM_ENABLED:
                // 切换背景音乐
                {
                    bool newState = !Config::GetInstance().IsBgmEnabled();
                    Config::GetInstance().SetBgmEnabled(newState);
                    // MusicPlayer会在下一帧检查配置并更新状态
                }
                break;
            case SETTINGS_LOGGING_ENABLED:
                // 切换日志启用状态
                {
                    bool newState = !Config::GetInstance().IsLoggingEnabled();
                    Config::GetInstance().SetLoggingEnabled(newState);
                    FileLogger::GetInstance().SetEnabled(newState);
                    if (newState) {
                        FileLogger::GetInstance().StartLog();
                    }
                }
                break;
            case SETTINGS_LOGGING_VERBOSE:
                // 切换详细日志模式
                {
                    bool newState = !Config::GetInstance().IsVerboseLogging();
                    Config::GetInstance().SetVerboseLogging(newState);
                    FileLogger::GetInstance().SetVerbose(newState);
                }
                break;
            case SETTINGS_CLEAR_CACHE:
                // 清除缓存
                {
                    ClearCache();
                }
                break;
        }
    }
    
    return true;
}

bool SettingsScreen::UpdateLanguageDialog(Input &input) {
    const auto& languages = Lang().GetAvailableLanguages();
    
    // 按B键关闭对话框
    if (input.data.buttons_d & Input::BUTTON_B) {
        mLanguageDialogOpen = false;
        mDialogHoldFrames = 0;  // 重置计数器
        return true;
    }
    
    const int itemsPerColumn = (static_cast<int>(languages.size()) + 1) / 2;
    
    // 触摸处理
    if (input.data.touched && input.data.validPointer) {
        float scaleX = 1920.0f / 1280.0f;
        float scaleY = 1080.0f / 720.0f;
        int touchX = (Gfx::SCREEN_WIDTH / 2) + (int)(input.data.x * scaleX);
        int touchY = (Gfx::SCREEN_HEIGHT / 2) - (int)(input.data.y * scaleY);
        
        // 检测点击哪个语言卡片
        for (size_t i = 0; i < languages.size(); i++) {
            const auto& bounds = mLanguageCardBounds[i];
            if (touchX >= bounds.x && touchX <= bounds.x + bounds.w &&
                touchY >= bounds.y && touchY <= bounds.y + bounds.h) {
                if (!mTouchStartedOnLanguageCard) {
                    int column = static_cast<int>(i) / itemsPerColumn;
                    int rowInColumn = static_cast<int>(i) % itemsPerColumn;
                    
                    if (column == mSelectedColumn && rowInColumn == mSelectedLanguage) {
                        // 双击效果: 点击选中项 = 确认
                        std::string newLang = languages[i].code;
                        Lang().SetCurrentLanguage(newLang);
                        mLanguageDialogOpen = false;
                    } else {
                        // 切换选中项
                        mSelectedColumn = column;
                        mSelectedLanguage = rowInColumn;
                    }
                    mTouchStartedOnLanguageCard = true;
                }
                break;
            }
        }
    } else {
        mTouchStartedOnLanguageCard = false;
    }
    
    // 检测上下左右按键(D-Pad + 摇杆)
    bool upPressed = (input.data.buttons_d & Input::BUTTON_UP) || (input.data.buttons_d & Input::STICK_L_UP);
    bool downPressed = (input.data.buttons_d & Input::BUTTON_DOWN) || (input.data.buttons_d & Input::STICK_L_DOWN);
    bool leftPressed = (input.data.buttons_d & Input::BUTTON_LEFT) || (input.data.buttons_d & Input::STICK_L_LEFT);
    bool rightPressed = (input.data.buttons_d & Input::BUTTON_RIGHT) || (input.data.buttons_d & Input::STICK_L_RIGHT);
    bool upHeld = (input.data.buttons_h & Input::BUTTON_UP) || (input.data.buttons_h & Input::STICK_L_UP);
    bool downHeld = (input.data.buttons_h & Input::BUTTON_DOWN) || (input.data.buttons_h & Input::STICK_L_DOWN);
    
    // 上下选择语言
    bool shouldMoveUp = false;
    bool shouldMoveDown = false;
    
    if (upPressed) {
        shouldMoveUp = true;
        mDialogHoldFrames = 0;
    } else if (upHeld) {
        mDialogHoldFrames++;
        if (mDialogHoldFrames >= mRepeatDelay) {
            if ((mDialogHoldFrames - mRepeatDelay) % mRepeatRate == 0) {
                shouldMoveUp = true;
            }
        }
    } else if (downPressed) {
        shouldMoveDown = true;
        mDialogHoldFrames = 0;
    } else if (downHeld) {
        mDialogHoldFrames++;
        if (mDialogHoldFrames >= mRepeatDelay) {
            if ((mDialogHoldFrames - mRepeatDelay) % mRepeatRate == 0) {
                shouldMoveDown = true;
            }
        }
    } else {
        mDialogHoldFrames = 0;
    }
    
    // 上下移动
    if (shouldMoveUp) {
        if (mSelectedLanguage > 0) {
            mSelectedLanguage--;
        } else if (mSelectedColumn == 1) {
            // 在右列顶部,移到左列底部
            mSelectedColumn = 0;
            mSelectedLanguage = itemsPerColumn - 1;
        }
    } else if (shouldMoveDown) {
        int currentColumnItems = (mSelectedColumn == 0) ? itemsPerColumn : 
                                  static_cast<int>(languages.size()) - itemsPerColumn;
        if (mSelectedLanguage < currentColumnItems - 1) {
            mSelectedLanguage++;
        } else if (mSelectedColumn == 0 && itemsPerColumn < static_cast<int>(languages.size())) {
            // 在左列底部,移到右列顶部
            mSelectedColumn = 1;
            mSelectedLanguage = 0;
        }
    }
    
    // 左右切换列
    if (leftPressed && mSelectedColumn == 1) {
        mSelectedColumn = 0;
        // 确保索引有效
        if (mSelectedLanguage >= itemsPerColumn) {
            mSelectedLanguage = itemsPerColumn - 1;
        }
        mDialogHoldFrames = 0;
    } else if (rightPressed && mSelectedColumn == 0) {
        int rightColumnItems = static_cast<int>(languages.size()) - itemsPerColumn;
        if (rightColumnItems > 0) {
            mSelectedColumn = 1;
            // 确保索引有效
            if (mSelectedLanguage >= rightColumnItems) {
                mSelectedLanguage = rightColumnItems - 1;
            }
        }
        mDialogHoldFrames = 0;
    }
    
    // A键确认选择语言
    if (input.data.buttons_d & Input::BUTTON_A) {
        int selectedIndex = mSelectedColumn * itemsPerColumn + mSelectedLanguage;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(languages.size())) {
            std::string newLang = languages[selectedIndex].code;
            // SetCurrentLanguage内部会自动保存到Config
            Lang().SetCurrentLanguage(newLang);
            mLanguageDialogOpen = false;
        }
    }
    
    return true;
}

void SettingsScreen::ClearCache() {
    FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Starting cache clear operation");
    
    // 不区分大小写比较字符串的辅助函数
    auto strcasecmp_custom = [](const char* s1, const char* s2) -> bool {
        while (*s1 && *s2) {
            if (tolower(*s1) != tolower(*s2)) return false;
            s1++;
            s2++;
        }
        return *s1 == *s2;
    };
    
    // 递归删除目录的辅助函数
    std::function<bool(const std::string&)> removeDirectory = [&](const std::string& path) -> bool {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            FileLogger::GetInstance().LogError("[CLEAR CACHE] Failed to open directory: %s", path.c_str());
            return false;
        }
        
        FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Processing directory: %s", path.c_str());
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string fullPath = path + "/" + entry->d_name;
            
            if (entry->d_type == DT_DIR) {
                // 递归删除子目录
                removeDirectory(fullPath);
            } else {
                // 删除文件
                FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Deleting file: %s", fullPath.c_str());
                if (remove(fullPath.c_str()) != 0) {
                    FileLogger::GetInstance().LogError("[CLEAR CACHE] Failed to delete file: %s", fullPath.c_str());
                }
            }
        }
        closedir(dir);
        
        // 删除目录本身
        FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Removing directory: %s", path.c_str());
        bool result = rmdir(path.c_str()) == 0;
        if (!result) {
            FileLogger::GetInstance().LogError("[CLEAR CACHE] Failed to remove directory: %s", path.c_str());
        }
        return result;
    };
    
    // 删除 SD:/utheme 文件夹(扫描并不区分大小写匹配)
    FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Scanning SD root for utheme folders");
    DIR* sdRoot = opendir("fs:/vol/external01");
    if (sdRoot) {
        struct dirent* entry;
        while ((entry = readdir(sdRoot)) != nullptr) {
            if (strcasecmp_custom(entry->d_name, "utheme")) {
                std::string fullPath = std::string("fs:/vol/external01/") + entry->d_name;
                FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Found utheme folder: %s", fullPath.c_str());
                struct stat st;
                if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                    removeDirectory(fullPath);
                }
            }
        }
        closedir(sdRoot);
    } else {
        FileLogger::GetInstance().LogError("[CLEAR CACHE] Failed to open SD root directory");
    }
    
    // 删除配置文件 SD:/wiiu/utheme.cfg(扫描并不区分大小写匹配)
    FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Scanning wiiu folder for config files");
    DIR* wiiuDir = opendir("fs:/vol/external01/wiiu");
    if (wiiuDir) {
        struct dirent* entry;
        while ((entry = readdir(wiiuDir)) != nullptr) {
            if (strcasecmp_custom(entry->d_name, "utheme.cfg")) {
                std::string fullPath = std::string("fs:/vol/external01/wiiu/") + entry->d_name;
                FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Deleting config file: %s", fullPath.c_str());
                if (remove(fullPath.c_str()) != 0) {
                    FileLogger::GetInstance().LogError("[CLEAR CACHE] Failed to delete config file: %s", fullPath.c_str());
                }
            }
        }
        closedir(wiiuDir);
    } else {
        FileLogger::GetInstance().LogError("[CLEAR CACHE] Failed to open wiiu directory");
    }
    
    FileLogger::GetInstance().LogInfo("[CLEAR CACHE] Cache clear operation completed");
    
    // 显示两条通知
    Screen::GetBgmNotification().ShowInfo(_("settings.cache_cleared"));
    Screen::GetBgmNotification().ShowInfo(_("settings.restarting_app"));
    
    // 启动退出计时器
    mWaitingForExit = true;
    mExitStartTime = OSGetSystemTime();
}
