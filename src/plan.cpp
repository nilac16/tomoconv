#include "plan.h"
#include "lookup.h"
#include "error.h"
#include "log.h"

using namespace std::literals;


tomo::plan::trial::trial()
{

}


void tomo::plan::trial::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("doseVolumeList", xref(m_doses, "doseVolumeList"));
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


tomo::plan::delivery::delivery()
{

}


void tomo::plan::delivery::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    root = xchild(root, "deliveryReview");
    xtable.insert("dbInfo", dbinfo());
    xtable.insert("machineUID", machine_uid());
    xtable.insert("machineName", machine_name());
    xtable.insert("isPlanApproved", approved());
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


void tomo::plan::find_rp_uid()
{
    bool found = false;

    for (const auto &del: m_dlvryreview) {
        if (del.approved()) {
            if (found) {
                /* This is entirely possible I think. Need to get a reference
                case for this behavior */
                throw std::runtime_error("Multiple approved delivery review nodes");
            }
            rtplan_uid() = del.dbinfo().uid();
            found = true;
        }
    }
    if (!found) {
        throw std::runtime_error("No approved delivery review nodes");
    }
}


tomo::plan::plan()
{

}


void tomo::plan::construct_brief(pugi::xml_node brief)
{
    tomo::xtable xtable;

    xtable.insert("dbInfo", dbinfo());
    xtable.insert("planLabel", label());
    xtable.search(brief);
    if (xtable.size()) {
        throw missing_keys(brief, xtable);
    }
}


void tomo::plan::construct_plan(pugi::xml_node plan)
{
    tomo::xtable xtable;
    pugi::xml_node brief;

    brief = xchild(plan, "briefPlan");
    xtable.insert("beamletIVDT", beamlet_ivdt());
    xtable.insert("fullDoseIVDT", fulldose_ivdt());
    xtable.search(plan);
    if (xtable.size()) {
        throw missing_keys(plan, xtable);
    }
    construct_brief(brief);
}


void tomo::plan::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    construct_plan(xchild(root, "plan"));
    xtable.insert("plannedStructureSet", m_structs);
    xtable.insert("fullImageDataArray", xref(m_images, "fullImageDataArray"));
    xtable.insert("fullDeliveryReviewDataArray", xref(m_dlvryreview, "fullDeliveryReviewDataArray"));
    xtable.insert("fullPlanTrialArray", xref(m_trials, "fullPlanTrialArray"));
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
    /* These branches are not possible, since the xvector callback throws an
    exception if its result is empty */
    /* if (!m_images.size()) {
        throw std::runtime_error("Empty plan image sequence");
    }
    if (!m_trials.size()) {
        throw std::runtime_error("Empty plan trial sequence");
    } */
    find_rp_uid();
}
