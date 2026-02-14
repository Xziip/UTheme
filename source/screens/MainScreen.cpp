#include "MainScreen.hpp"
#include "Gfx.hpp"
#include "MenuScreen.hpp"
#include "common.h"
#include "../utils/LanguageManager.hpp"
#include "../utils/Config.hpp"
#include "../utils/PluginDownloader.hpp"
#include "../utils/FileLogger.hpp"
#include <mocha/mocha.h>
#include <utility>
#include <cmath>

bool MainScreen::sMochaAvailable = false;

// 旧的语言选项数组已不再使用,现在使用LanguageManager的完整语言列表

MainScreen::~MainScreen() {
    if (sMochaAvailable && mState > STATE_INIT_MOCHA) {
        Mocha_DeInitLibrary();
    }
}

void MainScreen::DrawLoadingSpinner(int x, int y, int size, float progress) {
    const int numDots = 8;
    const float radius = size / 2.0f;
    
    for (int i = 0; i < numDots; i++) {
        float angle = (progress * 2.0f * M_PI) + (i * 2.0f * M_PI / numDots);
        int dotX = x + (int)(cos(angle) * radius);
        int dotY = y + (int)(sin(angle) * radius);
        
        // Fade dots based on position
        float fade = (float)(numDots - i) / (float)numDots;
        SDL_Color dotColor = Gfx::COLOR_ACCENT;
        dotColor.a = (Uint8)(255 * fade);
        
        int dotSize = 8 + (int)(fade * 8);
        Gfx::DrawRectRounded(dotX - dotSize/2, dotY - dotSize/2, dotSize, dotSize, dotSize/2, dotColor);
    }
}

void MainScreen::Draw() {
    mFrameCount++;
    mLoadingAnim.Update();
    
    Gfx::Clear(Gfx::COLOR_BACKGROUND);

    if (mMenuScreen) {
        mMenuScreen->Draw();
        return;
    }

    // Draw gradient background
    Gfx::DrawGradientV(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_HEIGHT, 
                       Gfx::COLOR_BACKGROUND, Gfx::COLOR_ALT_BACKGROUND);

    // 使用简化的顶部栏绘制
    float loadProgress = mLoadingAnim.GetValue();
    
    // draw top bar
    Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, 120, Gfx::COLOR_BARS);
    
    int titleY = 25 - (int)((1.0f - loadProgress) * 50);
    SDL_Color titleColor = Gfx::COLOR_TEXT;
    titleColor.a = (Uint8)(255 * loadProgress);
    
    Gfx::DrawIcon(60, titleY + 40, 60, Gfx::COLOR_ACCENT, 0xf53f, Gfx::ALIGN_VERTICAL);
    Gfx::Print(140, titleY + 40, 56, titleColor, _("app_name"), Gfx::ALIGN_VERTICAL);
    
    // Draw animated accent line
    SDL_Color accentColor = Gfx::COLOR_ACCENT;
    accentColor.a = (Uint8)(180 * loadProgress);
    Gfx::DrawRectFilled(0, 115, (int)(Gfx::SCREEN_WIDTH * loadProgress), 5, accentColor);

    // Center loading card - 根据状态调整大小
    // 语言选择界面使用更大的尺寸以适应全屏显示
    int cardW = (mState == STATE_LANGUAGE_SELECT) ? 1600 : 800;
    int cardH = (mState == STATE_LANGUAGE_SELECT) ? 800 : 400;  // 减少高度从900到800
    int cardX = (Gfx::SCREEN_WIDTH - cardW) / 2;
    int cardY = (mState == STATE_LANGUAGE_SELECT) ? 
                ((Gfx::SCREEN_HEIGHT - cardH) / 2 + 80) : // 语言选择对话框往下移80px
                ((Gfx::SCREEN_HEIGHT - cardH) / 2);       // 其他状态居中
    
    // Draw card shadow and background - simple shadow for performance
    SDL_Color shadowColor = Gfx::COLOR_SHADOW;
    shadowColor.a = 80;
    Gfx::DrawRectRounded(cardX + 8, cardY + 8, cardW, cardH, 25, shadowColor);
    Gfx::DrawRectRounded(cardX, cardY, cardW, cardH, 25, Gfx::COLOR_CARD_BG);

    switch (mState) {
        case STATE_INIT:
            break;
        case STATE_LANGUAGE_SELECT: {
            // 更新语言项动画进度
            const auto& languages = LanguageManager::getInstance().GetAvailableLanguages();
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
            
            // 语言选择界面 - 双列布局,全屏尺寸
            // 双列布局
            const int columnW = (cardW - 160) / 2;  // 两列宽度,增加边距
            const int columnSpacing = 60;  // 列间距
            const int leftColumnX = cardX + 60;
            const int rightColumnX = leftColumnX + columnW + columnSpacing;
            const int itemH = 100;          // 增加到100以适应更大屏幕
            const int listStartY = cardY + 60;  // 顶部留60px边距
            
            for (size_t i = 0; i < languages.size(); i++) {
                // 确定当前项在哪一列
                int column = static_cast<int>(i) / itemsPerColumn;
                int rowInColumn = static_cast<int>(i) % itemsPerColumn;
                
                int itemX = (column == 0) ? leftColumnX : rightColumnX;
                int itemY = listStartY + rowInColumn * itemH;
                
                // 修复选中判断:需要同时匹配列和行
                bool isSelected = (column == mSelectedColumn && rowInColumn == mSelectedLanguage);
                
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
                
                // 临时切换字体
                bool needsCJKFont = (languages[i].code == "zh-cn" || 
                                     languages[i].code == "zh-tw" || 
                                     languages[i].code == "ja-jp" || 
                                     languages[i].code == "ko-kr");
                
                Gfx::SetUseLatinFont(!needsCJKFont);
                
                Gfx::Print(drawX + 40, drawY + scaledH/2, 48, textColor, 
                          languages[i].name, Gfx::ALIGN_VERTICAL);
                
                // 恢复字体设置 (默认使用CJK字体)
                Gfx::SetUseLatinFont(false);
                
                // 保存边界用于触摸检测
                mLanguageCardBounds[i] = {drawX, drawY, scaledW, scaledH};
            }
            
            break;
        }
        case STATE_INIT_MOCHA:
            if (mStateFailure) {
                Gfx::DrawIcon(cardX + cardW/2, cardY + 100, 80, Gfx::COLOR_WARNING, 0xf06a, Gfx::ALIGN_CENTER);
                Gfx::Print(cardX + cardW/2, cardY + 200, 48, Gfx::COLOR_WARNING, _("common.local_mode"), Gfx::ALIGN_CENTER);
                Gfx::Print(cardX + cardW/2, cardY + 260, 32, Gfx::COLOR_ALT_TEXT, 
                          _("common.mocha_unavailable"), Gfx::ALIGN_CENTER);
                Gfx::Print(cardX + cardW/2, cardY + 300, 28, Gfx::COLOR_ALT_TEXT, 
                          _("common.other_features_available"), Gfx::ALIGN_CENTER);
                          
                // 显示继续提示
                if ((mFrameCount / 30) % 2 == 0) {
                    Gfx::Print(cardX + cardW/2, cardY + 350, 32, Gfx::COLOR_ACCENT, 
                              _("common.press_a_continue"), Gfx::ALIGN_CENTER);
                }
            } else {
                DrawLoadingSpinner(cardX + cardW/2, cardY + 120, 80, (mFrameCount % 60) / 60.0f);
                Gfx::Print(cardX + cardW/2, cardY + 220, 44, Gfx::COLOR_TEXT, _("common.init_mocha"), Gfx::ALIGN_CENTER);
            }
            break;
        case STATE_INIT_FS:
            if (mStateFailure) {
                Gfx::DrawIcon(cardX + cardW/2, cardY + 100, 80, Gfx::COLOR_ERROR, 0xf071, Gfx::ALIGN_CENTER);
                Gfx::Print(cardX + cardW/2, cardY + 200, 48, Gfx::COLOR_ERROR, _("common.filesystem_error"), Gfx::ALIGN_CENTER);
            } else {
                DrawLoadingSpinner(cardX + cardW/2, cardY + 120, 80, (mFrameCount % 60) / 60.0f);
                Gfx::Print(cardX + cardW/2, cardY + 220, 44, Gfx::COLOR_TEXT, _("common.mount_filesystem"), Gfx::ALIGN_CENTER);
            }
            break;
        case STATE_CHECK_STYLEMIIU:
            // 检查 StyleMiiU 插件状态
            break;
        case STATE_LOAD_MENU:
            DrawLoadingSpinner(cardX + cardW/2, cardY + 120, 80, (mFrameCount % 60) / 60.0f);
            Gfx::Print(cardX + cardW/2, cardY + 220, 44, Gfx::COLOR_SUCCESS, _("common.load_complete"), Gfx::ALIGN_CENTER);
            break;
        case STATE_IN_MENU:
            break;
    }

    // Bottom bar
    if (mStateFailure) {
        Gfx::DrawRectFilled(0, Gfx::SCREEN_HEIGHT - 80, Gfx::SCREEN_WIDTH, 80, Gfx::COLOR_BARS);
        Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT - 40, 40, Gfx::COLOR_TEXT, 
                  (std::string("\ue044 ") + _("input.exit")).c_str(), Gfx::ALIGN_CENTER);
    }
}

bool MainScreen::Update(Input &input) {
    if (mMenuScreen) {
        return mMenuScreen->Update(input);
    }

    // 在语言选择状态下处理输入
    if (mState == STATE_LANGUAGE_SELECT) {
        const auto& languages = LanguageManager::getInstance().GetAvailableLanguages();
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
                            Config::GetInstance().SetLanguage(languages[i].code);
                            LanguageManager::getInstance().LoadLanguage(languages[i].code);
                            mState = STATE_INIT_MOCHA;
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
        
        // 上下左右导航
        if (input.data.buttons_d & Input::BUTTON_UP) {
            if (mSelectedLanguage > 0) {
                mSelectedLanguage--;
            } else if (mSelectedColumn == 1) {
                // 在右列顶部,移到左列底部
                mSelectedColumn = 0;
                mSelectedLanguage = itemsPerColumn - 1;
            }
        }
        if (input.data.buttons_d & Input::BUTTON_DOWN) {
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
        if (input.data.buttons_d & Input::BUTTON_LEFT) {
            if (mSelectedColumn == 1) {
                mSelectedColumn = 0;
                // 确保索引有效
                if (mSelectedLanguage >= itemsPerColumn) {
                    mSelectedLanguage = itemsPerColumn - 1;
                }
            }
        }
        if (input.data.buttons_d & Input::BUTTON_RIGHT) {
            if (mSelectedColumn == 0) {
                int rightColumnItems = static_cast<int>(languages.size()) - itemsPerColumn;
                if (rightColumnItems > 0) {
                    mSelectedColumn = 1;
                    // 确保索引有效
                    if (mSelectedLanguage >= rightColumnItems) {
                        mSelectedLanguage = rightColumnItems - 1;
                    }
                }
            }
        }
        
        // 确认选择
        if (input.data.buttons_d & Input::BUTTON_A) {
            // 计算全局索引
            int selectedIndex = mSelectedColumn * itemsPerColumn + mSelectedLanguage;
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(languages.size())) {
                // 保存语言设置
                Config::GetInstance().SetLanguage(languages[selectedIndex].code);
                LanguageManager::getInstance().LoadLanguage(languages[selectedIndex].code);
                
                // 进入Mocha初始化
                mState = STATE_INIT_MOCHA;
            }
        }
        
        return true;
    }

    // 在 Mocha 失败状态下,允许按 A 继续到本地模式
    if (mStateFailure) {
        if (mState == STATE_INIT_MOCHA && (input.data.buttons_d & Input::BUTTON_A)) {
            mStateFailure = false;
            mState = STATE_LOAD_MENU;  // 直接跳到加载菜单
        }
        return true;
    }

    switch (mState) {
        case STATE_INIT: {
            // 检查是否已保存语言设置
            std::string savedLang = Config::GetInstance().GetLanguage();
            if (!savedLang.empty()) {
                // 已有语言设置,直接使用
                LanguageManager::getInstance().LoadLanguage(savedLang);
                
                // 找到对应的索引(用于显示选中项)
                const auto& languages = LanguageManager::getInstance().GetAvailableLanguages();
                for (size_t i = 0; i < languages.size(); i++) {
                    if (savedLang == languages[i].code) {
                        const int itemsPerColumn = (static_cast<int>(languages.size()) + 1) / 2;
                        mSelectedColumn = static_cast<int>(i) / itemsPerColumn;
                        mSelectedLanguage = static_cast<int>(i) % itemsPerColumn;
                        break;
                    }
                }
                
                // 跳过语言选择,直接进入Mocha初始化
                mState = STATE_INIT_MOCHA;
            } else {
                // 第一次运行,显示语言选择
                mState = STATE_LANGUAGE_SELECT;
            }
            break;
        }
        case STATE_LANGUAGE_SELECT:
            // 语言选择在Update开头已处理
            break;
        case STATE_INIT_MOCHA: {
            MochaUtilsStatus status = Mocha_InitLibrary();
            if (status == MOCHA_RESULT_SUCCESS) {
                sMochaAvailable = true;
                mState = STATE_INIT_FS;
                break;
            }

            // Mocha不可用,但不阻止程序运行
            sMochaAvailable = false;
            mStateFailure = true;
            break;
        }
        case STATE_INIT_FS: {
            // 只有Mocha可用时才挂载
            if (sMochaAvailable) {
                auto res = Mocha_MountFS(MLC_STORAGE_PATH, "/dev/mlc01", "/vol/storage_mlc01");
                if (res == MOCHA_RESULT_ALREADY_EXISTS) {
                    res = Mocha_MountFS(MLC_STORAGE_PATH, nullptr, "/vol/storage_mlc01");
                }
                if (res == MOCHA_RESULT_SUCCESS) {
                    mState = STATE_CHECK_STYLEMIIU;
                    break;
                }

                mStateFailure = true;
                break;
            } else {
                // 没有Mocha,跳过StyleMiiU检查直接进入菜单
                mState = STATE_LOAD_MENU;
                break;
            }
        }
        case STATE_CHECK_STYLEMIIU:
            // 检查并下载 StyleMiiU 插件 (仅在 Mocha 可用时运行)
            if (sMochaAvailable) {
                FileLogger::GetInstance().LogInfo("Checking for StyleMiiU plugin...");
                PluginDownloader::GetInstance().CheckAndDownloadStyleMiiU();
            }
            mState = STATE_LOAD_MENU;
            break;
        case STATE_LOAD_MENU:
            mMenuScreen = std::make_unique<MenuScreen>();
            break;
        case STATE_IN_MENU:
            break;
    };

    return true;
}

void MainScreen::DrawStatus(std::string status, SDL_Color color) {
    Gfx::Print(Gfx::SCREEN_WIDTH / 2, Gfx::SCREEN_HEIGHT / 2, 64, color, std::move(status), Gfx::ALIGN_CENTER);
}
