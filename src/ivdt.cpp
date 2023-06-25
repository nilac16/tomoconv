#include <algorithm>
#include <fstream>
#include <utility>
#include "ivdt.h"
#include "aux.h"
#include "error.h"


void tomo::ivdt::data::construct(pugi::xml_node root)
{
    tomo::xtable xtable;
    pugi::xml_node node;
    unsigned i = 0;

    xtable.insert("sinogramDataFile", filename);
    xtable.insert("compressionType", compression);
    xtable.insert("dataType", datatype);
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
    node = xchild(root, "dimensions");
    for (pugi::xml_node child: node.children()) {
        if (i < dim.size()) {
            dim[i] = child.text().as_int();
        }
        i++;
    }
    if (i != dim.size()) {
        throw missing_keys(root, "dimensions");
    }
}


tomo::ivdt::ivdt()
{

}


bool tomo::ivdt::load(const std::filesystem::path &xml)
{
    pugi::xml_parse_result res;
    pugi::xml_document doc;
    pugi::xml_node node;

    res = doc.load_file(xml.string().c_str());
    if (!res) {
        return false;
    }
    try {
        node = doc.root().first_child();
        construct(xchild(xchild(node, "FullImagingEquipment"), "imagingEquipment"));
        dir() = xml;
        dir().remove_filename();
        return true;
    } catch (missing_keys &) {
        
    }
    return false;
}


static void print_ivdt(std::vector<float> &tbl, const std::array<int, 2> &dim)
{
    int i, j;

    for (j = 0; j < dim[1]; j++) {
        for (i = 0; i < dim[0]; i++) {
            printf("%8g\t", tbl[i + j * dim[1]]);
        }
        std::cout << '\n';
    }
}


void tomo::ivdt::load_data()
{
    std::filesystem::path file = dir();
    std::ifstream input;
    uintmax_t fsize;
    size_t xpect = (size_t)dim(0) * dim(1);

    file.append(filename());
    fsize = std::filesystem::file_size(file);
    if (fsize / sizeof data()[0] != xpect) {
        std::stringstream ss;
        ss << "Unexpected sinogram size: Dimensions " << dim(0) << 'x' << dim(1) << ", file size " << fsize << " bytes\n";
        throw std::runtime_error(ss.str());
    }
    std::cout << "Resizing to " << xpect << " elements\n";
    data().resize(xpect);
    input.open(file.string(), std::ios::in | std::ios::binary);
    input.read(reinterpret_cast<char *>(data().data()), fsize);
    input.close();
    for (auto &x: data()) {
        endianswap(x);
    }
    print_ivdt(data(), dim());
}


void tomo::ivdt::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("dbInfo", dbinfo());
    xtable.insert("isCurrentlyCommissioned", currently_commissioned());
    xtable.insert("isLatest", is_latest());
    xtable.insert("imagingEquipmentData", m_data);
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}
