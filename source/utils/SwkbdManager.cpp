#include "SwkbdManager.hpp"
#include "FileLogger.hpp"
#include "../Gfx.hpp"
#include <coreinit/memdefaultheap.h>
#include <vpad/input.h>
#include <padscore/kpad.h>
#include <locale>
#include <codecvt>
#include <cstring>

SwkbdManager& SwkbdManager::GetInstance() {
    static SwkbdManager instance;
    return instance;
}

bool SwkbdManager::Init() {
    if (mInitialized) {
        FileLogger::GetInstance().LogWarning("[SwkbdManager] Already initialized");
        return true;
    }
    
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Initializing");
    
    // 创建 FSClient
    if (FSAddClient(&mFsClient, FS_ERROR_FLAG_NONE) != FS_STATUS_OK) {
        FileLogger::GetInstance().LogError("[SwkbdManager] Failed to add FSClient");
        return false;
    }
    
    // 分配工作内存
    uint32_t workMemSize = nn::swkbd::GetWorkMemorySize(0);
    mWorkMemory = MEMAllocFromDefaultHeap(workMemSize);
    if (!mWorkMemory) {
        FileLogger::GetInstance().LogError("[SwkbdManager] Failed to allocate work memory");
        FSDelClient(&mFsClient, FS_ERROR_FLAG_NONE);
        return false;
    }
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Allocated %u bytes work memory", workMemSize);
    
    // 创建 swkbd
    nn::swkbd::CreateArg createArg = {};
    createArg.workMemory = mWorkMemory;
    createArg.regionType = nn::swkbd::RegionType::Europe;
    createArg.fsClient = &mFsClient;
    
    if (!nn::swkbd::Create(createArg)) {
        FileLogger::GetInstance().LogError("[SwkbdManager] Failed to create swkbd");
        MEMFreeToDefaultHeap(mWorkMemory);
        mWorkMemory = nullptr;
        FSDelClient(&mFsClient, FS_ERROR_FLAG_NONE);
        return false;
    }
    
    mInitialized = true;
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Initialized successfully");
    return true;
}

void SwkbdManager::Shutdown() {
    if (!mInitialized) {
        return;
    }
    
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Shutting down");
    
    // 禁用触摸屏传感器条
    VPADSetSensorBar(VPAD_CHAN_0, false);
    
    // 如果键盘还在显示，先隐藏
    if (nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden) {
        nn::swkbd::DisappearInputForm();
        // 等待淡出动画完成
        VPADStatus vpadStatus;
        nn::swkbd::ControllerInfo controllerInfo;
        while (nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden) {
            VPADRead(VPAD_CHAN_0, &vpadStatus, 1, nullptr);
            VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpadStatus.tpNormal, &vpadStatus.tpNormal);
            controllerInfo.vpad = &vpadStatus;
            
            if (nn::swkbd::IsNeedCalcSubThreadFont()) {
                nn::swkbd::CalcSubThreadFont();
            }
            nn::swkbd::Calc(controllerInfo);
            
            nn::swkbd::DrawDRC();
            nn::swkbd::DrawTV();
            Gfx::Render();
        }
    }
    
    nn::swkbd::Destroy();
    
    if (mWorkMemory) {
        MEMFreeToDefaultHeap(mWorkMemory);
        mWorkMemory = nullptr;
    }
    
    FSDelClient(&mFsClient, FS_ERROR_FLAG_NONE);
    mInitialized = false;
    
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Shutdown complete");
}

bool SwkbdManager::ShowKeyboard(std::string& outText, const std::string& hintText, const std::string& initialText, int maxLength) {
    if (!mInitialized) {
        FileLogger::GetInstance().LogError("[SwkbdManager] Not initialized");
        return false;
    }
    
    if (nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden) {
        FileLogger::GetInstance().LogWarning("[SwkbdManager] Keyboard already visible");
        return false;
    }
    
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Showing keyboard");
    
    // 配置键盘显示参数
    nn::swkbd::AppearArg appearArg = {};
    
    appearArg.keyboardArg.configArg.languageType = nn::swkbd::LanguageType::English;
    appearArg.keyboardArg.configArg.keyboardMode = nn::swkbd::KeyboardMode::Full;
    appearArg.inputFormArg.type = nn::swkbd::InputFormType::Default;
    appearArg.inputFormArg.maxTextLength = maxLength;
    
    // 设置提示文本
    std::u16string hintTextU16;
    if (!hintText.empty()) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        hintTextU16 = converter.from_bytes(hintText);
        appearArg.inputFormArg.hintText = hintTextU16.c_str();
    }
    
    // 设置初始文本
    std::u16string initialTextU16;
    if (!initialText.empty()) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        initialTextU16 = converter.from_bytes(initialText);
        appearArg.inputFormArg.initialText = initialTextU16.c_str();
        appearArg.inputFormArg.higlightInitialText = true;
    }
    
    // 显示键盘
    if (!nn::swkbd::AppearInputForm(appearArg)) {
        FileLogger::GetInstance().LogError("[SwkbdManager] Failed to appear input form");
        return false;
    }
    
    // 启用触摸屏传感器条（必须，否则触摸不工作）
    VPADSetSensorBar(VPAD_CHAN_0, true);
    
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Keyboard appeared");
    
    // 输入循环
    nn::swkbd::ControllerInfo controllerInfo;
    VPADStatus vpadStatus;
    KPADStatus kpadStatus[4];
    
    while (nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden) {
        // 读取输入（VPADStatus 包含 GamePad 按钮、摇杆和触摸屏）
        VPADRead(VPAD_CHAN_0, &vpadStatus, 1, nullptr);
        
        // 校准触摸屏坐标（必须，否则触摸位置不准确）
        VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpadStatus.tpNormal, &vpadStatus.tpNormal);
        
        controllerInfo.vpad = &vpadStatus;
        
        // 读取 Wiimote 输入
        for (int i = 0; i < 4; i++) {
            KPADReadEx((KPADChan)i, &kpadStatus[i], 1, nullptr);
            controllerInfo.kpad[i] = &kpadStatus[i];
        }
        
        // 处理字体和预测计算
        if (nn::swkbd::IsNeedCalcSubThreadFont()) {
            nn::swkbd::CalcSubThreadFont();
        }
        if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
            nn::swkbd::CalcSubThreadPredict();
        }
        
        // 更新键盘状态（这里会处理触摸屏输入）
        nn::swkbd::Calc(controllerInfo);
        
        // 检查确认或取消（必须在 Calc 之后）
        if (nn::swkbd::IsDecideOkButton(nullptr)) {
            FileLogger::GetInstance().LogInfo("[SwkbdManager] OK button pressed");
            break;
        }
        if (nn::swkbd::IsDecideCancelButton(nullptr)) {
            FileLogger::GetInstance().LogInfo("[SwkbdManager] Cancel button pressed");
            break;
        }
        
        // 绘制背景（半透明遮罩）
        SDL_Color overlayColor = {0, 0, 0, 180};
        Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_HEIGHT, overlayColor);
        
        // 绘制键盘（DRC 和 TV）
        nn::swkbd::DrawDRC();
        nn::swkbd::DrawTV();
        
        Gfx::Render();
    }
    
    // 获取结果
    bool success = false;
    const char16_t* result = nn::swkbd::GetInputFormString();
    if (result) {
        // 转换 UTF-16 到 UTF-8
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        outText = converter.to_bytes(result);
        FileLogger::GetInstance().LogInfo("[SwkbdManager] Input text: %s", outText.c_str());
        success = true;
    }
    
    // 禁用触摸屏传感器条
    VPADSetSensorBar(VPAD_CHAN_0, false);
    
    // 隐藏键盘（等待淡出动画完全结束）
    if (nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden) {
        FileLogger::GetInstance().LogInfo("[SwkbdManager] Disappearing keyboard...");
        nn::swkbd::DisappearInputForm();
        
        // 必须继续渲染直到键盘完全隐藏（限制最多60帧避免卡死）
        VPADStatus vpadStatusFade;
        KPADStatus kpadStatusFade[4];
        nn::swkbd::ControllerInfo controllerInfoFade;
        int fadeFrames = 0;
        const int MAX_FADE_FRAMES = 60;  // 最多1秒（60fps）
        
        while (nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden && fadeFrames < MAX_FADE_FRAMES) {
            // 继续读取输入并更新键盘状态
            VPADRead(VPAD_CHAN_0, &vpadStatusFade, 1, nullptr);
            VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpadStatusFade.tpNormal, &vpadStatusFade.tpNormal);
            controllerInfoFade.vpad = &vpadStatusFade;
            
            for (int i = 0; i < 4; i++) {
                KPADReadEx((KPADChan)i, &kpadStatusFade[i], 1, nullptr);
                controllerInfoFade.kpad[i] = &kpadStatusFade[i];
            }
            
            if (nn::swkbd::IsNeedCalcSubThreadFont()) {
                nn::swkbd::CalcSubThreadFont();
            }
            if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
                nn::swkbd::CalcSubThreadPredict();
            }
            
            nn::swkbd::Calc(controllerInfoFade);
            
            // 绘制背景遮罩（与主循环一致）
            SDL_Color overlayColor = {0, 0, 0, 180};
            Gfx::DrawRectFilled(0, 0, Gfx::SCREEN_WIDTH, Gfx::SCREEN_HEIGHT, overlayColor);
            
            // 绘制淡出动画
            nn::swkbd::DrawDRC();
            nn::swkbd::DrawTV();
            Gfx::Render();
            
            fadeFrames++;
        }
        
        if (fadeFrames >= MAX_FADE_FRAMES) {
            FileLogger::GetInstance().LogWarning("[SwkbdManager] Fade timeout after %d frames", fadeFrames);
        } else {
            FileLogger::GetInstance().LogInfo("[SwkbdManager] Keyboard hidden after %d frames", fadeFrames);
        }
    }
    
    FileLogger::GetInstance().LogInfo("[SwkbdManager] Keyboard hidden");
    return success;
}
