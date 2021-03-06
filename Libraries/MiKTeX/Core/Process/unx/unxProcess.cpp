/* unxProcess.cpp:

   Copyright (C) 1996-2017 Christian Schenk

   This file is part of the MiKTeX Core Library.

   The MiKTeX Core Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   The MiKTeX Core Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the MiKTeX Core Library; if not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA. */

#include "StdAfx.h"

#include "internal.h"

#include "miktex/Core/Directory.h"
#include "miktex/Core/CommandLineBuilder.h"
#include "miktex/Core/StreamReader.h"

#include "unxProcess.h"

#include "Session/SessionImpl.h"

using namespace MiKTeX::Core;
using namespace MiKTeX::Util;
using namespace std;

const int filenoStdin = 0;
const int filenoStdout = 1;
const int filenoStderr = 2;

#if defined(HAVE_FORK)
#  define FORK fork
#else
#  error this system does not have a working fork() function
#endif

#if defined(HAVE_VFORK)
#  define VFORK vfork
#else
#  define VFORK fork
#endif

MIKTEXSTATICFUNC(int) Dup(int fd)
{
  int dupfd = dup(fd);
  if (dupfd < 0)
  {
    MIKTEX_FATAL_CRT_ERROR("dup");
  }
  return dupfd;
}

MIKTEXSTATICFUNC(void) Dup2(int fd, int fd2)
{
  if (dup2(fd, fd2) < 0)
  {
    MIKTEX_FATAL_CRT_ERROR("dup2");
  }
}

MIKTEXSTATICFUNC(void) Close_(int fd)
{
  if (close(fd) < 0)
  {
    MIKTEX_FATAL_CRT_ERROR("close");
  }
}

class Pipe
{
public:
  ~Pipe() noexcept
  {
    try
    {
      Dispose();
    }
    catch (const exception&)
    {
    }
  }

public:
  void Create()
  {
    if (pipe(twofd) < 0)
    {
      MIKTEX_FATAL_CRT_ERROR("pipe");
    }
  }

public:
  int GetReadEnd() const
  {
    return twofd[0];
  }

public:
  int GetWriteEnd() const
  {
    return twofd[1];
  }

public:
  int StealReadEnd()
  {
    int fd = twofd[0];
    twofd[0] = -1;
    return fd;
  }

public:
  int StealWriteEnd()
  {
    int fd = twofd[1];
    twofd[1] = -1;
    return fd;
  }

public:
  void CloseReadEnd()
  {
    int fd = twofd[0];
    twofd[0] = -1;
    Close_(fd);
  }

public:
  void CloseWriteEnd()
  {
    int fd = twofd[1];
    twofd[1] = -1;
    Close_(fd);
  }

public:
  void Dispose()
  {
    if (twofd[0] >= 0)
    {
      CloseReadEnd();
    }
    if (twofd[1] >= 0)
    {
      CloseWriteEnd();
    }
  }

private:
  int twofd[2] = { -1, -1 };
};

Process* Process::Start(const ProcessStartInfo& startinfo)
{
  return new unxProcess(startinfo);
}

#if defined(NDEBUG)
#  define TRACEREDIR 0
#else
#  define TRACEREDIR 1
#endif

void unxProcess::Create()
{
  if (startinfo.FileName.empty())
  {
    MIKTEX_UNEXPECTED();
  }

  Argv argv(startinfo.FileName, startinfo.Arguments);

  Pipe pipeStdout;
  Pipe pipeStderr;
  Pipe pipeStdin;

  int fdChildStdin = -1;
  int fdChildStderr = -1;

  // create stdout pipe
  if (startinfo.RedirectStandardOutput)
  {
    pipeStdout.Create();
#if 1
    if (!startinfo.RedirectStandardError)
    {
      fdChildStderr = Dup(pipeStdout.GetWriteEnd());
    }
#endif
  }

  // create stderr pipe
  if (startinfo.RedirectStandardError)
  {
    pipeStderr.Create();
  }

  // use file descriptor or create stdin pipe
  if (startinfo.StandardInput != nullptr)
  {
    // caller feeds stdin via a stdio stream
    fdChildStdin = fileno(startinfo.StandardInput);
    if (fdChildStdin < 0)
    {
      MIKTEX_FATAL_CRT_ERROR("fileno");
    }
  }
  else if (startinfo.RedirectStandardInput)
  {
    pipeStdin.Create();
  }

  SessionImpl::GetSession()->UnloadFilenameDatabase();

  // fork
  SessionImpl::GetSession()->trace_process->WriteFormattedLine("core", T_("forking..."));
  if (pipeStdout.GetReadEnd() >= 0
    || pipeStderr.GetReadEnd() >= 0
    || pipeStdin.GetReadEnd() >= 0
    || fdChildStdin >= 0
    || fdChildStderr >= 0)
  {
    pid = FORK();
  }
  else
  {
    pid = VFORK();
  }
  if (pid < 0)
  {
    MIKTEX_FATAL_CRT_ERROR("fork");
  }

  if (pid == 0)
  {
    try
    {
      // I'm a child
      if (pipeStdout.GetWriteEnd() >= 0)
      {
        Dup2(pipeStdout.GetWriteEnd(), filenoStdout);
      }
      if (pipeStderr.GetWriteEnd() >= 0)
      {
        Dup2(pipeStderr.GetWriteEnd(), filenoStderr);
      }
      else if (fdChildStderr >= 0)
      {
        Dup2(fdChildStderr, filenoStderr);
        ::Close_(fdChildStderr);
      }
      if (pipeStdin.GetReadEnd() >= 0)
      {
        Dup2(pipeStdin.GetReadEnd(), filenoStdin);
      }
      else if (fdChildStdin >= 0)
      {
        Dup2(fdChildStdin, filenoStdin);
        ::Close_(fdChildStdin);
      }
      pipeStdout.Dispose();
      pipeStderr.Dispose();
      pipeStdin.Dispose();
      SessionImpl::GetSession()->SetEnvironmentVariables();
      SessionImpl::GetSession()->trace_process->WriteFormattedLine("core", "execv: %s", startinfo.FileName.c_str());
      for (int idx = 0; argv[idx] != nullptr; ++idx)
      {
        SessionImpl::GetSession()->trace_process->WriteFormattedLine("core", " argv[%d]: %s", idx, argv[idx]);
      }
      if (!startinfo.WorkingDirectory.empty())
      {
        Directory::SetCurrent(startinfo.WorkingDirectory);
      }
      execv(startinfo.FileName.c_str(), const_cast<char*const*>(argv.GetArgv()));
      perror("execv failed");
    }
    catch (const exception&)
    {
    }
    _exit(127);
  }

  MIKTEX_ASSERT(pid > 0);

  if (startinfo.RedirectStandardOutput)
  {
    fdStandardOutput = pipeStdout.StealReadEnd();
  }

  if (startinfo.RedirectStandardError)
  {
    fdStandardError = pipeStderr.StealReadEnd();
  }

  if (startinfo.RedirectStandardInput)
  {
    fdStandardInput = pipeStdin.StealWriteEnd();
  }

  if (fdChildStderr >= 0)
  {
    ::Close_(fdChildStderr);
  }

  pipeStdout.Dispose();
  pipeStderr.Dispose();
  pipeStdin.Dispose();
}

unxProcess::unxProcess(const ProcessStartInfo& startinfo) :
  startinfo(startinfo)
{
  Create();
}

unxProcess::~unxProcess()
{
  try
  {
    Close();
  }
  catch (const exception&)
  {
  }
}

void unxProcess::Close()
{
  if (fdStandardOutput >= 0)
  {
    close(fdStandardOutput);
    fdStandardOutput = -1;
  }
  if (fdStandardError >= 0)
  {
    close(fdStandardError);
    fdStandardError = -1;
  }
  if (fdStandardInput >= 0)
  {
    close(fdStandardInput);
    fdStandardInput = -1;
  }
  this->pid = -1;
}

FILE* unxProcess::get_StandardInput()
{
  if (pFileStandardInput != nullptr)
  {
    return pFileStandardInput;
  }
  if (fdStandardInput < 0)
  {
    return nullptr;
  }
  pFileStandardInput = FdOpen(fdStandardInput, "wb");
  fdStandardInput = -1;
  return pFileStandardInput;
}

FILE* unxProcess::get_StandardOutput()
{
  if (pFileStandardOutput != nullptr)
  {
    return pFileStandardOutput;
  }
  if (fdStandardOutput < 0)
  {
    return nullptr;
  }
  pFileStandardOutput = FdOpen(fdStandardOutput, "rb");
  fdStandardOutput = -1;
  return pFileStandardOutput;
}

FILE* unxProcess::get_StandardError()
{
  if (pFileStandardError != nullptr)
  {
    return pFileStandardError;
  }
  if (fdStandardError < 0)
  {
    return nullptr;
  }
  pFileStandardError = FdOpen(fdStandardError, "rb");
  fdStandardError = -1;
  return pFileStandardError;
}

void unxProcess::WaitForExit()
{
  if (this->pid > 0)
  {
    auto session = SessionImpl::GetSession();
    session->trace_process->WriteFormattedLine("core", T_("waiting for process %d"), static_cast<int>(this->pid));
    pid_t pid = this->pid;
    this->pid = -1;
    if (waitpid(pid, &status, 0) <= 0)
    {
      MIKTEX_FATAL_CRT_ERROR("waitpid");
    }
    if (WIFEXITED(status) != 0)
    {
      session->trace_process->WriteFormattedLine("core", T_("process %d exited with status %d"), static_cast<int>(pid), WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status) != 0)
    {
      session->trace_process->WriteFormattedLine("core", T_("process %d terminated due to signal %d"), static_cast<int>(pid), WTERMSIG(status));
    }
    else if (WIFSTOPPED(status) != 0)
    {
      session->trace_process->WriteFormattedLine("core", T_("process %d stopped due to signal %d"), static_cast<int>(pid), WSTOPSIG(status));
    }
    else if (WIFCONTINUED(status) != 0)
    {
      session->trace_process->WriteFormattedLine("core", T_("process %d continued"));
    }
  }
}

bool unxProcess::WaitForExit(int milliseconds)
{
  if (this->pid <= 0)
  {
    return true;
  }
  do
  {
    pid_t pid = waitpid(this->pid, &status, WNOHANG);
    if (pid == this->pid)
    {
      this->pid = -1;
      return true;
    }
    else if (pid < 0)
    {
      this->pid = -1;
      MIKTEX_FATAL_CRT_ERROR("waitpid");
    }
    MIKTEX_ASSERT(pid == 0);
    this_thread::sleep_for(chrono::milliseconds(1));
    --milliseconds;
  } while (milliseconds > 0);
  return false;
}

int unxProcess::get_ExitCode() const
{
  if (WIFEXITED(status) != 0)
  {
    return WEXITSTATUS(status);
  }
  else if (WIFSIGNALED(status) != 0)
  {
    MIKTEX_FATAL_ERROR_2(T_("Process terminated due to a signal."), "fileName", startinfo.FileName, "signal", std::to_string(WTERMSIG(status)));
  }
  else
  {
    MIKTEX_UNEXPECTED();
  }
}

string ConfStr(int name)
{
  size_t n = confstr(name, nullptr, 0);
  if (n == 0)
  {
    MIKTEX_FATAL_CRT_ERROR("confstr");
  }
  CharBuffer<char> result(n);
  n = confstr(name, result.GetData(), n);
  if (n == 0)
  {
    MIKTEX_FATAL_CRT_ERROR("confstr");
  }
  if (n > result.GetCapacity())
  {
    MIKTEX_UNEXPECTED();
  }
  return result.ToString();
}

MIKTEXSTATICFUNC(PathName) FindSystemShell()
{
#if defined(HAVE_CONFSTR) && defined(_CS_SHELL)
  return ConfStr(_CS_SHELL);
#else
  return "/bin/sh";
#endif
}

void Process::StartSystemCommand(const string& command)
{
  string arguments = "-c '";
  arguments += command;
  arguments += '\'';
  Process::Start(FindSystemShell(), arguments);
}

/*
 *
 * Process::ExecuteSystemCommand
 *
 * Start the shell (usually /bin/sh) with a given command string.
 * Pass output (stdout & stderr) to caller.
 *
 * Suppose the command string is: foo x | bar y > z
 *
 * Then we start as follows:
 *
 *   /bin/sh -c 'foo x | bar y > z'
 */
bool Process::ExecuteSystemCommand(const string& command, int* exitCode, IRunProcessCallback* callback, const char* directory)
{
  string arguments = "-c '";
  arguments += command;
  arguments += '\'';
  return Process::Run(FindSystemShell(), arguments, callback, exitCode, directory);
}

Process2* Process2::GetCurrentProcess()
{
  unxProcess* currentProcess = new unxProcess();
  currentProcess->pid = getpid();
  return currentProcess;
}

Process2* unxProcess::get_Parent()
{
#if defined(__linux__)
  string path = "/proc/" + std::to_string(pid) + "/stat";
  StreamReader reader(path);
  string line;
  while (reader.ReadLine(line))
  {
    Tokenizer tok(line, " ");
    ++tok;
    ++tok;
    ++tok;
    unxProcess* parentProcess = new unxProcess();
    parentProcess->pid = std::stoi(*tok);
    return parentProcess;
  }
#elif defined(__APPLE__)
  struct proc_bsdinfo procinfo;
  if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &procinfo, PROC_PIDTBSDINFO_SIZE) == 0)
  {
    return nullptr;
  }
  unxProcess* parentProcess = new unxProcess();
  parentProcess->pid = procinfo.pbi_ppid;
  return parentProcess;
#else
  return nullptr;
#endif
}

string unxProcess::get_ProcessName()
{
#if defined(__linux__)
  string path = "/proc/" + std::to_string(pid) + "/comm";
  StreamReader reader(path);
  string line;
  while (reader.ReadLine(line))
  {
    return line;
  }
#elif defined(__APPLE__)
  char path[PROC_PIDPATHINFO_MAXSIZE];
  if (proc_pidpath(pid, path, sizeof(path)) == 0)
  {
    return "?";
  }
  return PathName(path).GetFileName().ToString();
#else
  return "?";
#endif
}
