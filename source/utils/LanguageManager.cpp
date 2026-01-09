#include "LanguageManager.hpp"
#include "Config.hpp"
#include "Utils.hpp"
#include "logger.h"
#include "../Gfx.hpp"
#include <fstream>
#include <sstream>
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

// 简单的JSON解析器（只处理字符串值）
class SimpleJsonParser {
public:
    static std::map<std::string, std::string> ParseFlat(const std::string& jsonStr) {
        std::map<std::string, std::string> result;
        std::string content = jsonStr;
        
        // 移除空白字符和换行
        content.erase(std::remove_if(content.begin(), content.end(), 
                     [](char c) { return c == '\n' || c == '\r' || c == '\t'; }), content.end());
        
        size_t pos = 0;
        std::string currentPath = "";
        
        ParseObject(content, pos, result, currentPath);
        
        return result;
    }

private:
    static void ParseObject(const std::string& content, size_t& pos, 
                           std::map<std::string, std::string>& result, 
                           const std::string& path) {
        // 跳过空白
        SkipWhitespace(content, pos);
        
        if (pos >= content.length() || content[pos] != '{') {
            return;
        }
        
        pos++; // 跳过 '{'
        
        while (pos < content.length()) {
            SkipWhitespace(content, pos);
            
            if (pos >= content.length()) break;
            if (content[pos] == '}') {
                pos++;
                break;
            }
            
            // 解析键
            if (content[pos] != '"') {
                pos++;
                continue;
            }
            
            std::string key = ParseString(content, pos);
            
            SkipWhitespace(content, pos);
            if (pos >= content.length() || content[pos] != ':') {
                continue;
            }
            pos++; // 跳过 ':'
            
            SkipWhitespace(content, pos);
            
            std::string fullKey = path.empty() ? key : path + "." + key;
            
            if (pos < content.length()) {
                if (content[pos] == '"') {
                    // 字符串值
                    std::string value = ParseString(content, pos);
                    result[fullKey] = value;
                } else if (content[pos] == '{') {
                    // 嵌套对象
                    ParseObject(content, pos, result, fullKey);
                } else {
                    // 跳过其他类型的值
                    SkipValue(content, pos);
                }
            }
            
            SkipWhitespace(content, pos);
            if (pos < content.length() && content[pos] == ',') {
                pos++;
            }
        }
    }
    
    static std::string ParseString(const std::string& content, size_t& pos) {
        if (pos >= content.length() || content[pos] != '"') {
            return "";
        }
        
        pos++; // 跳过开始的引号
        size_t start = pos;
        
        while (pos < content.length() && content[pos] != '"') {
            if (content[pos] == '\\') {
                pos += 2; // 跳过转义字符
            } else {
                pos++;
            }
        }
        
        std::string result = content.substr(start, pos - start);
        
        if (pos < content.length() && content[pos] == '"') {
            pos++; // 跳过结束的引号
        }
        
        return result;
    }
    
    static void SkipWhitespace(const std::string& content, size_t& pos) {
        while (pos < content.length() && 
               (content[pos] == ' ' || content[pos] == '\t' || 
                content[pos] == '\n' || content[pos] == '\r')) {
            pos++;
        }
    }
    
    static void SkipValue(const std::string& content, size_t& pos) {
        SkipWhitespace(content, pos);
        if (pos >= content.length()) return;
        
        if (content[pos] == '"') {
            ParseString(content, pos);
        } else if (content[pos] == '{') {
            int braceCount = 1;
            pos++;
            while (pos < content.length() && braceCount > 0) {
                if (content[pos] == '{') braceCount++;
                else if (content[pos] == '}') braceCount--;
                pos++;
            }
        } else if (content[pos] == '[') {
            int bracketCount = 1;
            pos++;
            while (pos < content.length() && bracketCount > 0) {
                if (content[pos] == '[') bracketCount++;
                else if (content[pos] == ']') bracketCount--;
                pos++;
            }
        } else {
            // 跳过数字、布尔值等
            while (pos < content.length() && 
                   content[pos] != ',' && content[pos] != '}' && content[pos] != ']') {
                pos++;
            }
        }
    }
};

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
    mTexts = SimpleJsonParser::ParseFlat(content);
    
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
