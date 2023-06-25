#include "image.h"
#include "lookup.h"
#include "error.h"


void tomo::img_data::construct(pugi::xml_node root)
{
    img.construct(xchild(root, "image"));
}


tomo::image::array_header::array_header()
{
    
}


static void pugiread(int &res, pugi::xml_node node)
{
    res = node.text().as_int();
}

static void pugiread(float &res, pugi::xml_node node)
{
    res = node.text().as_float();
}


template <class ArrT, size_t Len>
static void load_array(std::array<ArrT, Len> &arr, pugi::xml_node root)
{
    size_t i = 0;

    for (pugi::xml_node node: root.children()) {
        pugiread(arr.at(i++), node);
    }
    if (i != Len) {
        std::stringstream ss;
        ss << "Extracted " << i << " elements for array " << root.name() << "; expected " << Len;
        throw std::runtime_error(ss.str());
    }
}


void tomo::image::array_header::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("binaryFileName", filename());
    xtable.insert("maxValue", max());
    xtable.insert("minValue", min());
    xtable.insert("useAlternateZs", use_altz());
    xtable.insert("compressionType", compression());
    xtable.insert("dataType", datatype());
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
    load_array(dim(), root.child("dimensions"));
    load_array(orig_axdim(), root.child("origAxialDimensions"));
    load_array(start(), root.child("start"));
    for (auto &x: start()) {
        x *= 10.0;
    }
    load_array(res(), root.child("elementSize"));
    for (auto &x: res()) {
        x *= 10.0;
    }
    for (pugi::xml_node node: root.child("originalZCoordinates").children()) {
        origz().push_back(node.text().as_float() * 10.0f);
    }
}


tomo::image::image()
{

}


void tomo::image::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("dbInfo", dbinfo());
    xtable.insert("frameOfReference", frame_of_ref());
    xtable.insert("patientPosition", pt_position());
    xtable.insert("imageType", image_type());
    xtable.insert("arrayHeader", header());
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}
