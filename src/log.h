#pragma once

#ifndef TOMO_LOG_H
#define TOMO_LOG_H

#include <iostream>
#include <sstream>


namespace tomo {


class logfile;


class log {
public:
    enum level {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    /** Adds @p lf to the log list */
    static void add(logfile &lf);

    /** Finds and removes @p lf from the log list */
    static void remove(const logfile &lf);

    /** @brief Issue @p msg to all logging classes
     *  @param lvl
     *      Logging level of this call
     *  @param msg
     *      Log message
     *  @returns Nonzero if any callback does
     */
    static int puts(level lvl, const char *msg);

    /** @brief Issue formatted output to all logging classes
     *  @param lvl
     *      Logging level of this call
     *  @param fmt
     *      ANSI format string. Be aware that this blindly calls vsnprintf, and
     *      the caller should take care to avoid potential uncontrolled format
     *      exploits
     *  @returns Nonzero if any callback does
     */
    static int printf(level lvl, const char *fmt, ...);
};


class logfile {
protected:
    log::level m_threshold;


    log::level &threshold() noexcept { return m_threshold; }

public:
    logfile(log::level threshold);

    log::level threshold() const noexcept { return m_threshold; }

    /** Return nonzero on error. Avoid exceptions if you can
     */
    virtual int write(log::level lvl, const char *msg) = 0;
};


class logger {
public:
    enum flag {
        FLUSH   /* Force everything written up to this point to be flushed to a
                new log line */
    };

    class appender {
        std::stringstream ss;
        log::level lvl;

        void clear();

    public:
        appender(log::level lvl);

        /** This will always flush, be wary */
        ~appender();

        template <class ItemT>
        appender &operator<<(ItemT item)
        {
            ss << item;
            return *this;
        }

        appender &operator<<(logger::flag flag);
    };

    logger() = default;

    appender operator<<(log::level lvl);
};


/** operator<< this a log::level before sending it further data
 *  unlike the printf function above this buffers the message on the heap using
 *  a stringstream, which is really not a good practice for a simple tracer
 */
extern logger log;


};


#endif /* TOMO_LOG_H */
