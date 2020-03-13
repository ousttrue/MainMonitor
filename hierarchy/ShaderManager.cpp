#include "ShaderManager.h"
#include <thread>
#include <functional>
#include <fstream>
#include <windows.h>

namespace hierarchy
{

static std::string ReadAllText(const std::filesystem::path &path)
{
    std::string result;
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (ifs)
    {
        auto pos = ifs.tellg();
        result.resize(pos);
        ifs.seekg(0, std::ios::beg);
        ifs.read((char *)result.data(), pos);
    }
    return result;
}

static std::wstring multi_to_wide_winapi(std::string const &src)
{
    auto const dest_size = ::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, nullptr, 0U);
    std::vector<wchar_t> dest(dest_size, L'\0');
    if (::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, dest.data(), dest.size()) == 0)
    {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    dest.resize(std::char_traits<wchar_t>::length(dest.data()));
    dest.shrink_to_fit();
    return std::wstring(dest.begin(), dest.end());
}

class DirectoryWatcher
{
    std::filesystem::path m_path;
    HANDLE m_hDir = NULL;
    std::thread m_thread;

public:
    DirectoryWatcher(const std::filesystem::path &path, const std::function<void(const std::wstring &, int)> &callback)
        : m_path(path)
    {
        auto hDir = CreateFileW((LPCWSTR)path.u16string().c_str(),
                                FILE_LIST_DIRECTORY,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
        if (hDir == INVALID_HANDLE_VALUE)
        {
            throw "CreateFile failed.";
        }
        m_hDir = hDir;

        m_thread = std::thread([hDir, callback]() {
            while (true)
            {
                uint8_t buffer[1024] = {};
                DWORD dwBytesReturned;
                auto bRet = ReadDirectoryChangesW(hDir,
                                                  buffer,
                                                  sizeof(buffer),
                                                  TRUE,
                                                  FILE_NOTIFY_CHANGE_LAST_WRITE,
                                                  &dwBytesReturned,
                                                  NULL,
                                                  NULL);
                if (!bRet)
                {
                    throw "ReadDirectoryChangesW failed.";
                }

                if (dwBytesReturned == 0)
                {
                    throw;
                }

                FILE_NOTIFY_INFORMATION *p = nullptr;
                for (DWORD i = 0; i < dwBytesReturned; i += p->NextEntryOffset)
                {
                    p = (FILE_NOTIFY_INFORMATION *)&buffer[i];
                    callback(std::wstring(p->FileName, p->FileName + p->FileNameLength), p->Action);
                    // switch (p->Action)
                    // {
                    // case FILE_ACTION_ADDED:
                    //     // wprintf(L"FILE_ACTION_ADDED: %s\n", filename);
                    //     break;

                    // case FILE_ACTION_REMOVED:
                    //     // wprintf(L"FILE_ACTION_REMOVED: %s\n", filename);
                    //     break;

                    // case FILE_ACTION_MODIFIED:
                    //     // wprintf(L"FILE_ACTION_MODIFIED: %s\n", filename);
                    //     break;

                    // case FILE_ACTION_RENAMED_OLD_NAME:
                    //     // wprintf(L"FILE_ACTION_RENAMED_OLD_NAME: %s\n", filename);
                    //     break;

                    // case FILE_ACTION_RENAMED_NEW_NAME:
                    //     // wprintf(L"FILE_ACTION_RENAMED_NEW_NAME: %s\n", filename);
                    //     break;

                    // default:
                    //     // wprintf(L"Unknown File Action: %s\n", filename);
                    //     break;
                    // }

                    if (!p->NextEntryOffset)
                    {
                        break;
                    }
                }
            }

            auto a = 0;
        });
    }

    ~DirectoryWatcher()
    {
        if (m_hDir)
        {
            CloseHandle(m_hDir);
            m_hDir = nullptr;
        }
        m_thread.join();
    }

    std::filesystem::path getPath(const std::wstring &shaderName)
    {
        return m_path.append(shaderName);
    }

}; // namespace hierarchy

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
    stop();
}

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

void ShaderManager::watch(std::filesystem::path &path)
{
    stop();
    m_watcher = new DirectoryWatcher(path, std::bind(&ShaderManager::onFile, this, std::placeholders::_1, std::placeholders::_2));
}

void ShaderManager::stop()
{
    if (m_watcher)
    {
        delete m_watcher;
        m_watcher = nullptr;
    }
}

// default
ShaderWatcherPtr ShaderManager::get(const std::string &shaderName)
{
    auto fileName = multi_to_wide_winapi(shaderName + ".hlsl");
    auto found = m_shaderMap.find(fileName);
    if (found != m_shaderMap.end())
    {
        return found->second;
    }

    auto shader = std::make_shared<ShaderWatcher>();
    auto source = ReadAllText(m_watcher->getPath(fileName));
    shader->source(source);

    {
        std::lock_guard<std::mutex> scoped(m_mutex);
        m_shaderMap.insert(std::make_pair(fileName, shader));
    }

    return shader;
}

void ShaderManager::onFile(const std::wstring &fileName, int action)
{
    if (action == FILE_ACTION_MODIFIED)
    {
        std::lock_guard<std::mutex> scoped(m_mutex);
        auto found = m_shaderMap.find(fileName);
        if (found != m_shaderMap.end())
        {
            auto source = ReadAllText(m_watcher->getPath(fileName));
            found->second->source(source);
        }
    }
}

} // namespace hierarchy
