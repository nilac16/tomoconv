#pragma once

#ifndef XML_LOOKUP_H
#define XML_LOOKUP_H

#include <iostream>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <pugixml.hpp>
#include "constructible.h"

/** The operation that has evolved appears to be:
 *   :: Insert keys of interest
 *   :: Search (using the mthod)
 *   :: Delete optional keys
 *   :: Check the table size
 * 
 *  Once optional keys are deleted, the table size will be nonzero irrespective
 *  of whether the optional keys were found. xtable::remove returns nonzero if
 *  the key exists, but provides no further information
 */


namespace tomo {


class xref {
    void (xref::*m_func)(pugi::xml_node);
    const pugi::string_t m_vkey;
    void *m_ptr;

    void read_int(pugi::xml_node node);
    void read_bool(pugi::xml_node node);
    void read_float(pugi::xml_node node);
    void read_double(pugi::xml_node node);
    void read_string(pugi::xml_node node);
    void read_subtree(pugi::xml_node node);

    template <class ConstructT>
    void read_vector(pugi::xml_node node)
    {
        using vec_t = std::vector<ConstructT>;
        vec_t *vec = reinterpret_cast<vec_t *>(ptr());
        std::stringstream ss;

        vec->clear();
        for (pugi::xml_node child: node.children()) {
            if (child.name() == vkey()) {
                vec->push_back({ });
                dynamic_cast<constructible &>(vec->back()).construct(child);
            }
        }
        /* Oops I forgot about this behavior. It has never happened */
        if (!vec->size()) {
            ss << "Array list " << node.name() << " is empty";
            throw std::runtime_error(ss.str());
        }
    }

    void *ptr() noexcept { return m_ptr; }
    const pugi::string_t &vkey() const noexcept { return m_vkey; }

public:
    xref() = delete;
    /* No explicit constructors pl0x, there's too much typing in this language
    as it is already */
    xref(int &x): m_func(&xref::read_int), m_ptr(&x) { }
    xref(bool &b): m_func(&xref::read_bool), m_ptr(&b) { }
    xref(float &flt): m_func(&xref::read_float), m_ptr(&flt) { }
    xref(double &dub): m_func(&xref::read_double), m_ptr(&dub) { }
    xref(std::string &str): m_func(&xref::read_string), m_ptr(&str) { }
    xref(constructible &cons): m_func(&xref::read_subtree), m_ptr(&cons) { }

    template <class ConstructT>
    xref(std::vector<ConstructT> &vec, const pugi::string_t &key):
        m_func(&xref::read_vector<ConstructT>),
        m_vkey(key),
        m_ptr(&vec)
    {
        
    }

    void exec(pugi::xml_node node) { (this->*m_func)(node); }
};


class xtable {
    using tbl_t = std::unordered_map<pugi::string_t, xref>;
    tbl_t m_tbl;

    tbl_t &table() noexcept { return m_tbl; }
    const tbl_t &table() const noexcept { return m_tbl; }

public:
    xtable();

    void insert(const pugi::string_t &key, const xref &ref) { table().insert({ key, ref }); }

    bool remove(const pugi::string_t &key) { return table().erase(key); }


    /** @brief Removes optional keys in @p opt from this table, issuing warning
     *      messages to log files if requested
     *  @param root
     *      Name of root node
     *  @param opts
     *      Set of optional keys to be removed. The boolean value mapped to this
     *      key determines whether this function issues a warning message to log
     *      files
     */
    void remove(pugi::xml_node                                  root,
                const std::unordered_map<pugi::string_t, bool> &opts);


    /** @brief Search for every node in the table and execute the relevant
     *      function when found
     *  @param root
     *      Root node to search
     */
    void search(pugi::xml_node root);

    size_t size() const noexcept { return table().size(); }
    tbl_t::const_iterator begin() const noexcept { return table().begin(); }
    tbl_t::const_iterator end() const noexcept { return table().end(); }
};


};


#endif /* XML_LOOKUP_H */
