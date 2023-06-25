#pragma once

#ifndef RTSTRUCT_H
#define RTSTRUCT_H

#include "archive.h"
#include "dicom.h"


namespace tomo {


/** TODO: URGENT: Find a tool that visualizes ROIs. If the coordinate systems do
 *      not match, then I will need to correct that too
 */
class rtstruct: public dicom {
    const tomo::structset &m_structs;   /* This needs to be the one in disease
                                        root, not plandata */

    const tomo::image *m_image;         /* Plan CT. This must be found from the
                                        structure set key "associatedImage" */


    void find_plan_image();

    void write_frame_of_ref_seq();
    void write_structset_roi_seq();
    void write_roi_contour_seq();
    void write_observations_seq();

    void write_numeric_attributes();
    void write_attributes();


    const tomo::structset &structure_set() const noexcept { return m_structs; }
    const tomo::image &image() const noexcept { return *m_image; }

public:
    rtstruct(const tomo::archive   &arch,
             const tomo::disease   &dis,
             const tomo::structset &ss);

    virtual void flush(const std::filesystem::path &dir, bool dry_run) override;
};


};


#endif /* RTSTRUCT_H */
