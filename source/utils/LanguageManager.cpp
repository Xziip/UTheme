#include "LanguageManager.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "logger.h"
#include "../Gfx.hpp"
#include "rapidjson/document.h"
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>

// 包含嵌入的语言文件
#include <zh-cn_json.h>
#include <zh-tw_json.h>
#include <en-us_json.h>
#include <ja-jp_json.h>
#include <ko-kr_json.h>
#include <fr-fr_json.h>
#include <de-de_json.h>
#include <es-es_json.h>
#include <it-it_json.h>
#include <ru-ru_json.h>
#include <pt-br_json.h>
#include <nl-nl_json.h>
#include <pl-pl_json.h>

LanguageManager* LanguageManager::mInstance = nullptr;

// 使用 rapidjson 解析平坦化的 JSON
static std::map<std::string, std::string> ParseFlatJson(const std::string& jsonStr) {
    std::map<std::string, std::string> result;
    
    try {
        rapidjson::Document j;
        j.Parse(jsonStr.c_str());
        
        if (j.HasParseError()) {
            DEBUG_FUNCTION_LINE("JSON parsing failed: parse error");
            return result;
        }
        
        // 使用递归函数将嵌套的 JSON 对象平坦化
        std::function<void(const rapidjson::Value&, const std::string&)> flatten;
        flatten = [&](const rapidjson::Value& node, const std::string& prefix) {
            if (node.IsObject()) {
                for (auto it = node.MemberBegin(); it != node.MemberEnd(); ++it) {
                    std::string key = prefix.empty() ? it->name.GetString() : prefix + "." + it->name.GetString();
                    flatten(it->value, key);
                }
            } else if (node.IsString()) {
                result[prefix] = node.GetString();
            }
            // 忽略其他类型（数组、数字等）
        };
        
        flatten(j, "");
        
    } catch (const std::exception& e) {
        DEBUG_FUNCTION_LINE("JSON parsing failed: %s", e.what());
    }
    
    return result;
}

LanguageManager& LanguageManager::getInstance() {
    if (!mInstance) {
        mInstance = new LanguageManager();
    }
    return *mInstance;
}

bool LanguageManager::Initialize() {
    DEBUG_FUNCTION_LINE("Initializing LanguageManager");
    
    // 设置可用语言
    mAvailableLanguages = {
        {"zh-cn", "简体中文", "zh-cn.json"},
        {"zh-tw", "繁體中文", "zh-tw.json"},
        {"en-us", "English", "en-us.json"},
        {"ja-jp", "日本語", "ja-jp.json"},
        {"ko-kr", "한국어", "ko-kr.json"},
        {"fr-fr", "Français", "fr-fr.json"},
        {"de-de", "Deutsch", "de-de.json"},
        {"es-es", "Español", "es-es.json"},
        {"it-it", "Italiano", "it-it.json"},
        {"ru-ru", "Русский", "ru-ru.json"},
        {"pt-br", "Português", "pt-br.json"},
        {"nl-nl", "Nederlands", "nl-nl.json"},
        {"pl-pl", "Polski", "pl-pl.json"}
    };
    
    // 加载设置
    LoadSettings();
    
    // 加载默认语言
    if (!LoadLanguage(mCurrentLanguage)) {
        // 如果加载失败，尝试加载英语
        if (!LoadLanguage("en-us")) {
            // 如果还是失败，尝试加载中文
            LoadLanguage("zh-cn");
        }
    }
    
    return true;
}

bool LanguageManager::LoadLanguage(const std::string& languageCode) {
    DEBUG_FUNCTION_LINE("Loading language: %s", languageCode.c_str());
    
    // 获取嵌入的语言数据
    const uint8_t* langData = nullptr;
    size_t langSize = 0;
    
    if (languageCode == "zh-cn") {
        langData = zh_cn_json;
        langSize = zh_cn_json_size;
    } else if (languageCode == "zh-tw") {
        langData = zh_tw_json;
        langSize = zh_tw_json_size;
    } else if (languageCode == "en-us") {
        langData = en_us_json;
        langSize = en_us_json_size;
    } else if (languageCode == "ja-jp") {
        langData = ja_jp_json;
        langSize = ja_jp_json_size;
    } else if (languageCode == "ko-kr") {
        langData = ko_kr_json;
        langSize = ko_kr_json_size;
    } else if (languageCode == "fr-fr") {
        langData = fr_fr_json;
        langSize = fr_fr_json_size;
    } else if (languageCode == "de-de") {
        langData = de_de_json;
        langSize = de_de_json_size;
    } else if (languageCode == "es-es") {
        langData = es_es_json;
        langSize = es_es_json_size;
    } else if (languageCode == "it-it") {
        langData = it_it_json;
        langSize = it_it_json_size;
    } else if (languageCode == "ru-ru") {
        langData = ru_ru_json;
        langSize = ru_ru_json_size;
    } else if (languageCode == "pt-br") {
        langData = pt_br_json;
        langSize = pt_br_json_size;
    } else if (languageCode == "nl-nl") {
        langData = nl_nl_json;
        langSize = nl_nl_json_size;
    } else if (languageCode == "pl-pl") {
        langData = pl_pl_json;
        langSize = pl_pl_json_size;
    }
    
    if (!langData || langSize == 0) {
        DEBUG_FUNCTION_LINE("Language data not found: %s", languageCode.c_str());
        return false;
    }
    
    // 转换为字符串
    std::string content(reinterpret_cast<const char*>(langData), langSize);
    
    if (content.empty()) {
        DEBUG_FUNCTION_LINE("Language content is empty: %s", languageCode.c_str());
        return false;
    }
    
    // 解析JSON
    mTexts = ParseFlatJson(content);
    
    if (mTexts.empty()) {
        DEBUG_FUNCTION_LINE("Failed to parse language data: %s", languageCode.c_str());
        return false;
    }
    
    mCurrentLanguage = languageCode;
    
    // Determine if language needs CJK font or Latin font
    // CJK languages: zh-cn, zh-tw, ja-jp, ko-kr
    // All others use Latin/Cyrillic font (Noto Sans)
    bool needsCJKFont = (languageCode == "zh-cn" || 
                         languageCode == "zh-tw" || 
                         languageCode == "ja-jp" || 
                         languageCode == "ko-kr");
    
    Gfx::SetUseLatinFont(!needsCJKFont);
    
    DEBUG_FUNCTION_LINE("Successfully loaded language: %s (%d texts), using %s font", 
                       languageCode.c_str(), (int)mTexts.size(),
                       needsCJKFont ? "CJK" : "Latin");
    
    // 测试几个关键键是否存在
    DEBUG_FUNCTION_LINE("Test key 'app_name': %s", GetText("app_name").c_str());
    DEBUG_FUNCTION_LINE("Test key 'theme_detail.by': %s", GetText("theme_detail.by").c_str());
    
    return true;
}

std::string LanguageManager::GetText(const std::string& key) const {
    auto it = mTexts.find(key);
    if (it != mTexts.end()) {
        return it->second;
    }
    
    // 如果没找到，返回键本身
    // DEBUG_FUNCTION_LINE("Language key not found: %s (total keys: %d)", key.c_str(), (int)mTexts.size());
    return key;
}

void LanguageManager::SetCurrentLanguage(const std::string& languageCode) {
    if (LoadLanguage(languageCode)) {
        mCurrentLanguage = languageCode;
        SaveLanguageSettings();
    }
}

void LanguageManager::LoadSettings() {
    // 从Config加载语言设置
    std::string configLang = Config::GetInstance().GetLanguage();
    
    // 验证语言代码是否有效
    bool validLanguage = false;
    for (const auto& lang : mAvailableLanguages) {
        if (lang.code == configLang) {
            validLanguage = true;
            break;
        }
    }
    
    if (validLanguage) {
        mCurrentLanguage = configLang;
        DEBUG_FUNCTION_LINE("Loaded language setting from config: %s", mCurrentLanguage.c_str());
    } else {
        // 如果配置中的语言无效,使用默认中文
        mCurrentLanguage = "zh-cn";
        DEBUG_FUNCTION_LINE("Invalid language in config, using default: zh-cn");
    }
}

void LanguageManager::SaveLanguageSettings() {
    // 保存语言设置到Config
    Config::GetInstance().SetLanguage(mCurrentLanguage);
    DEBUG_FUNCTION_LINE("Saved language setting to config: %s", mCurrentLanguage.c_str());
}
