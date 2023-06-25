#include <utility>
#include "machine.h"
#include "error.h"


tomo::machine::machine()
{

}


bool tomo::machine::load_file(const std::filesystem::path &xml)
{
    pugi::xml_parse_result res;
    pugi::xml_document doc;
    pugi::xml_node node;

    res = doc.load_file(xml.string().c_str());
    if (!res) {
        return false;
    }
    node = doc.root().first_child().child("FullMachine");
    if (!node) {
        return false;
    }
    try {
        construct(node);
        return true;
    } catch (tomo::missing_keys &e) {
        /* std::cerr << "Cannot read machine XML, missing keys:\n";
        for (auto &key: std::as_const(e).keys()) {
            std::cerr << '\t' << key << '\n';
        } */
    }
    return false;
}


void tomo::machine::construct(pugi::xml_node root)
{
    pugi::xml_node node;

    node = xchild(xchild(root, "machine"), "briefMachine");
    node = xchild(node, "machineName");
    name() = node.text().as_string();
}
