#include <algorithm>
#include <iostream>
#include <set>
#include "archive.h"
#include "ctseries.h"
#include "rtdose.h"
#include "rtstruct.h"
#include "auxiliary.h"
#include "log.h"

using namespace std::literals;


void tomo::archive::load_machine()
{
    std::filesystem::directory_iterator dir_iter(dir());
    bool found = false;
    
    for (auto &file: dir_iter) {
        if (file.path().extension() == ".xml"
         && machine().load_file(file.path())) {
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("Cannot find machine XML");
    }
}


void tomo::archive::load_common()
{
    pugi::xml_node root;
    tomo::xtable xtable;
    
    root = pt_doc().root().first_child();
    root = xchild(root, "FullPatient");
    xtable.insert("patient", patient());
    xtable.insert("fullDiseaseDataArray", xref(diseases(), "fullDiseaseDataArray"));
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


/* void tomo::archive::find_machine()
{
    const std::string suffix = "machine.xml"s;
    std::filesystem::directory_iterator dir_iter(dir());
    std::set<std::filesystem::path> candidates;
    std::string path;

    for (auto &file: dir_iter) {
        path = file.path().string();
        if (file.path().extension() == ".xml" && has_suffix(path, suffix)) {
            candidates.insert(file.path());
        }
    }
    if (!candidates.size()) {
        throw std::runtime_error("Cannot find machine XML file");
    }
    load_machine(*candidates.begin());
}


void tomo::archive::load_machine(const std::filesystem::path &path)
{
    pugi::xml_parse_result res;

    res = machine().load_file(path.c_str());
    if (!res) {
        throw res;
    }
}


void tomo::archive::find_imaging_equipment()
{
    const std::string suffix = "imagingequipment.xml"s;
    const std::string substr = "kvct"s;
    std::filesystem::directory_iterator dir_iter(dir());
    std::set<std::filesystem::path> candidates;
    std::string path;

    for (auto &file: dir_iter) {
        path = file.path().string();
        if (file.path().extension() == ".xml"
         && has_suffix(path, suffix)
         && has_substr(path, substr)) {
            candidates.insert(file.path());
        }
    }
    if (!candidates.size()) {
        throw std::runtime_error("Cannot find imaging equipment XML file");
    }
    load_imaging_equipment(*candidates.begin());
}


void tomo::archive::load_imaging_equipment(const std::filesystem::path &path)
{
    pugi::xml_parse_result res;

    res = img_equip().load_file(path.c_str());
    if (!res) {
        throw res;
    }
} */


tomo::archive::archive()
{

}


tomo::archive::archive(const std::filesystem::path &ptxml)
{
    load_file(ptxml);
}


void tomo::archive::load_file(const std::filesystem::path &ptxml)
{
    pugi::xml_parse_result res;

    res = pt_doc().load_file(ptxml.c_str());
    if (!res) {
        throw parse_error(res, ptxml);
    }
    dir() = ptxml;
    dir().remove_filename();
    load_common();
    load_machine();
}


void tomo::archive::flush(const std::filesystem::path &dir, bool dry_run)
{
    std::set<std::string> uids;
    const std::string *uid;

    for (const auto &dis: diseases()) {
        log::printf(tomo::log::DEBUG, "Exporting disease %s", dis.name().c_str());
        for (const auto &img: dis.images()) {
            tomo::ctseries ct(*this, dis, img.img);

            uid = &img.img.dbinfo().uid();
            log::printf(tomo::log::DEBUG, "Exporting %s image %s", img.img.image_type().c_str(), uid->c_str());
            if (uids.find(*uid) == uids.end()) {
                ct.flush(dir, dry_run);
                uids.insert(*uid);
            } else {
                log::printf(tomo::log::WARN, "Repeated CT series UID: %s", uid->c_str());
            }
        }
        for (const auto &plan: dis.plans()) {
            tomo::rtdose rd(*this, dis, plan);

            log::printf(tomo::log::DEBUG, "Exporting plan %s", plan.label().c_str());
            rd.flush(dir, dry_run);
        }
        for (const auto &ss: dis.structure_sets()) {
            tomo::rtstruct rs(*this, dis, ss);

            log::printf(tomo::log::DEBUG, "Exporting structure set %s", ss.dbinfo().uid().c_str());
            rs.flush(dir, dry_run);
        }
    }
}
