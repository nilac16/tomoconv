#pragma once

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <filesystem>
#include <vector>
#include <pugixml.hpp>
#include "patient.h"
#include "machine.h"
#include "disease.h"
#include "error.h"


namespace tomo {


class archive {
    std::filesystem::path m_archdir;

    pugi::xml_document m_ptroot;    /* Patient XML document root */

    tomo::machine m_machine;
    tomo::patient m_patient;
    std::vector<tomo::disease> m_diseases;


    /** Finds the machine file */
    void load_machine();

    /** Loads common data from the patient XML */
    void load_common();

    std::filesystem::path &dir() noexcept { return m_archdir; }
    
    pugi::xml_document &pt_doc() noexcept { return m_ptroot; }

    tomo::machine &machine() noexcept { return m_machine; }
    tomo::patient &patient() noexcept { return m_patient; }
    std::vector<tomo::disease> &diseases() noexcept { return m_diseases; }

public:
    archive();
    explicit archive(const std::filesystem::path &ptxml);


    /** @brief Loads the patient archive using the path to the patient's XML
     *  @param ptxml
     *      Path to patient XML
     *  @throws pugi::xml_parse_result on parse failure
     */
    void load_file(const std::filesystem::path &ptxml);


    /** @brief Writes the DICOM series to disk
     *  @param dir
     *      Directory to write each file to
     *  @param dry_run
     *      Proceed as normal, but do NOT write the files to disk. This can help
     *      with testing
     */
    void flush(const std::filesystem::path &dir = ".", bool dry_run = false);


    const tomo::machine &machine() const noexcept { return m_machine; }
    const tomo::patient &patient() const noexcept { return m_patient; }
    const std::vector<tomo::disease> &diseases() const noexcept { return m_diseases; }

    const pugi::xml_document &ptdoc() const noexcept { return m_ptroot; }

    const std::filesystem::path &dir() const noexcept { return m_archdir; }
};


};


#endif /* ARCHIVE_H */
