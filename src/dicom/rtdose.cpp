#include <dcmtk/dcmdata/dctk.h>
#include <cmath>
#include "rtdose.h"
#include "../log.h"

using namespace std::literals;


void tomo::rtdose::compute_grid_scaling() noexcept
{
    float max = 0.0f;

    std::for_each(px_data().begin(), px_data().end(),
        [&max](float x) {
            max = std::max(max, x);
        });
    /* Dose grid scaling will be different than tomo's because Java */
    dose_grid_scaling() = max / (float)UINT16_MAX;
}


void tomo::rtdose::load_dose()
{
    /* The same issue holds here as with the ctseries pixel data */
    if (dose().header().datatype() != "Float_Data") {
        throw std::runtime_error("Unsupported dose pixel type: " + dose().header().datatype());
    }
    px_data() = dose().load_file<float>(archive().dir());
    compute_grid_scaling();
}


void tomo::rtdose::calc_geometry() noexcept
{
    image_position(0) = dose().header().start(0);
    image_position(1) = -std::fma(dose().header().dim(1) - 1,
                                  dose().header().res(1),
                                  dose().header().start(1));
    image_position(2) = -dose().header().start(2);
                        /* -std::fma(dose().header().dim(2) - 1,
                                  dose().header().res(2),
                                  dose().header().start(2)); */
    frame_len() = (size_t)dose().header().dim(0) * dose().header().dim(1);
}


void tomo::rtdose::find_dose()
{
    const std::string query = "Opt_Dose_After_EOP"s;
    unsigned i, j = 0;

    for (i = 0; i < plan().ntrials(); i++) {
        for (j = 0; j < plan().trial(i).ndoses(); j++) {
            if (plan().trial(i).dose(j).image_type() == query) {
                m_dose = &plan().trial(i).dose(j);
                goto find_dose_l1;
            }
        }
    }
find_dose_l1:
    if (!m_dose) {
        throw std::runtime_error("Cannot find final optimized dose");
    }
    m_doseno = j;
    calc_geometry();
    load_dose();
}


void tomo::rtdose::find_image()
{
    const std::string &uid = structure_set().mod_associated_img();
    unsigned i;

    for (i = 0; i < plan().nimages(); i++) {
        if (plan().image(i).dbinfo().uid() == uid) {
            m_image = &plan().image(i);
            break;
        }
    }
    if (!m_image) {
        throw std::runtime_error("Cannot find plan reference image");
    }
}


void tomo::rtdose::find_structs() noexcept
{
    m_structs = &plan().structure_set();
}


void tomo::rtdose::find_plan()
{
    find_structs();
    find_image();
    find_dose();
}


void tomo::rtdose::write_grid_frame_offset_vec()
{
    std::vector<float> vec; /* gross */
    float z = 0.0f;
    int k;

    for (k = 0; k < dose().header().dim(2); k++) {
        vec.push_back(z);
        z -= dose().header().res(2);
    }
    insert(DCM_GridFrameOffsetVector, vec.size(), vec.data());
}


void tomo::rtdose::write_numeric_attributes()
{
    std::array<float, 6> orient = { 1, 0, 0, 0, 1, 0 };
    char buf[50];

    std::snprintf(buf, sizeof buf, "%e", dose_grid_scaling());
    insert(DCM_SliceThickness, dose().header().res(2));
    insert(DCM_InstanceNumber, instance());
    insert(DCM_ImagePositionPatient, image_position().size(), image_position().data());
    insert(DCM_ImageOrientationPatient, orient.size(), orient.data());
    insert(DCM_ImagesInAcquisition, 1);
    insert(DCM_SliceLocation, -image_position(2));
    insert(DCM_SamplesPerPixel, 1);
    insert(DCM_NumberOfFrames, dose().header().dim(2));
    insert(DCM_FrameIncrementPointer, DCM_GridFrameOffsetVector);
    insert(DCM_Rows, dose().header().dim(1));
    insert(DCM_Columns, dose().header().dim(0));
    insert(DCM_PixelSpacing, 2, dose().header().res().data());
    insert(DCM_BitsAllocated, 16);
    insert(DCM_BitsStored, 16);
    insert(DCM_HighBit, 15);
    insert(DCM_PixelRepresentation, 0);
    insert(DCM_DoseGridScaling, buf);
    write_grid_frame_offset_vec();
}


using seqptr_t = std::unique_ptr<DcmSequenceOfItems>;
using itemptr_t = std::unique_ptr<DcmItem>;


static itemptr_t make_ref_seq_item(const char *sop, const char *uid)
{
    itemptr_t res;

    res = itemptr_t(new DcmItem);
    tomo::insert_wrap(res.get(), DCM_ReferencedSOPClassUID, sop);
    tomo::insert_wrap(res.get(), DCM_ReferencedSOPInstanceUID, uid);
    return res;
}


void tomo::rtdose::write_reference_img_seq()
{
    const char *ctuid = image().dbinfo().uid().c_str();
    const int n = image().header().dim(2);
    itemptr_t item;
    seqptr_t seq;
    char uid[65];
    int i;

    seq = seqptr_t(new DcmSequenceOfItems(DCM_ReferencedImageSequence));
    for (i = 0; i < n; i++) {
        snprintf(uid, sizeof uid, "%s.%d", ctuid, i + 1);
        item = make_ref_seq_item(UID_CTImageStorage, uid);
        insert_wrap(seq.get(), item.get());
        item.release();
    }
    insert(seq.get());
    seq.release();
}


void tomo::rtdose::write_reference_rtplan_seq()
{
    itemptr_t item;
    
    item = make_ref_seq_item(UID_RTPlanStorage, plan().rtplan_uid().c_str());
    insert(DCM_ReferencedRTPlanSequence, item.get());
    item.release();
}


void tomo::rtdose::write_reference_structset_seq()
{
    itemptr_t item;
    
    item = make_ref_seq_item(UID_RTStructureSetStorage, structure_set().dbinfo().uid().c_str());
    insert(DCM_ReferencedStructureSetSequence, item.get());
    item.release();
}


void tomo::rtdose::write_attributes()
{
    using pair_t = std::pair<DcmTag, const char *>;
    std::string instuid = dose().dbinfo().uid() + ".1";
    std::string serieno = series_number(dose().dbinfo().date(), dose().dbinfo().time());
    const pair_t pairs[] = {
        { DCM_ImageType, "ORIGINAL\\PRIMARY\\AXIAL" },
        { DCM_SOPClassUID, UID_RTDoseStorage },
        { DCM_SOPInstanceUID, dose().dbinfo().uid().c_str() },
        { DCM_AcquisitionDate, dose().dbinfo().date().c_str() },
        { DCM_AcquisitionTime, dose().dbinfo().time().c_str() },
        { DCM_Modality, "RTDOSE" },
        { DCM_StationName, archive().machine().name().c_str() },
        { DCM_OperatorsName, nullptr },
        { DCM_SeriesDescription, "TomoTherapy Planned Dose" },
        { DCM_SeriesInstanceUID, instuid.c_str() },
        { DCM_SeriesNumber, serieno.c_str() },
        { DCM_FrameOfReferenceUID, dose().frame_of_ref().c_str() },
        { DCM_PositionReferenceIndicator, nullptr },
        { DCM_PhotometricInterpretation, "MONOCHROME2" },
        { DCM_DoseUnits, "GY" },
        { DCM_DoseType, "PHYSICAL" },
        { DCM_DoseComment, plan().label().c_str() },
        { DCM_DoseSummationType, "PLAN" },
        { DCM_TissueHeterogeneityCorrection, "ROI_OVERRIDE" },
    };

    for (const auto &pair: pairs) {
        insert(pair.first, pair.second);
    }
    write_numeric_attributes();
    write_reference_img_seq();
    write_reference_rtplan_seq();
    write_reference_structset_seq();
}


tomo::rtdose::rtdose(const tomo::archive &arch,
                     const tomo::disease &dis,
                     const tomo::plan    &plan):
    tomo::dicom(arch, dis),
    m_plan(plan),
    m_structs(nullptr),
    m_image(nullptr),
    m_dose(nullptr)
{
    find_plan();
    write_attributes();
}


void tomo::rtdose::flush(const std::filesystem::path &dir, bool dry_run)
{
    char fbuf[80];
    std::vector<uint16_t> data;
    std::filesystem::path path(dir);

    std::snprintf(fbuf, sizeof fbuf, "RD%s.dcm", dose().dbinfo().uid().c_str());
    path.append(fbuf);
    data.resize(px_data().size());
    std::transform(px_data().begin(), px_data().end(), data.begin(),
        [this](float x) {
            return static_cast<uint16_t>(x / dose_grid_scaling());
        });
    insert_pixels(data);
    if (!dry_run) {
        save_file(path);
    }
}
