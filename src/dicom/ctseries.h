#pragma once

#ifndef CTSERIES_H
#define CTSERIES_H

#include "image.h"
#include "ivdt.h"
#include "dicom.h"
#include "archive.h"


namespace tomo {


class ctseries: public dicom {
    /** The CT volume itself. This is the only information required for this
     *  DICOM */
    const tomo::image &m_image;

    std::array<float, 3> m_imgpos;
    std::vector<uint16_t> m_pxdata;
    size_t m_framelen;


    /** @brief Convert from their coordinate system to DICOM's */
    void calc_geometry() noexcept;
    void load_pixel_data();

    void write_numeric_attributes();
    void write_attributes();


    std::array<float, 3> &image_position() noexcept { return m_imgpos; }
    const std::array<float, 3> &image_position() const noexcept { return m_imgpos; }
    float &image_position(size_t i) noexcept { return m_imgpos[i]; }
    float image_position(size_t i) const noexcept { return m_imgpos[i]; }

    size_t &frame_len() noexcept { return m_framelen; }
    size_t frame_len() const noexcept { return m_framelen; }

    int nframes() const noexcept { return image().header().dim(2); }

    const tomo::image &image() const noexcept { return m_image; }

    std::vector<uint16_t> &px_data() noexcept { return m_pxdata; }

public:
    ctseries() = delete;
    ctseries(const tomo::archive &arch,
             const tomo::disease &dis,
             const tomo::image   &img);


    /** @brief Writes the CT series to disk in @p dir */
    virtual void flush(const std::filesystem::path &dir, bool dry_run) override;


    /** @brief Gets the series instance UID of this CT volume */
    const std::string &uid() const { return image().dbinfo().uid(); }
};


};


#endif /* CTSERIES_H */
