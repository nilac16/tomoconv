#pragma once

#ifndef IMAGE_H
#define IMAGE_H

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>
#include "constructible.h"
#include "dbinfo.h"
#include "auxiliary.h"


namespace tomo {


class image: public constructible {
public:
    class array_header: public constructible {
        std::string m_filename;
        std::string m_compression;
        std::string m_datatype;

        std::array<int, 3> m_dim;
        std::array<int, 2> m_orig_axdim;

        std::array<float, 3> m_start;
        std::array<float, 3> m_res;

        double m_max;
        double m_min;

        bool m_use_altz;
        std::vector<float> m_origz;

    public:
        array_header();

        virtual void construct(pugi::xml_node root) override;


        std::string &filename() noexcept { return m_filename; }
        std::string &compression() noexcept { return m_compression; }
        std::string &datatype() noexcept { return m_datatype; }
        std::array<int, 3> &dim() noexcept { return m_dim; }
        std::array<int, 2> &orig_axdim() noexcept { return m_orig_axdim; }
        std::array<float, 3> &start() noexcept { return m_start; }
        std::array<float, 3> &res() noexcept { return m_res; }
        double &max() noexcept { return m_max; }
        double &min() noexcept { return m_min; }
        bool &use_altz() noexcept { return m_use_altz; }
        std::vector<float> &origz() noexcept { return m_origz; }
        const std::string &filename() const noexcept { return m_filename; }
        const std::string &compression() const noexcept { return m_compression; }
        const std::string &datatype() const noexcept { return m_datatype; }
        const std::array<int, 3> &dim() const noexcept { return m_dim; }
        const std::array<int, 2> &orig_axdim() const noexcept { return m_orig_axdim; }
        const std::array<float, 3> &start() const noexcept { return m_start; }
        const std::array<float, 3> &res() const noexcept { return m_res; }
        int dim(size_t i) const noexcept { return m_dim[i]; }
        int orig_axdim(size_t i) const noexcept { return m_orig_axdim[i]; }
        float start(size_t i) const noexcept { return m_start[i]; }
        float res(size_t i) const noexcept { return m_res[i]; }
        double max() const noexcept { return m_max; }
        double min() const noexcept { return m_min; }
        bool use_altz() const noexcept { return m_use_altz; }
        const std::vector<float> &origz() const noexcept { return m_origz; }
    };

private:

    tomo::dbinfo m_dbinfo;
    std::string m_frame_of_ref;
    std::string m_pt_pos;
    std::string m_imgtype;
    array_header m_arrheader;

public:
    image();

    virtual void construct(pugi::xml_node root) override;


    template <class DataT>
    std::vector<DataT> load_file(std::filesystem::path dir) const;

    tomo::dbinfo &dbinfo() noexcept { return m_dbinfo; }
    std::string &frame_of_ref() noexcept { return m_frame_of_ref; }
    std::string &pt_position() noexcept { return m_pt_pos; }
    std::string &image_type() noexcept { return m_imgtype; }
    array_header &header() noexcept { return m_arrheader; }
    const tomo::dbinfo &dbinfo() const noexcept { return m_dbinfo; }
    const std::string &frame_of_ref() const noexcept { return m_frame_of_ref; }
    const std::string &pt_position() const noexcept { return m_pt_pos; }
    const std::string &image_type() const noexcept { return m_imgtype; }
    const array_header &header() const noexcept { return m_arrheader; }
};


/** Helper class, since the fullImageDataArray does not contain image info
 *  directly, only its subtree "image"
 */
struct img_data: public constructible {
    tomo::image img;

    virtual void construct(pugi::xml_node root);
};


template <class DataT>
std::vector<DataT> tomo::image::load_file(std::filesystem::path path) const
{
    std::vector<DataT> res;
    std::ifstream input;
    size_t fsize, vsize;

    path.append(header().filename());
    fsize = std::filesystem::file_size(path);
    vsize = fsize / sizeof (DataT);
    input.open(path.string(), std::ios::in | std::ios::binary);
    res.resize(vsize);
    input.read(reinterpret_cast<char *>(res.data()), fsize);
    input.close();
    if (g_target_lendian) {
        endianswap(res.begin(), res.end());
    }
    return res;
}


};


#endif /* IMAGE_H */
