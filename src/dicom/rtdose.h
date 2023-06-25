#pragma once

#ifndef RTDOSE_H
#define RTDOSE_H

#include "dicom.h"
#include "image.h"
#include "plan.h"


namespace tomo {


class rtdose: public dicom {
    const tomo::plan &m_plan;

    const tomo::structset *m_structs;   /* The planned structure set */

    const tomo::image *m_image;         /* "finalModifiedAssociatedImage," which
                                        is used for attribute (0008,1140)
                                        ReferencedImageSequence. The pixel data
                                        referenced by this XML tree is empty (?) */

    const tomo::image *m_dose;          /* The dose volume itself */

    std::vector<float> m_pxdata;
    float m_gridscal;
    float m_pxmax;

    size_t m_framelen;

    std::array<float, 3> m_imgpos;

    int m_doseno;   /* The index of the dose volume in its list, used for
                    (0020,0013) InstanceNumber (I GUESS) */


    void compute_grid_scaling() noexcept;
    void load_dose();
    void calc_geometry() noexcept;

    void find_dose();
    void find_image();
    void find_structs() noexcept;
    void find_plan();

    void write_grid_frame_offset_vec();
    void write_numeric_attributes();
    void write_reference_img_seq();
    void write_reference_rtplan_seq();
    void write_reference_structset_seq();
    void write_attributes();


    const tomo::plan &plan() const noexcept { return m_plan; }
    const tomo::structset &structure_set() const noexcept { return *m_structs; }
    const tomo::image &image() const noexcept { return *m_image; }
    const tomo::image &dose() const noexcept { return *m_dose; }

    std::vector<float> &px_data() noexcept { return m_pxdata; }
    const std::vector<float> &px_data() const noexcept { return m_pxdata; }

    size_t &frame_len() noexcept { return m_framelen; }
    size_t frame_len() const noexcept { return m_framelen; }

    size_t nframes() const noexcept { return dose().header().dim(2); }

    float &dose_grid_scaling() noexcept { return m_gridscal; }
    float dose_grid_scaling() const noexcept { return m_gridscal; }

    std::array<float, 3> &image_position() noexcept { return m_imgpos; }
    const std::array<float, 3> &image_position() const noexcept { return m_imgpos; }
    float &image_position(size_t i) noexcept { return m_imgpos[i]; }
    float image_position(size_t i) const noexcept { return m_imgpos[i]; }

    int instance() const noexcept { return m_doseno + 1; }

public:
    rtdose(const tomo::archive &arch,
           const tomo::disease &dis,
           const tomo::plan    &plan);


    virtual void flush(const std::filesystem::path &dir, bool dry_run) override;

};


};


#endif /* RTDOSE_H */
