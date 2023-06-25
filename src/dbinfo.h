#pragma once

#ifndef DBINFO_H
#define DBINFO_H

#include "constructible.h"


namespace tomo {


class dbinfo: public constructible {
    std::string m_uid;
    std::string m_date;
    std::string m_time;

public:
    dbinfo();

    virtual void construct(pugi::xml_node root) override;

    std::string &uid() noexcept { return m_uid; }
    std::string &date() noexcept { return m_date; }
    std::string &time() noexcept { return m_time; }
    const std::string &uid() const noexcept { return m_uid; }
    const std::string &date() const noexcept { return m_date; }
    const std::string &time() const noexcept { return m_time; }
};


};


#endif /* DBINFO_H */
