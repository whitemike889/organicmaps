#include "platform/platform.hpp"

#include "platform/socket.hpp"

#include "coding/file_reader.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <string>

#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

namespace
{
// Web service ip to check internet connection. Now it's a mail.ru ip.
char constexpr kSomeWorkingWebServer[] = "217.69.139.202";

// Returns directory where binary resides, including slash at the end.
bool GetBinaryDir(string & outPath)
{
  char path[4096] = {};
  if (::readlink("/proc/self/exe", path, ARRAY_SIZE(path)) <= 0)
    return false;
  outPath = path;
  outPath.erase(outPath.find_last_of('/') + 1);
  return true;
}

// Returns true if EULA file exists in directory.
bool IsEulaExist(string const & directory)
{
  return Platform::IsFileExistsByFullPath(base::JoinPath(directory, "eula.html"));
}

string HomeDir()
{
  char const * homePath = ::getenv("HOME");
  if (homePath == nullptr)
    MYTHROW(RootException, ("The environment variable HOME is not set"));
  return homePath;
}

}  // namespace

namespace platform
{
unique_ptr<Socket> CreateSocket()
{
  return unique_ptr<Socket>();
}
} // namespace platform


Platform::Platform()
{
  // Init directories.
  string path;
  CHECK(GetBinaryDir(path), ("Can't retrieve path to executable"));

  m_settingsDir = base::JoinPath(HomeDir(), ".config", "OMaps");

  if (!IsFileExistsByFullPath(base::JoinPath(m_settingsDir, SETTINGS_FILE_NAME)))
  {
    if (!MkDirRecursively(m_settingsDir))
      MYTHROW(FileSystemException, ("Can't create directory", m_settingsDir));
  }

  char const * resDir = ::getenv("MWM_RESOURCES_DIR");
  char const * writableDir = ::getenv("MWM_WRITABLE_DIR");
  if (resDir && writableDir)
  {
    m_resourcesDir = resDir;
    m_writableDir = writableDir;
  }
  else if (resDir)
  {
    m_resourcesDir = resDir;
    m_writableDir = base::JoinPath(HomeDir(), ".local", "share", "OMaps");
    if (!MkDirRecursively(m_writableDir))
      MYTHROW(FileSystemException, ("Can't create directory:", m_writableDir));
  }
  else
  {
    initializer_list<string> dirs = {
      "./data",                                 // check symlink in the current folder
      "../data",                                // check if we are in the 'build' folder inside repo
      base::JoinPath(path, "..", "..", "data"), // check symlink from bundle?
      base::JoinPath(path, "..", "share"),      // installed version with packages
      base::JoinPath(path, "..", "OMaps")       // installed version without packages
    };

    for (auto const & dir : dirs)
    {
      if (IsEulaExist(dir))
      {
        m_resourcesDir = dir;
        m_writableDir = (writableDir != nullptr) ? writableDir : m_resourcesDir;
        break;
      }
    }
  }

  m_resourcesDir += '/';
  m_settingsDir += '/';
  m_writableDir += '/';

  char const * tmpDir = ::getenv("TMPDIR");
  if (tmpDir)
    m_tmpDir = tmpDir;
  else
    m_tmpDir = "/tmp";
  m_tmpDir += '/';

  m_privateDir = m_settingsDir;

  m_guiThread = make_unique<platform::GuiThread>();

  LOG(LDEBUG, ("Resources directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings directory:", m_settingsDir));
  LOG(LDEBUG, ("Client ID:", UniqueClientId()));
}

string Platform::UniqueClientId() const
{
  string machineFile = "/var/lib/dbus/machine-id";
  if (IsFileExistsByFullPath("/etc/machine-id"))
    machineFile = "/etc/machine-id";

  if (IsFileExistsByFullPath(machineFile))
  {
    string content;
    FileReader(machineFile).ReadAsString(content);
    return content.substr(0, 32);
  }

  return "n0dbus0n0lsb00000000000000000000";
}

string Platform::MacAddress(bool md5Decoded) const
{
  // Not implemented.
  UNUSED_VALUE(md5Decoded);
  return {};
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

string Platform::DeviceModel() const
{
  return {};
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  int socketFd = socket(AF_INET, SOCK_STREAM, 0);
  SCOPE_GUARD(closeSocket, bind(&close, socketFd));
  if (socketFd < 0)
    return EConnectionType::CONNECTION_NONE;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, kSomeWorkingWebServer, &addr.sin_addr);

  if (connect(socketFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    return EConnectionType::CONNECTION_NONE;

  return EConnectionType::CONNECTION_WIFI;
}

Platform::ChargingStatus Platform::GetChargingStatus()
{
  return Platform::ChargingStatus::Plugged;
}

uint8_t Platform::GetBatteryLevel()
{
  // This value is always 100 for desktop.
  return 100;
}
