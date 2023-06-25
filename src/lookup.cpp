#include "lookup.h"
#include "log.h"


void tomo::xref::read_int(pugi::xml_node node)
{
    *reinterpret_cast<int *>(ptr()) = node.text().as_int();
}


void tomo::xref::read_bool(pugi::xml_node node)
{
    *reinterpret_cast<bool *>(ptr()) = node.text().as_bool();
}


void tomo::xref::read_float(pugi::xml_node node)
{
    *reinterpret_cast<float *>(ptr()) = node.text().as_float();
}


void tomo::xref::read_double(pugi::xml_node node)
{
    *reinterpret_cast<double *>(ptr()) = node.text().as_double();
}


void tomo::xref::read_string(pugi::xml_node node)
{
    *reinterpret_cast<std::string *>(ptr()) = node.text().as_string();
}


void tomo::xref::read_subtree(pugi::xml_node node)
{
    reinterpret_cast<constructible *>(ptr())->construct(node);
}


tomo::xtable::xtable()
{

}


void tomo::xtable::remove(pugi::xml_node                                  root,
                          const std::unordered_map<pugi::string_t, bool> &opts)
{
    bool first = true;

    for (const auto &opt: opts) {
        if (remove(opt.first) && opt.second) {
            if (first) {
                log::printf(tomo::log::WARN, "Tree %s is missing keys:", root.name());
                first = false;
            }
            log::printf(tomo::log::WARN, "   - %s", opt.first.c_str());
        }
    }
}


void tomo::xtable::search(pugi::xml_node root)
{
    tbl_t::iterator it;

    for (pugi::xml_node node: root.children()) {
        it = table().find(node.name());
        if (it != table().end()) {
            it->second.exec(node);
            table().erase(it);
        }
    }
}
