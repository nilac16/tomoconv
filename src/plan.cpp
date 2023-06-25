#include "plan.h"
#include "lookup.h"
#include "error.h"


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
    xtable.insert("fullPlanTrialArray", xref(m_trials, "fullPlanTrialArray"));
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
    if (!m_images.size()) {
        throw std::runtime_error("Empty plan image sequence");
    }
    if (!m_trials.size()) {
        throw std::runtime_error("Empty plan trial sequence");
    }
    /* forgive me father for i have sinned */
    /* replace this with a function that finds the first approved one */
    rtplan_uid() = xchild(xchild(xchild(xchild(xchild(root, "fullDeliveryReviewDataArray"), "fullDeliveryReviewDataArray"), "deliveryReview"), "dbInfo"), "databaseUID").text().as_string();
}
