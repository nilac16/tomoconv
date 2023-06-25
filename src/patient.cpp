#include "patient.h"
#include "lookup.h"
#include "auxiliary.h"
#include "error.h"

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
