#include "disease.h"
#include "lookup.h"
#include "error.h"
#include "log.h"

using namespace std::literals;


tomo::dcmstudy::dcmstudy()
{

}


void tomo::dcmstudy::construct(pugi::xml_node node)
{
    const std::unordered_map<pugi::string_t, bool> opts = {
        { "accessionNumber", true }
    };
    tomo::xtable xtable;

    node = xchild(node, "dicomStudy");
    xtable.insert("originalStudyUID", uid());
    xtable.insert("studyDescription", description());
    xtable.insert("accessionNumber", accession_num());
    xtable.insert("originalStudyDate", date());
    xtable.insert("originalStudyTime", time());
    xtable.search(node);

    xtable.remove(node, opts);
    if (xtable.size()) {
        throw missing_keys(node, xtable);
    }
}


void tomo::dcmstudy::set_default(const tomo::disease &dis)
{
    const std::string desc = "TomoTherapy Patient Disease"s;

    date() = dis.dbinfo().date();
    time() = dis.dbinfo().time();
    description() = desc;
    uid() = dis.dbinfo().uid();
}


void tomo::disease::info::construct(pugi::xml_node root)
{
    tomo::xtable xtable;
    pugi::xml_node node;

    ptage = xchild(root, "patientsAge").text().as_string();

    node = xchild(root, "briefDisease");
    xtable.insert("dbInfo", dbinfo);
    xtable.insert("diseaseName", name);
    xtable.search(node);
    if (xtable.size()) {
        throw missing_keys(node, xtable);
    }
}


tomo::disease::disease()
{

}


void tomo::disease::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("disease", m_info);
    xtable.insert("fullStructureSetDataArray", xref(m_structs, "fullStructureSetDataArray"));
    xtable.insert("fullDicomStudyDataArray", xref(dcm_studies(), "fullDicomStudyDataArray"));
    xtable.insert("fullPlanDataArray", xref(m_plans, "fullPlanDataArray"));
    xtable.insert("fullImageDataArray", xref(m_images, "fullImageDataArray"));
    xtable.search(root);

    if (xtable.remove("fullDicomStudyDataArray")) {
        log::puts(tomo::log::WARN, "Missing fullDicomStudyDataArray");
        dcm_studies().push_back({ });
        dcm_studies().back().set_default(*this);
    } else if (m_dcmstudies.size() > 1) {
        log::printf(tomo::log::WARN, "Found %zu DICOM studies", m_dcmstudies.size());
    }

    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
    if (!m_structs.size()) {
        throw std::runtime_error("No structure set information");
    }
    if (!m_plans.size()) {
        throw std::runtime_error("No plan information");
    }
    if (!m_images.size()) {
        throw std::runtime_error("No plan images");
    }
}
