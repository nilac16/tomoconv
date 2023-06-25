#include <cstdarg>
#include <list>
#include <utility>
#include "log.h"


static std::list<tomo::logfile *> logs = { };


void tomo::log::add(logfile &lf)
{
    logs.emplace_front(&lf);
}


void tomo::log::remove(const logfile &lf)
{
    for (auto it = logs.begin(); it != logs.end(); ++it) {
        if (*it == &lf) {
            logs.erase(it);
            break;
        }
    }
}


int tomo::log::puts(level lvl, const char *msg)
{
    int res = 0;

    for (auto *log: logs) {
        if (std::as_const(*log).threshold() <= lvl) {
            res = log->write(lvl, msg) || res;
        }
    }
    return res;
}


int tomo::log::printf(tomo::log::level lvl, const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    return log::puts(lvl, buf);
}


tomo::logfile::logfile(log::level threshold):
    m_threshold(threshold)
{

}


void tomo::logger::appender::clear()
{
    ss.str("");
    ss.clear();
}


tomo::logger::appender::appender(log::level lvl):
    lvl(lvl)
{

}


tomo::logger::appender::~appender()
{
    *this << FLUSH;
}


tomo::logger::appender &tomo::logger::appender::operator<<(logger::flag flag)
{
    switch (flag) {
    case FLUSH:
        log::puts(lvl, ss.str().c_str());
        clear();
        break;
    }
    return *this;
}


tomo::logger::appender tomo::logger::operator<<(log::level lvl)
{
    return appender(lvl);
}


tomo::logger tomo::log;
