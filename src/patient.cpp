#include <cstring>
#include "patient.h"
#include "lookup.h"
#include "auxiliary.h"
#include "error.h"
#include "log.h"
#include "lookup/lookup.h"

using namespace std::literals;


tomo::patient::patient()
{

}


void tomo::patient::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("dbInfo", dbinfo());
    xtable.insert("patientName", name());
    xtable.insert("patientID", mrn());
    xtable.insert("patientBirthDate", bday());
    xtable.insert("patientGender", gender());
    
    root = xchild(root, "briefPatient");

    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


const char *tomo::patient::dcmgender() const noexcept
{
    const std::string male = "male"s, female = "female"s;

    if (strequal(gender(), male)) {
        return "M";
    } else if (strequal(gender(), female)) {
        return "F";
    } else {
        return "O";
    }
}


static std::vector<std::string> parse_reply(const char *reply)
{
    std::vector<std::string> res;
    const char *rptr;

    rptr = strchr(reply, '\n');
    while (rptr) {
        res.push_back(std::string(reply, rptr));
        reply = rptr + 1;
        rptr = strchr(reply, '\n');
    }
    res.push_back(std::string(reply));
    return res;
}


void tomo::patient::update_mrn(const char *host, uint16_t port)
{
    std::vector<std::string> mrns;
    char buf[256];

    snprintf(buf, sizeof buf, "%s", name().c_str());
    lookup_name(host, port, buf, sizeof buf);
    mrns = parse_reply(buf);
    if (mrns.size() > 1) {
        throw std::runtime_error("Cannot map MRN: Patient name is not unique");
    }
    mrn().assign(mrns.front());
    log::printf(tomo::log::DEBUG, "Updated patient MRN to %s", mrn().c_str());
}
