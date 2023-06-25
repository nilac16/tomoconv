#pragma once

#ifndef PATIENT_H
#define PATIENT_H

#include <string>
#include <pugixml.hpp>
#include "dbinfo.h"


namespace tomo {


class patient: public constructible {
    tomo::dbinfo m_dbinfo;

    std::string m_name;
    std::string m_mrn;
    std::string m_bday;
    std::string m_gender;


    tomo::dbinfo &dbinfo() noexcept { return m_dbinfo; }
    std::string &name() noexcept { return m_name; }
    std::string &mrn() noexcept { return m_mrn; }
    std::string &bday() noexcept { return m_bday; }
    std::string &gender() noexcept { return m_gender; }

public:
    patient();

    /** Constructed from the node "patient" */
    virtual void construct(pugi::xml_node root) override;

    const tomo::dbinfo &dbinfo() const noexcept { return m_dbinfo; }
    const std::string &name() const noexcept { return m_name; }
    const std::string &mrn() const noexcept { return m_mrn; }
    const std::string &bday() const noexcept { return m_bday; }
    const std::string &gender() const noexcept { return m_gender; }

    /** Returns the PatientSex DICOM code string appropriate for this patient */
    const char *dcmgender() const noexcept;

    /** Use the lookup library to find an updated MRN if possible. Throws a
     *  std::runtime_error if it fails
     */
    void update_mrn(const char *host, uint16_t port);
};


};


#endif /* PATIENT_H */
