#pragma once

namespace AppInstaller::Utility
{
    const unsigned int APPINSTALLER_DOWNLOAD_SUCCESS = 0;
    const unsigned int APPINSTALLER_DOWNLOAD_FAILED = 1;
    const unsigned int APPINSTALLER_DOWNLOAD_CANCELED = 2;

    class IDownloaderCallback
    {
    public:
        virtual void OnStarted(LONGLONG downloadSize);

        virtual void OnProgress(LONGLONG progress);

        virtual void OnCanceled();

        virtual void OnCompleted();
    };

    class Downloader
    {
    public:
        std::future<std::pair<unsigned int, std::string>> DownloadAsync(
            const std::string& url,
            const std::filesystem::path& dest,
            bool computeHash = false,
            IDownloaderCallback* callback = nullptr);

        void Cancel();

    private:
        HINTERNET m_session;
        HINTERNET m_urlFile;
        LONGLONG m_downloadSize = 0;
        LONGLONG m_progress = 0;
        bool m_isCanceled = false;
        bool m_downloading = false;
        BYTE* m_buffer;
        std::ofstream m_outfile;

        std::pair<unsigned int, std::string> DownloadInternal(
            const std::string& url,
            const std::filesystem::path& dest,
            bool computeHash,
            IDownloaderCallback* callback);

        void Cleanup();
    };
}