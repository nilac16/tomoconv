#include "error.h"


tomo::missing_keys::missing_keys(pugi::xml_node root, const pugi::char_t *key):
    std::runtime_error("Missing XML node"),
    m_root(root),
    m_keys({ key })
{

}


tomo::missing_keys::missing_keys(pugi::xml_node root, const xtable &tbl):
    std::runtime_error("Missing XML nodes"),
    m_root(root),
    m_keys({ })
{
    for (auto &pair: tbl) {
        keys().push_back(pair.first);
    }
}


tomo::parse_error::parse_error(const pugi::xml_parse_result &res,
                               const std::filesystem::path  &path):
    std::runtime_error("pugixml document parse error"),
    m_res(res),
    m_path(path)
{

}
