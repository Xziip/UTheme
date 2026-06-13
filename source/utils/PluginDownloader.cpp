#include "PluginDownloader.hpp"
#include "FileLogger.hpp"
#include "Utils.hpp"
#include "../Screen.hpp"
#include <curl/curl.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

PluginDownloader& PluginDownloader::GetInstance() {
    static PluginDownloader instance;
    return instance;
}

PluginDownloader::PluginDownloader() 
    : mState(PLUGIN_IDLE)
    , mProgress(0.0f)
    , mDownloadedBytes(0)
    , mTotalBytes(0)
    , mCancelRequested(false)
    , mThreadRunning(false) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    FileLogger::GetInstance().LogInfo("[PluginDownloader] Initialized");
}

PluginDownloader::~PluginDownloader() {
    Cancel();
    
    if (mDownloadThread.joinable()) {
        mDownloadThread.join();
    }
    
    curl_global_cleanup();
    FileLogger::GetInstance().LogInfo("[PluginDownloader] Destroyed");
}

void PluginDownloader::StartDownload() {
    // 如果正在下载,先取消
    if (IsDownloading()) {
        FileLogger::GetInstance().LogInfo("[PluginDownloader] Already downloading, canceling previous");
        Cancel();
        
        if (mDownloadThread.joinable()) {
            mDownloadThread.join();
        }
    }
    
    // 使用动态环境路径
    std::string envPath = Utils::GetEnvironmentPath();
    if (envPath.empty()) {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrorMessage = "Failed to get environment path";
        FileLogger::GetInstance().LogError("[PluginDownloader] %s", mErrorMessage.c_str());
        mState.store(PLUGIN_ERROR);
        if (mCompletionCallback) {
            mCompletionCallback(false, mErrorMessage);
        }
        return;
    }
    
    mDestPath = envPath + "/plugins/stylemiiu.wps";
    
    // 检查文件是否已存在
    struct stat st;
    if (stat(mDestPath.c_str(), &st) == 0) {
        FileLogger::GetInstance().LogInfo("[PluginDownloader] StyleMiiU plugin already exists");
        mState.store(PLUGIN_COMPLETE);
        mProgress.store(1.0f);
        std::lock_guard<std::mutex> lock(mMutex);
        if (mCompletionCallback) {
            mCompletionCallback(true, mDestPath);
        }
        return;
    }
    
    mCancelRequested.store(false);
    mState.store(PLUGIN_DOWNLOADING);
    mProgress.store(0.0f);
    mDownloadedBytes.store(0);
    mTotalBytes.store(0);
    mErrorMessage.clear();
    
    FileLogger::GetInstance().LogInfo("[PluginDownloader] Starting async download to: %s", mDestPath.c_str());
    
    // 启动后台线程
    mThreadRunning = true;
    mDownloadThread = std::thread([this]() {
        PerformDownload();
        mThreadRunning = false;
    });
}

void PluginDownloader::Cancel() {
    if (IsDownloading()) {
        FileLogger::GetInstance().LogInfo("[PluginDownloader] Canceling download");
        mCancelRequested.store(true);
        mState.store(PLUGIN_CANCELLED);
    }
}

void PluginDownloader::SetCompletionCallback(std::function<void(bool, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(mMutex);
    mCompletionCallback = callback;
}

void PluginDownloader::Update() {
    // 状态由后台线程更新,这里不需要额外处理
}

int PluginDownloader::ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, 
                                        curl_off_t ultotal, curl_off_t ulnow) {
    PluginDownloader* downloader = static_cast<PluginDownloader*>(clientp);
    
    if (downloader->mCancelRequested.load()) {
        return 1;
    }
    
    if (dltotal > 0) {
        downloader->mTotalBytes.store(dltotal);
        downloader->mDownloadedBytes.store(dlnow);
        float progress = (float)dlnow / (float)dltotal;
        downloader->mProgress.store(progress);
    }
    
    return 0;
}

void PluginDownloader::PerformDownload() {
    const char* downloadUrl = "https://github.com/Themiify-hb/StyleMiiU-Plugin/releases/latest/download/stylemiiu.wps";
    std::string tempPath = mDestPath + ".tmp";
    
    FileLogger::GetInstance().LogInfo("[PluginDownloader] Background download started: %s -> %s", 
        downloadUrl, mDestPath.c_str());
    
    // 确保目录存在
    std::string dirPath = mDestPath.substr(0, mDestPath.find_last_of('/'));
    struct stat st;
    if (stat(dirPath.c_str(), &st) != 0) {
        // 递归创建目录
        size_t pos = 0;
        while ((pos = dirPath.find('/', pos + 1)) != std::string::npos) {
            std::string subDir = dirPath.substr(0, pos);
            if (stat(subDir.c_str(), &st) != 0) {
                mkdir(subDir.c_str(), 0777);
            }
        }
        mkdir(dirPath.c_str(), 0777);
    }
    
    // 打开临时文件
    FILE* file = fopen(tempPath.c_str(), "wb");
    if (!file) {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrorMessage = "Failed to create temporary file";
        FileLogger::GetInstance().LogError("[PluginDownloader] %s", mErrorMessage.c_str());
        mState.store(PLUGIN_ERROR);
        Screen::GetBgmNotification().ShowError("Plugin download failed: " + mErrorMessage);
        if (mCompletionCallback) {
            mCompletionCallback(false, mErrorMessage);
        }
        return;
    }
    
    // 初始化CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrorMessage = "Failed to initialize CURL";
        FileLogger::GetInstance().LogError("[PluginDownloader] %s", mErrorMessage.c_str());
        fclose(file);
        remove(tempPath.c_str());
        mState.store(PLUGIN_ERROR);
        Screen::GetBgmNotification().ShowError("Plugin download failed: " + mErrorMessage);
        if (mCompletionCallback) {
            mCompletionCallback(false, mErrorMessage);
        }
        return;
    }
    
    // 配置CURL
    curl_easy_setopt(curl, CURLOPT_URL, downloadUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_USER_AGENT);
    
    // 执行下载(在后台线程中,不影响UI)
    CURLcode res = curl_easy_perform(curl);
    
    fclose(file);
    
    // 获取HTTP状态码
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    curl_easy_cleanup(curl);
    
    // 检查结果
    if (res != CURLE_OK) {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrorMessage = curl_easy_strerror(res);
        FileLogger::GetInstance().LogError("[PluginDownloader] Download failed: %s", mErrorMessage.c_str());
        remove(tempPath.c_str());
        mState.store(PLUGIN_ERROR);
        Screen::GetBgmNotification().ShowError("Plugin download failed: " + mErrorMessage);
        if (mCompletionCallback) {
            mCompletionCallback(false, mErrorMessage);
        }
        return;
    }
    
    if (httpCode != 200) {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrorMessage = "HTTP error: " + std::to_string(httpCode);
        FileLogger::GetInstance().LogError("[PluginDownloader] %s", mErrorMessage.c_str());
        remove(tempPath.c_str());
        mState.store(PLUGIN_ERROR);
        Screen::GetBgmNotification().ShowError(mErrorMessage);
        if (mCompletionCallback) {
            mCompletionCallback(false, mErrorMessage);
        }
        return;
    }
    
    // 重命名临时文件
    remove(mDestPath.c_str());
    if (rename(tempPath.c_str(), mDestPath.c_str()) != 0) {
        std::lock_guard<std::mutex> lock(mMutex);
        mErrorMessage = "Failed to rename temporary file";
        FileLogger::GetInstance().LogError("[PluginDownloader] %s", mErrorMessage.c_str());
        remove(tempPath.c_str());
        mState.store(PLUGIN_ERROR);
        Screen::GetBgmNotification().ShowError("Plugin install failed: " + mErrorMessage);
        if (mCompletionCallback) {
            mCompletionCallback(false, mErrorMessage);
        }
        return;
    }
    
    // 下载成功
    FileLogger::GetInstance().LogInfo("[PluginDownloader] Download completed successfully");
    mState.store(PLUGIN_COMPLETE);
    mProgress.store(1.0f);
    
    Screen::GetBgmNotification().ShowNowPlaying("StyleMiiU Plugin Downloaded");
    
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCompletionCallback) {
        mCompletionCallback(true, mDestPath);
    }
}
