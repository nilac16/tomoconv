#pragma once

#ifndef CONSTRUCTIBLE_H
#define CONSTRUCTIBLE_H

#include <pugixml.hpp>


namespace tomo {


class constructible {

public:
    constructible() = default;

    virtual void construct(pugi::xml_node root) = 0;
};


};


#endif /* CONSTRUCTIBLE_H */
