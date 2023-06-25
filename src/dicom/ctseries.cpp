#include <dcmtk/dcmdata/dctk.h>
#include <cmath>
#include "ctseries.h"

using namespace std::literals;


void tomo::ctseries::calc_geometry() noexcept
{
    image_position(0) = image().header().start(0);
    image_position(1) = -std::fma(image().header().dim(1) - 1,
                                  image().header().res(1),
                                  image().header().start(1));
    /* This may be inducing a fencepost error in the registration between plan
    CT images and the RTDose file... There is literally no way for me to check
    this, since the tomo station doesn't properly export the dose file */
    image_position(2) = -image().header().start(2);
    frame_len() = (size_t)image().header().dim(0)
                        * image().header().dim(1);
}


void tomo::ctseries::load_pixel_data()
{
    /* If this conversion ever fails I'm going to have to rewrite this entire
    class as a template... */
    if (image().header().datatype() != "Short_Data") {
        throw std::runtime_error("Unsupported CT data type: "s + image().header().datatype());
    }
    px_data() = image().load_file<uint16_t>(archive().dir());
}


void tomo::ctseries::write_numeric_attributes()
{
    std::array<float, 6> orient = { 1, 0, 0, 0, 1, 0 };

    insert(DCM_SliceThickness, image().header().res(2));
    insert(DCM_ImageOrientationPatient, orient.size(), orient.data());
    insert(DCM_ImagesInAcquisition, nframes());
    insert(DCM_SamplesPerPixel, 1);
    insert(DCM_Rows, image().header().dim(1));
    insert(DCM_Columns, image().header().dim(0));
    insert(DCM_PixelSpacing, 2, image().header().res().data());
    insert(DCM_BitsAllocated, 16);
    insert(DCM_BitsStored, 16);
    insert(DCM_HighBit, 15);
    insert(DCM_PixelRepresentation, 0);
    insert(DCM_RescaleIntercept, -1024.0);
    insert(DCM_RescaleSlope, 1.0);
}


void tomo::ctseries::write_attributes()
{
    using pair_t = std::pair<DcmTag, const char *>;
    const pair_t pairs[] = {
        { DCM_ImageType, "ORIGINAL\\SECONDARY\\AXIAL" },
        { DCM_SOPClassUID, UID_CTImageStorage },
        { DCM_SeriesDate, image().dbinfo().date().c_str() },
        { DCM_AcquisitionDate, image().dbinfo().date().c_str() },
        { DCM_SeriesTime, image().dbinfo().time().c_str() },
        { DCM_AcquisitionTime, image().dbinfo().time().c_str() },
        { DCM_Modality, "CT" },
        { DCM_SeriesDescription, "kVCT Image Set" },
        { DCM_DerivationDescription, "Resampled kVCT data set" },
        { DCM_ContrastBolusAgent, nullptr },
        { DCM_KVP, nullptr },
        { DCM_PatientPosition, image().pt_position().c_str() },
        { DCM_SeriesInstanceUID, image().dbinfo().uid().c_str() },
        { DCM_SeriesNumber, series_number(image().dbinfo().date(), image().dbinfo().time()).c_str() },
        { DCM_AcquisitionNumber, nullptr },
        { DCM_FrameOfReferenceUID, image().frame_of_ref().c_str() },
        { DCM_PositionReferenceIndicator, nullptr },
        { DCM_PhotometricInterpretation, "MONOCHROME2" }
    };

    for (const auto &pair: pairs) {
        insert(pair.first, pair.second);
    }
    write_numeric_attributes();
}


tomo::ctseries::ctseries(const tomo::archive &arch,
                         const tomo::disease &dis,
                         const tomo::image   &img):
    tomo::dicom(arch, dis),
    m_image(img)
{
    if (image().image_type() != "KVCT"s) {
        throw std::runtime_error("Unexpected image type: " + image().image_type());
    }
    calc_geometry();
    load_pixel_data();
    write_attributes();
}


void tomo::ctseries::flush(const std::filesystem::path &dir, bool dry_run)
{
    std::filesystem::path path;
    std::vector<uint16_t> data;
    //std::vector<uint16_t>::const_iterator it, end;
    /* Windows does not like advancing iterators past the end */
    /* Ironic that I have to use pointers because of Windows */
    /* If I had access to Merge DICOM I wouldn't even be using C++ though */
    const uint16_t *it, *end;
    char buf[100];
    int inst;

    data.resize(frame_len());
    it = px_data().data();
    end = it + frame_len();
    for (inst = 1; inst <= nframes(); inst++) {
        /* Put this in another function */
        snprintf(buf, sizeof buf, "%s.%d", uid().c_str(), inst);
        insert(DCM_SOPInstanceUID, buf);
        insert(DCM_InstanceNumber, inst);
        insert(DCM_ImagePositionPatient, image_position().size(), image_position().data());
        insert(DCM_SliceLocation, -image_position(2));
        image_position(2) -= image().header().res(2);
        std::transform(it, end, data.begin(), [this](auto x){
            /* Leaving this as a std::transform just in case I do actually need
            to modulate the data later */
            return x;
        });
        it = end;
        end += frame_len();
        insert_pixels(data);
        snprintf(buf, sizeof buf, "CT%s.%d.dcm", uid().c_str(), inst);
        path = dir;
        path.append(buf);
        if (!dry_run) {
            save_file(path);
        }
    }
}
