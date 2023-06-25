#pragma once

#ifndef PLAN_H
#define PLAN_H

#include <vector>
#include <pugixml.hpp>
#include "constructible.h"
#include "structures.h"
#include "image.h"
#include "dbinfo.h"


namespace tomo {


class plan: public constructible {
public:
    class trial: public constructible {
        std::vector<tomo::image> m_doses;

    public:
        trial();

        /** Constructed from root node "fullPlanTrialArray" (lower) */
        virtual void construct(pugi::xml_node root) override;

        size_t ndoses() const noexcept { return m_doses.size(); }
        const tomo::image &dose(size_t i) const noexcept { return m_doses[i]; }
    };

private:
    tomo::dbinfo m_dbinfo;

    std::string m_label;

    std::string m_beamletivdt;
    std::string m_fulldoseivdt;

    std::string m_rtplan_uid;   /* BURIED in deliveryReview */

    tomo::structset m_structs;  /* plannedStructureSet */

    std::vector<tomo::img_data> m_images;   /* fullImageDataArray */
    std::vector<trial> m_trials;            /* fullPlanTrialArray */


    tomo::dbinfo &dbinfo() noexcept { return m_dbinfo; }
    std::string &label() noexcept { return m_label; }

    std::string &beamlet_ivdt() noexcept { return m_beamletivdt; }
    std::string &fulldose_ivdt() noexcept { return m_fulldoseivdt; }
    std::string &rtplan_uid() noexcept { return m_rtplan_uid; }

    tomo::structset &structure_set() noexcept { return m_structs; }

public:
    plan();

    
    void construct_plan(pugi::xml_node plan);
    void construct_brief(pugi::xml_node brief);

    /* Constructed from the root node "fullPlanDataArray" (lower) */
    virtual void construct(pugi::xml_node root) override;


    const tomo::dbinfo &dbinfo() const noexcept { return m_dbinfo; }
    const std::string &label() const noexcept { return m_label; }

    const std::string &beamlet_ivdt() const noexcept { return m_beamletivdt; }
    const std::string &fulldose_ivdt() const noexcept { return m_fulldoseivdt; }
    const std::string &rtplan_uid() const noexcept { return m_rtplan_uid; }

    size_t nimages() const noexcept { return m_images.size(); }
    const tomo::image &image(size_t i) const noexcept { return m_images[i].img; }

    size_t ntrials() const noexcept { return m_trials.size(); }
    const trial &trial(size_t i) const noexcept { return m_trials[i]; }

    const tomo::structset &structure_set() const noexcept { return m_structs; }
};


};


#endif /* PLAN_H */
