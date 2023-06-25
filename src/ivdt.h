#pragma once

#ifndef IVDT_H
#define IVDT_H

#include <array>
#include <filesystem>
#include <vector>
#include <pugixml.hpp>
#include "dbinfo.h"
#include "constructible.h"


namespace tomo {


class ivdt: public constructible {
    std::filesystem::path m_dir;

    tomo::dbinfo m_dbinfo;

    bool m_curcomm;
    bool m_latest;

    struct data: public constructible {
        std::string filename;
        std::string compression;
        std::string datatype;

        std::array<int, 2> dim;

        std::vector<float> data;

        virtual void construct(pugi::xml_node root) override;
    
    } m_data;


    std::filesystem::path &dir() noexcept { return m_dir; }

    tomo::dbinfo &dbinfo() noexcept { return m_dbinfo; }

    bool &currently_commissioned() noexcept { return m_curcomm; }
    bool &is_latest() noexcept { return m_latest; }

    std::vector<float> &data() noexcept { return m_data.data; }

public:
    ivdt();

    /** Attempts to load from a file
     *  @param xml
     *      Path to file
     *  @returns true on success, false otherwise
     */
    bool load(const std::filesystem::path &xml);

    /** Loads the data from the file */
    void load_data();

    /** From the node "imagingEquipment" */
    virtual void construct(pugi::xml_node root) override;


    const tomo::dbinfo &dbinfo() const noexcept { return m_dbinfo; }

    bool currently_commissioned() const noexcept { return m_curcomm; }
    bool is_latest() const noexcept { return m_latest; }

    const std::string &filename() const noexcept { return m_data.filename; }
    const std::string &compression() const noexcept { return m_data.compression; }
    const std::string &datatype() const noexcept { return m_data.datatype; }

    int dim(size_t i) const noexcept { return m_data.dim[i]; }
    const std::array<int, 2> &dim() const noexcept { return m_data.dim; }

    uint16_t operator()(uint16_t pixel);
};


};


#endif /* IVDT_H */
