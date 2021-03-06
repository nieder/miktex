/* app.cpp:

   Copyright (C) 2005-2017 Christian Schenk
 
   This file is part of the MiKTeX App Library.

   The MiKTeX App Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.
   
   The MiKTeX App Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with the MiKTeX App Library; if not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA. */

#include "StdAfx.h"

#include "internal.h"
#include "app-version.h"

using namespace MiKTeX::App;
using namespace MiKTeX::Core;
using namespace MiKTeX::Packages;
using namespace MiKTeX::Util;
using namespace std;

#include "miktex/mpm.defaults.h"

static log4cxx::LoggerPtr logger;

static Application* instance = nullptr;

static bool initUiFrameworkDone = false;

static volatile sig_atomic_t cancelled;

static void MIKTEXCEECALL SignalHandler(int signalToBeHandled)
{
  switch (signalToBeHandled)
  {
  case SIGINT:
  case SIGTERM:
    signal(SIGINT, SIG_IGN);
    cancelled = true;
    break;
  }
}

bool Application::Cancelled()
{
  return cancelled == 0 ? false : true;
}

void Application::CheckCancel()
{
  if (Cancelled())
  {
    string programInvocationName = Utils::GetExeName();
    throw MiKTeXException(programInvocationName.c_str(), T_("The current operation has been cancelled (Ctrl-C)."), MiKTeXException::KVMAP(), SourceLocation());
  }
}

Application* Application::GetApplication()
{
  return instance;
}

class Application::impl
{
public:
  set<string> ignoredPackages;
public:
  TriState mpmAutoAdmin = TriState::Undetermined;
public:
  shared_ptr<PackageManager> packageManager;
public:
  shared_ptr<PackageInstaller> installer;
public:
  bool initialized = false;
public:
  vector<TraceCallback::TraceMessage> pendingTraceMessages;
public:
  TriState enableInstaller = TriState::Undetermined;
public:
  bool beQuiet;
public:
  shared_ptr<Session> session;
public:
  bool isLog4cxxConfigured = false;
};

Application::Application() :
  pimpl(make_unique<impl>())
{
}

Application::~Application() noexcept
{
  try
  {
    if (pimpl->initialized)
    {
      Finalize();
    }
    FlushPendingTraceMessages();
  }
  catch (const exception&)
  {
  }
}

void InstallSignalHandler(int sig)
{
  void(*oldHandlerFunc) (int);
  oldHandlerFunc = signal(sig, SignalHandler);
  if (oldHandlerFunc == SIG_ERR)
  {
    MIKTEX_FATAL_CRT_ERROR("signal");
  }
  if (oldHandlerFunc != SIG_DFL)
  {
    if (signal(sig, oldHandlerFunc) == SIG_ERR)
    {
      MIKTEX_FATAL_CRT_ERROR("signal");
    }
  }
}

void Application::Init(const Session::InitInfo& initInfoArg, vector<char*>& args)
{
  instance = this;
  pimpl->initialized = true;
  Session::InitInfo initInfo(initInfoArg);
  MIKTEX_ASSERT(!empty.args() && args.back() == nullptr);
  CommandLineBuilder cmdLineToLog;
  vector<char*>::iterator it = args.begin();
  while (it != args.end() && *it != nullptr)
  {
    cmdLineToLog.AppendArgument(*it);
    bool keepArgument = false;
    if (strcmp(*it, "--miktex-admin") == 0)
    {
      initInfo.AddOption(Session::InitOption::AdminMode);
    }
    else if (strcmp(*it, "--miktex-disable-installer") == 0)
    {
      pimpl->enableInstaller = TriState::False;
    }
    else if (strcmp(*it, "--miktex-enable-installer") == 0)
    {
      pimpl->enableInstaller = TriState::True;
    }
    else
    {
      keepArgument = true;
    }
    if (keepArgument)
    {
      ++it;
    }
    else
    {
      it = args.erase(it);
    }
  }
  initInfo.SetTraceCallback(this);
  pimpl->session = Session::Create(initInfo);
  pimpl->session->SetFindFileCallback(this);
  ConfigureLogging();
  LOG4CXX_INFO(logger, "starting with command line: " << cmdLineToLog.ToString());
  pimpl->beQuiet = false;
  if (pimpl->enableInstaller == TriState::Undetermined)
  {
    pimpl->enableInstaller = pimpl->session->GetConfigValue(MIKTEX_REGKEY_PACKAGE_MANAGER, MIKTEX_REGVAL_AUTO_INSTALL, mpm::AutoInstall()).GetTriState();
  }
  pimpl->mpmAutoAdmin = pimpl->session->GetConfigValue(MIKTEX_REGKEY_PACKAGE_MANAGER, MIKTEX_REGVAL_AUTO_ADMIN, mpm::AutoAdmin()).GetTriState();
  InstallSignalHandler(SIGINT);
  InstallSignalHandler(SIGTERM);
  AutoMaintenance();
}

void Application::ConfigureLogging()
{
  string myName = Utils::GetExeName();
  PathName xmlFileName;
  if (pimpl->session->FindFile(myName + "." + MIKTEX_LOG4CXX_CONFIG_FILENAME, MIKTEX_PATH_TEXMF_PLACEHOLDER "/" MIKTEX_PATH_MIKTEX_PLATFORM_CONFIG_DIR, xmlFileName)
    || pimpl->session->FindFile(MIKTEX_LOG4CXX_CONFIG_FILENAME, MIKTEX_PATH_TEXMF_PLACEHOLDER "/" MIKTEX_PATH_MIKTEX_PLATFORM_CONFIG_DIR, xmlFileName))
  {
    PathName logDir = pimpl->session->GetSpecialPath(SpecialPath::DataRoot) / MIKTEX_PATH_MIKTEX_LOG_DIR;
    string logName = myName;
    if (pimpl->session->IsAdminMode())
    {
      logName += MIKTEX_ADMIN_SUFFIX;
    }
    Utils::SetEnvironmentString("MIKTEX_LOG_DIR", logDir.ToString());
    Utils::SetEnvironmentString("MIKTEX_LOG_NAME", logName);
    log4cxx::xml::DOMConfigurator::configure(xmlFileName.ToWideCharString());
    pimpl->isLog4cxxConfigured = true;
  }
  else
  {
    log4cxx::BasicConfigurator::configure();
  }
  logger = log4cxx::Logger::getLogger(myName);
}

void Application::AutoMaintenance()
{
  time_t lastAdminMaintenance = static_cast<time_t>(std::stoll(pimpl->session->GetConfigValue(MIKTEX_REGKEY_CORE, MIKTEX_REGVAL_LAST_ADMIN_MAINTENANCE, "0").GetString()));
  PathName mpmDatabasePath(pimpl->session->GetMpmDatabasePathName());
  bool mustRefreshFndb = !File::Exists(mpmDatabasePath) || (!pimpl->session->IsAdminMode() && lastAdminMaintenance + 30 > File::GetLastWriteTime(mpmDatabasePath));
  PathName userLanguageDat = pimpl->session->GetSpecialPath(SpecialPath::UserConfigRoot);
  userLanguageDat /= MIKTEX_PATH_LANGUAGE_DAT;
  bool mustRefreshUserLanguageDat = !pimpl->session->IsAdminMode() && File::Exists(userLanguageDat) && lastAdminMaintenance + 30 > File::GetLastWriteTime(userLanguageDat);
  PathName initexmf;
  if ((mustRefreshFndb || mustRefreshUserLanguageDat) && pimpl->session->FindFile(MIKTEX_INITEXMF_EXE, FileType::EXE, initexmf))
  {
    CommandLineBuilder commonCommandLine;
    switch (pimpl->enableInstaller)
    {
    case TriState::False:
      commonCommandLine.AppendOption("--disable-installer");
      break;
    case TriState::True:
      commonCommandLine.AppendOption("--enable-installer");
      break;
    case TriState::Undetermined:
      break;
    }
    if (pimpl->session->IsAdminMode())
    {
      commonCommandLine.AppendOption("--admin");
    }
    commonCommandLine.AppendArgument("--quiet");
    if (mustRefreshFndb)
    {
      pimpl->session->UnloadFilenameDatabase();
      CommandLineBuilder commandLine(commonCommandLine);
      commandLine.AppendOption("--update-fndb");
      LOG4CXX_INFO(logger, "running 'initexmf " << commandLine.ToString() << "' to refresh the file name database");
      Process::Run(initexmf, commandLine.ToString());
    }
    if (mustRefreshUserLanguageDat)
    {
      MIKTEX_ASSERT(!pimpl->session->IsAdminMode());
      CommandLineBuilder commandLine(commonCommandLine);
      commandLine.AppendOption("--mklangs");
      LOG4CXX_INFO(logger, "running 'initexmf " << commandLine.ToString() << "' to refresh language.dat");
      Process::Run(initexmf, commandLine.ToString());
    }
  }
}

void Application::Init(const Session::InitInfo& initInfo)
{
  vector<char*> args{ nullptr };
  Init(initInfo, args);
}

void Application::Init(vector<char*>& args)
{
  MIKTEX_ASSERT(!args.empty() && args.back() != nullptr);
  Init(Session::InitInfo(args[0]), args);
}

void Application::Init(const string& programInvocationName, const string& theNameOfTheGame)
{
  Session::InitInfo initInfo(programInvocationName);
  if (!theNameOfTheGame.empty())
  {
    initInfo.SetTheNameOfTheGame(theNameOfTheGame);
  }
  // FIXME:: eliminate const cast
  vector<char*> args{ (char*)programInvocationName.c_str(), nullptr };
  Init(initInfo, args);
}

void Application::Init(const string& programInvocationName)
{
  Init(programInvocationName, "");
}

void Application::Finalize()
{
  if (logger != nullptr)
  {
    LOG4CXX_DEBUG(logger, "finishing...");
  }
  if (pimpl->installer != nullptr)
  {
    pimpl->installer->Dispose();
    pimpl->installer = nullptr;
  }
  if (pimpl->packageManager != nullptr)
  {
    pimpl->packageManager = nullptr;
  }
  pimpl->session = nullptr;
  pimpl->ignoredPackages.clear();
  if (initUiFrameworkDone)
  {
    MiKTeX::UI::FinalizeFramework();
    initUiFrameworkDone = false;
  }
  logger = nullptr;
  pimpl->initialized = false;
}

void Application::ReportLine(const string& str)
{
  LOG4CXX_INFO(logger, "mpm: " << str);
  if (!GetQuietFlag())
  {
    fputs(str.c_str(), stdout);
    putc('\n', stdout);
  }
}

bool Application::OnRetryableError(const string& message)
{
  UNUSED_ALWAYS(message);
  return false;
}

bool Application::OnProgress(Notification nf)
{
  UNUSED_ALWAYS(nf);
  return true;
}

MIKTEXAPPTHISAPI(void) Application::ShowLibraryVersions() const
{
  vector<LibraryVersion> versions;
  GetLibraryVersions(versions);
  for (auto& ver : set<LibraryVersion>(versions.begin(), versions.end()))
  {
    if (!ver.fromHeader.empty() && !ver.fromRuntime.empty())
    {
      cout << "compiled with " << ver.name << " version " << ver.fromHeader << "; using " << ver.fromRuntime << endl;
    }
    else if (!ver.fromHeader.empty())
    {
      cout << "compiled with " << ver.name << " version " << ver.fromHeader << endl;
    }
    else if (!ver.fromRuntime.empty())
    {
      cout << "using " << ver.name << " version " << ver.fromRuntime << endl;
    }
  }
}

const char* const SEP = "======================================================================";

bool Application::InstallPackage(const string& deploymentName, const PathName& trigger, PathName& installRoot)
{
  if (pimpl->ignoredPackages.find(deploymentName) != pimpl->ignoredPackages.end())
  {
    return false;
  }
  if (pimpl->enableInstaller == TriState::False)
  {
    return false;
  }
  if (pimpl->packageManager == nullptr)
  {
    pimpl->packageManager = PackageManager::Create();
  }
  if (pimpl->enableInstaller == TriState::Undetermined)
  {
    if (!initUiFrameworkDone)
    {
      MiKTeX::UI::InitializeFramework();
      initUiFrameworkDone = true;
    }
    bool doInstall = false;
    unsigned int msgBoxRet = MiKTeX::UI::InstallPackageMessageBox(pimpl->packageManager, deploymentName, trigger.ToString());
    doInstall = ((msgBoxRet & MiKTeX::UI::YES) != 0);
    if ((msgBoxRet & MiKTeX::UI::DONTASKAGAIN) != 0)
    {
      pimpl->enableInstaller = (doInstall ? TriState::True : TriState::False);
    }
    if (!doInstall)
    {
      pimpl->ignoredPackages.insert(deploymentName);
      return false;
    }
    pimpl->mpmAutoAdmin = (((msgBoxRet & MiKTeX::UI::ADMIN) != 0) ? TriState::True : TriState::False);
  }
  string url;
  RepositoryType repositoryType(RepositoryType::Unknown);
  ProxySettings proxySettings;
  if (PackageManager::TryGetDefaultPackageRepository(repositoryType, url)
    && repositoryType == RepositoryType::Remote
    && PackageManager::TryGetProxy(proxySettings)
    && proxySettings.useProxy
    && proxySettings.authenticationRequired
    && proxySettings.user.empty())
  {
    if (!initUiFrameworkDone)
    {
      MiKTeX::UI::InitializeFramework();
      initUiFrameworkDone = true;
    }
    if (!MiKTeX::UI::ProxyAuthenticationDialog())
    {
      return false;
    }
  }
  if (pimpl->installer == nullptr)
  {
    pimpl->installer = pimpl->packageManager->CreateInstaller();
  }
  pimpl->installer->SetCallback(this);
  vector<string> fileList;
  fileList.push_back(deploymentName);
  pimpl->installer->SetFileLists(fileList, vector<string>());
  LOG4CXX_INFO(logger, "installing package " << deploymentName << " triggered by " << trigger.ToString())
  if (!GetQuietFlag())
  {
    cout << endl << SEP << endl;
  }
  bool done = false;
  bool switchToAdminMode = (pimpl->mpmAutoAdmin == TriState::True && !pimpl->session->IsAdminMode());
  if (switchToAdminMode)
  {
    pimpl->session->SetAdminMode(true);
  }
  try
  {
    pimpl->installer->InstallRemove();
    installRoot = pimpl->session->GetSpecialPath(SpecialPath::InstallRoot);
    done = true;
  }
  catch (const MiKTeXException& ex)
  {
    pimpl->enableInstaller = TriState::False;
    pimpl->ignoredPackages.insert(deploymentName);
    LOG4CXX_FATAL(logger, ex.what());
    LOG4CXX_FATAL(logger, "Info: " << ex.GetInfo());
    LOG4CXX_FATAL(logger, "Source: " << ex.GetSourceFile());
    LOG4CXX_FATAL(logger, "Line: " << ex.GetSourceLine());
    cerr << endl << "Unfortunately, the package " << deploymentName << " could not be installed.";
    log4cxx::RollingFileAppenderPtr appender = log4cxx::Logger::getRootLogger()->getAppender(LOG4CXX_STR("RollingLogFile"));
    if (appender != nullptr)
    {
      cerr << "Please check the log file:" << endl << PathName(appender->getFile()).ToUnix() << endl;
    }
  }
  if (switchToAdminMode)
  {
    pimpl->session->SetAdminMode(false);
  }
  if (!GetQuietFlag())
  {
    cout << SEP << endl;
  }
  return done;
}

bool Application::TryCreateFile(const PathName& fileName, FileType fileType)
{
  CommandLineBuilder commandLine;
  switch (pimpl->enableInstaller)
  {
  case TriState::False:
    commandLine.AppendOption("--disable-installer");
    break;
  case TriState::True:
    commandLine.AppendOption("--enable-installer");
    break;
  case TriState::Undetermined:
    break;
  }
  if (pimpl->session->IsAdminMode())
  {
    commandLine.AppendOption("--admin");
  }
  PathName makeUtility;
  PathName baseName = fileName.GetFileNameWithoutExtension();
  switch (fileType)
  {
  case FileType::BASE:
  case FileType::FMT:
    if (!pimpl->session->FindFile(MIKTEX_INITEXMF_EXE, FileType::EXE, makeUtility))
    {
      MIKTEX_FATAL_ERROR(T_("The MiKTeX configuration utility (initexmf) could not be found."));
    }
    commandLine.AppendOption("--dump-by-name=", baseName);
    if (fileType == FileType::FMT)
    {
      commandLine.AppendOption("--engine=", pimpl->session->GetEngineName());
    }
    break;
  case FileType::TFM:
    if (!pimpl->session->FindFile(MIKTEX_MAKETFM_EXE, FileType::EXE, makeUtility))
    {
      MIKTEX_FATAL_ERROR(T_("The MakeTFM utility could not be found."));
    }
    commandLine.AppendArgument(baseName);
    break;
  default:
    return false;
  }
  LOG4CXX_INFO(logger, "going to create file: " << fileName);
  ProcessOutput<50000> processOutput;
  int exitCode;
  if (!Process::Run(makeUtility, commandLine.ToString(), &processOutput, &exitCode, nullptr))
  {
    LOG4CXX_ERROR(logger, makeUtility << " could not be started");
    return false;
  }
  if (exitCode != 0)
  {
    LOG4CXX_ERROR(logger, makeUtility << " did not succeed; exitCode: " << exitCode);
    LOG4CXX_ERROR(logger, "output:");
    LOG4CXX_ERROR(logger, processOutput.StdoutToString());
    return false;
  }
  return true;
}

void Application::EnableInstaller(TriState tri)
{
  pimpl->enableInstaller = tri;
}

TriState Application::GetEnableInstaller() const
{
  return pimpl->enableInstaller;
}

void Application::Trace(const TraceCallback::TraceMessage& traceMessage)
{
  if (!pimpl->isLog4cxxConfigured)
  {
    if (pimpl->pendingTraceMessages.size() > 100)
    {
      pimpl->pendingTraceMessages.clear();
    }
    pimpl->pendingTraceMessages.push_back(traceMessage);
    return;
  }
  FlushPendingTraceMessages();
  TraceInternal(traceMessage);
}

void Application::FlushPendingTraceMessages()
{
  for (const TraceCallback::TraceMessage& m : pimpl->pendingTraceMessages)
  {
    TraceInternal(m);
  }
  pimpl->pendingTraceMessages.clear();
}

void Application::TraceInternal(const TraceCallback::TraceMessage& traceMessage)
{
  if (pimpl->isLog4cxxConfigured)
  {
    log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger(string("trace.") + Utils::GetExeName() + "." + traceMessage.facility);
    if (traceMessage.streamName == MIKTEX_TRACE_ERROR)
    {
      LOG4CXX_ERROR(logger, traceMessage.message);
    }
    else
    {
      LOG4CXX_TRACE(logger, traceMessage.message);
    }
  }
  else
  {
    cerr << traceMessage.message << endl;
  }
}

void Application::Sorry(const string& name, const string& reason)
{
  if (cerr.fail())
  {
    return;
  }
  cerr << endl;
  if (reason.empty())
  {
    cerr << StringUtil::FormatString(T_("Sorry, but %s did not succeed."), Q_(name)) << endl;
  }
  else
  {
    cerr
      << StringUtil::FormatString(T_("Sorry, but %s did not succeed for the following reason:"), Q_(name)) << endl << endl
      << "  " << reason << endl;
  }
  log4cxx::RollingFileAppenderPtr appender = log4cxx::Logger::getRootLogger()->getAppender(LOG4CXX_STR("RollingLogFile"));
  if (appender != nullptr)
  {
    cerr
      << endl
      << "The log file hopefully contains the information to get MiKTeX going again:" << endl
      << endl
      << "  " << PathName(appender->getFile()).ToUnix() << endl;
  }
  cerr
    << endl
    << T_("You may want to visit the MiKTeX project page, if you need help.") << endl;
}

void Application::Sorry(const string& name, const MiKTeXException& ex)
{
  if (logger != nullptr)
  {
    LOG4CXX_FATAL(logger, ex.what());
    LOG4CXX_FATAL(logger, "Info: " << ex.GetInfo());
    LOG4CXX_FATAL(logger, "Source: " << ex.GetSourceFile());
    LOG4CXX_FATAL(logger, "Line: " << ex.GetSourceLine());
  }
  Sorry(name);
}

void Application::Sorry(const string& name, const exception& ex)
{
  if (logger != nullptr)
  {
    LOG4CXX_FATAL(logger, ex.what());
  }
  Sorry(name);
}

MIKTEXNORETURN void Application::FatalError(const char* lpszFormat, ...)
{
  va_list arglist;
  va_start(arglist, lpszFormat);
  string s;
  try
  {
    s = StringUtil::FormatStringVA(lpszFormat, arglist);
  }
  catch (...)
  {
    va_end(arglist);
    throw;
  }
  va_end(arglist);
  if (logger != nullptr)
  {
    LOG4CXX_FATAL(logger, s);
  }
  Sorry(Utils::GetExeName(), s);
  throw 1;
}

void Application::InvokeEditor(const PathName& editFileName, int editLineNumber, FileType editFileType, const PathName& transcriptFileName) const
{
  string defaultEditor;

  PathName texworks;
  if (pimpl->session->FindFile(MIKTEX_TEXWORKS_EXE, FileType::EXE, texworks))
  {
    defaultEditor = Q_(texworks);
    defaultEditor += " -p=%l \"%f\"";
  }
  else
  {
    defaultEditor = "notepad \"%f\"";
  }

  // read information from yap.ini
  // FIXME: use FindFile()
  PathName yapIni = pimpl->session->GetSpecialPath(SpecialPath::UserConfigRoot);
  yapIni /= MIKTEX_PATH_MIKTEX_CONFIG_DIR;
  yapIni /= MIKTEX_YAP_INI_FILENAME;
  if (File::Exists(yapIni))
  {
    unique_ptr<Cfg> yapConfig(Cfg::Create());
    yapConfig->Read(yapIni);
    string yapEditor;
    if (yapConfig->TryGetValue("Settings", "Editor", yapEditor))
    {
      defaultEditor = yapEditor;
    }
  }

  string templ = pimpl->session->GetConfigValue("", MIKTEX_REGVAL_EDITOR, defaultEditor).GetString();

  const char* lpszCommandLineTemplate = templ.c_str();

  string fileName;

  bool quoted = false;

  for (; *lpszCommandLineTemplate != ' ' || (*lpszCommandLineTemplate != 0 && quoted); ++lpszCommandLineTemplate)
  {
    if (*lpszCommandLineTemplate == '"')
    {
      quoted = !quoted;
    }
    else
    {
      fileName += *lpszCommandLineTemplate;
    }
  }

  for (; *lpszCommandLineTemplate == ' '; ++lpszCommandLineTemplate)
  {

  }

  string arguments;

  while (*lpszCommandLineTemplate != 0)
  {
    if (lpszCommandLineTemplate[0] == '%' && lpszCommandLineTemplate[1] != 0)
    {
      switch (lpszCommandLineTemplate[1])
      {
      default:
        break;
      case '%':
        arguments += '%';
        break;
      case 'f':
      {
        PathName path;
        if (pimpl->session->FindFile(editFileName.ToString(), editFileType, path))
        {
          arguments += path.GetData();
        }
        else
        {
          arguments += editFileName.GetData();
        }
        break;
      }
      case 'h':
        // TODO
        break;
      case 't':
        arguments += transcriptFileName.GetData();
        break;
      case 'l':
        arguments += std::to_string(editLineNumber);
        break;
      case 'm':
        // TODO
        break;
      }
      lpszCommandLineTemplate += 2;
    }
    else
    {
      arguments += *lpszCommandLineTemplate;
      ++lpszCommandLineTemplate;
    }
  }

  Process::Start(fileName, arguments);
}

bool Application::GetQuietFlag() const
{
  return pimpl->beQuiet;
}

void Application::SetQuietFlag(bool b)
{
  pimpl->beQuiet = b;
}

shared_ptr<Session> Application::GetSession() const
{
  if (!pimpl->session)
  {
    MIKTEX_UNEXPECTED();
  }
  return pimpl->session;
}

bool Application::IsLog4cxxConfigured() const
{
  return pimpl->isLog4cxxConfigured;
}
