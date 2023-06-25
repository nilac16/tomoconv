#pragma once

#ifndef MACHINE_H
#define MACHINE_H

#include <filesystem>
#include "constructible.h"


namespace tomo {


class machine: public constructible {
    std::string m_name;

    std::string &name() noexcept { return m_name; }

public:
    machine();

    /** Returns true if the file was successfully loaded */
    bool load_file(const std::filesystem::path &xml);

    /** From the node "fullMachine" */
    virtual void construct(pugi::xml_node root) override;

    const std::string &name() const noexcept { return m_name; }
};


};


#endif /* MACHINE_H */
