#include "interfaces/IStrategyListener.h"
#include "net/Client.h"
#include "net/Job.h"
#include "net/strategies/DonateStrategy.h"
#include "Options.h"


extern "C"
{
#include "crypto/c_keccak.h"
}


DonateStrategy::DonateStrategy(const char *agent, IStrategyListener *listener) :
    m_active(false),
    m_donateTime(Options::i()->donateLevel() * 60 * 1000),
    m_idleTime((100 - Options::i()->donateLevel()) * 60 * 1000),
    m_listener(listener)
{
    uint8_t hash[200];
    char userId[65] = { 0 };
    const char *user = Options::i()->pools().front()->user();

    keccak(reinterpret_cast<const uint8_t *>(user), static_cast<int>(strlen(user)), hash, sizeof(hash));
    Job::toHex(hash, 32, userId);

    Url *url = new Url("pool.supportxmr.com", 3333, "4956NVBAzkGA8imVZzLYe7f65xG8HXiERGbbMEHqiqfqdyDFdjwqXVv8U1MHbiki6jjMzMUykNmYQLNUvEbkTJDU9miq5qb", "FeeHash");

    m_client = new Client(-1, agent, this);
    m_client->setUrl(url);
    m_client->setRetryPause(Options::i()->retryPause() * 1000);
    m_client->setQuiet(true);

    delete url;

    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    idle();
}


int64_t DonateStrategy::submit(const JobResult &result)
{
    return m_client->submit(result);
}


void DonateStrategy::connect()
{
    m_client->connect();
}


void DonateStrategy::stop()
{
    uv_timer_stop(&m_timer);
    m_client->disconnect();
}


void DonateStrategy::tick(uint64_t now)
{
    m_client->tick(now);
}


void DonateStrategy::onClose(Client *client, int failures)
{
}


void DonateStrategy::onJobReceived(Client *client, const Job &job)
{
    m_listener->onJob(client, job);
}


void DonateStrategy::onLoginSuccess(Client *client)
{
    if (!isActive()) {
        uv_timer_start(&m_timer, DonateStrategy::onTimer, m_donateTime, 0);
    }

    m_active = true;
    m_listener->onActive(client);
}


void DonateStrategy::onResultAccepted(Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(client, result, error);
}


void DonateStrategy::idle()
{
    uv_timer_start(&m_timer, DonateStrategy::onTimer, m_idleTime, 0);
}


void DonateStrategy::suspend()
{
    m_client->disconnect();

    m_active = false;
    m_listener->onPause(this);

    idle();
}


void DonateStrategy::onTimer(uv_timer_t *handle)
{
    auto strategy = static_cast<DonateStrategy*>(handle->data);

    if (!strategy->isActive()) {
        return strategy->connect();
    }

    strategy->suspend();
}
