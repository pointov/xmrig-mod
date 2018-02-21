#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>


#include "App.h"
#include "Cpu.h"
#include "log/Log.h"
#include "Options.h"


void App::background()
{
    if (m_options->affinity() != -1L) {
        Cpu::setAffinity(-1, m_options->affinity());
    }

    if (!m_options->background()) {
        return;
    }

    int i = fork();
    if (i < 0) {
        exit(1);
    }

    if (i > 0) {
        exit(0);
    }

    i = setsid();

    if (i < 0) {
        LOG_ERR("setsid() failed (errno = %d)", errno);
    }

    i = chdir("/");
    if (i < 0) {
        LOG_ERR("chdir() failed (errno = %d)", errno);
    }
}
