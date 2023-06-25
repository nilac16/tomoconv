#pragma once

#ifndef ERROR_H
#define ERROR_H

#include <filesystem>
#include <pugixml.hpp>
#include "lookup.h"


namespace tomo {


class missing_keys: public std::runtime_error {
public:
    using kvec_t = std::vector<pugi::string_t>;

private:
    pugi::xml_node m_root;
    kvec_t m_keys;

    kvec_t &keys() noexcept { return m_keys; }

public:
    missing_keys(pugi::xml_node root, const pugi::char_t *key);
    missing_keys(pugi::xml_node root, const xtable &table);

    pugi::xml_node &root() noexcept { return m_root; }
    const pugi::xml_node &root() const noexcept { return m_root; }

    const kvec_t &keys() const noexcept { return m_keys; }
};


/** I have to stop throwing the parse result, which does not derive from
 *  std::exception */
class parse_error: public std::runtime_error {
    pugi::xml_parse_result m_res;
    std::filesystem::path m_path;

public:
    parse_error(const pugi::xml_parse_result &res,
                const std::filesystem::path  &path);

    const pugi::xml_parse_result &res() const noexcept { return m_res; }
    const std::filesystem::path &path() const noexcept { return m_path; }
};


static inline pugi::xml_node xchild(pugi::xml_node root, const pugi::char_t *name)
{
    pugi::xml_node node;

    node = root.child(name);
    if (!node) {
        throw missing_keys(root, name);
    }
    return node;
}


};


#endif /* ERROR_H */
