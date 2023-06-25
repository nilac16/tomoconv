#pragma once

#ifndef DISEASE_H
#define DISEASE_H

#include <vector>
#include <pugixml.hpp>
#include "dbinfo.h"
#include "constructible.h"
#include "plan.h"
#include "image.h"


namespace tomo {


class dcmstudy;




class disease: public constructible {    
    struct info: public constructible {
        tomo::dbinfo dbinfo;

        std::string name;
        std::string ptage;

        virtual void construct(pugi::xml_node root) override;

    } m_info;


    /** This must be valid. If the tree is not found, we must find this
     *  information elsewhere:
     *    - StudyDate           # disease->create_ts->time
     *    - StudyTime           # disease->create_ts->time
     *    - StudyDescription    # plan label? That would not be correct if the
     *                            reference images are shared between beamsets
     *    - StudyInstanceUID    # disease->dbinfo->uid
     */
    std::vector<tomo::dcmstudy> m_dcmstudies;

    std::vector<tomo::plan> m_plans;
    std::vector<tomo::img_data> m_images;
    std::vector<tomo::structset> m_structs; /* This one contains the correct SOP
                                            instance UID for the RT structure
                                            set */


    tomo::dbinfo &dbinfo() noexcept { return m_info.dbinfo; }

    std::string &name() noexcept { return m_info.name; }
    std::string &pt_age() noexcept { return m_info.ptage; }

    std::vector<tomo::dcmstudy> &dcm_studies() noexcept { return m_dcmstudies; }
    
    tomo::plan &plan(size_t i) noexcept { return m_plans[i]; }
    tomo::image &image(size_t i) noexcept { return m_images[i].img; }

    tomo::structset &structure_set(size_t i) noexcept { return m_structs[i]; }

public:
    disease();

    /** Constructed from the LOWER node "fullDiseaseDataArray" */
    virtual void construct(pugi::xml_node root) override;


    const tomo::dbinfo &dbinfo() const noexcept { return m_info.dbinfo; }

    const std::string &name() const noexcept { return m_info.name; }
    const std::string &pt_age() const noexcept { return m_info.ptage; }

    const std::vector<tomo::dcmstudy> &dcm_studies() const noexcept { return m_dcmstudies; }

    size_t n_images() const noexcept { return m_images.size(); }
    const tomo::image &image(size_t i) const noexcept { return m_images[i].img; }
    const std::vector<tomo::img_data> &images() const noexcept { return m_images; }

    size_t n_plans() const noexcept { return m_plans.size(); }
    const tomo::plan &plan(size_t i) const noexcept { return m_plans[i]; }
    const std::vector<tomo::plan> &plans() const noexcept { return m_plans; }

    size_t n_structs() const noexcept { return m_structs.size(); }
    const tomo::structset &structure_set(size_t i) const noexcept { return m_structs[i]; }
    const std::vector<tomo::structset> &structure_sets() const noexcept { return m_structs; }
};


class dcmstudy: public constructible {
    std::string m_uid;
    std::string m_desc;
    std::string m_acc;
    std::string m_date;
    std::string m_time;


    std::string &uid() noexcept { return m_uid; }
    std::string &description() noexcept { return m_desc; }
    std::string &accession_num() noexcept { return m_acc; }
    std::string &date() noexcept { return m_date; }
    std::string &time() noexcept { return m_time; }

public:
    dcmstudy();

    virtual void construct(pugi::xml_node root) override;

    /** Use this if fullDicomStudyArray does not exist */
    void set_default(const tomo::disease &dis);


    const std::string &uid() const noexcept { return m_uid; }
    const std::string &description() const noexcept { return m_desc; }
    const std::string &accession_num() const noexcept { return m_acc; }
    const std::string &date() const noexcept { return m_date; }
    const std::string &time() const noexcept { return m_time; }
};


};


#endif /* DISEASE_H */
