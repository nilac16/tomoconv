#include <dcmtk/dcmdata/dctk.h>
#include "rtstruct.h"
#include "log.h"


void tomo::rtstruct::find_plan_image()
{
    const std::string &uid = structure_set().associated_img();
    unsigned i;

    for (i = 0; i < disease().n_images(); i++) {
        if (disease().image(i).dbinfo().uid() == uid) {
            m_image = &disease().image(i);
            break;
        }
    }
    if (!m_image) {
        throw std::runtime_error("Missing plan image");
    }
}


using seqptr_t = std::unique_ptr<DcmSequenceOfItems>;
using itemptr_t = std::unique_ptr<DcmItem>;


static seqptr_t make_contour_img_sequence(const std::string &uid, int ninsts)
{
    char uidbuf[65];
    itemptr_t item;
    seqptr_t seq;
    int i;
    
    seq.reset(new DcmSequenceOfItems(DCM_ContourImageSequence));
    for (i = 1; i <= ninsts; i++) {
        item.reset(new DcmItem);
        snprintf(uidbuf, sizeof uidbuf, "%s.%d", uid.c_str(), i);
        tomo::insert_wrap(item.get(), DCM_ReferencedSOPClassUID, UID_CTImageStorage);
        tomo::insert_wrap(item.get(), DCM_ReferencedSOPInstanceUID, uidbuf);
        tomo::insert_wrap(seq.get(), item.get());
        item.release();
    }
    return seq;
}


static seqptr_t make_ref_series_sequence(const std::string &uid, int ninsts)
{
    seqptr_t seq, contour;
    itemptr_t item;

    seq.reset(new DcmSequenceOfItems(DCM_RTReferencedSeriesSequence));
    item.reset(new DcmItem);
    contour = make_contour_img_sequence(uid, ninsts);
    tomo::insert_wrap(item.get(), DCM_SeriesInstanceUID, uid.c_str());
    tomo::insert_wrap(item.get(), contour.get());
    contour.release();
    tomo::insert_wrap(seq.get(), item.get());
    item.release();
    return seq;
}


static seqptr_t make_ref_study_sequence(const std::string &studyuid,
                                        const std::string &imguid,
                                        int                ninsts)
{
    seqptr_t seq, refseries;
    itemptr_t item;

    seq.reset(new DcmSequenceOfItems(DCM_RTReferencedStudySequence));
    item.reset(new DcmItem);
    refseries = make_ref_series_sequence(imguid, ninsts);
    tomo::insert_wrap(item.get(), DCM_ReferencedSOPClassUID, UID_RETIRED_DetachedStudyManagementSOPClass);
    tomo::insert_wrap(item.get(), DCM_ReferencedSOPInstanceUID, studyuid.c_str());
    tomo::insert_wrap(item.get(), refseries.get());
    refseries.release();
    tomo::insert_wrap(seq.get(), item.get());
    item.release();
    return seq;
}


void tomo::rtstruct::write_frame_of_ref_seq()
{
    itemptr_t item;
    seqptr_t seq;

    item.reset(new DcmItem);
    seq = make_ref_study_sequence(disease().dcm_studies()[0].uid(), image().dbinfo().uid(), image().header().dim(2));
    insert_wrap(item.get(), DCM_FrameOfReferenceUID, image().frame_of_ref().c_str());
    insert_wrap(item.get(), seq.get());
    seq.release();
    insert(DCM_ReferencedFrameOfReferenceSequence, item.get());
    item.release();
}


void tomo::rtstruct::write_structset_roi_seq()
{
    static const char *gen_algo = "MANUAL";
    itemptr_t item;
    seqptr_t rois;

    rois.reset(new DcmSequenceOfItems(DCM_StructureSetROISequence));
    for (auto &roi: structure_set().roilist()) {
        item.reset(new DcmItem);
        insert_wrap(item.get(), DCM_ROINumber, roi.number());
        insert_wrap(item.get(), DCM_ReferencedFrameOfReferenceUID, image().frame_of_ref().c_str());
        insert_wrap(item.get(), DCM_ROIName, roi.name());
        insert_wrap(item.get(), DCM_ROIGenerationAlgorithm, gen_algo);
        insert_wrap(rois.get(), item.get());
        item.release();
    }
    insert(rois.get());
    rois.release();
}


static seqptr_t make_contour_image_sequence(const std::string &ctuid,
                                            int                idx)
{
    char uidbuf[65];
    itemptr_t item;
    seqptr_t res;

    res.reset(new DcmSequenceOfItems(DCM_ContourImageSequence));
    item.reset(new DcmItem);
    snprintf(uidbuf, sizeof uidbuf, "%s.%d", ctuid.c_str(), idx);
    tomo::insert_wrap(item.get(), DCM_ReferencedSOPClassUID, UID_CTImageStorage);
    tomo::insert_wrap(item.get(), DCM_ReferencedSOPInstanceUID, uidbuf);
    tomo::insert_wrap(res.get(), item.get());
    item.release();
    return res;
}


/** Returns an empty unique_ptr if the roi is empty */
static seqptr_t make_contour_sequence(const tomo::roi             &roi,
                                      const std::string           &ctuid,
                                      const std::filesystem::path &dir)
{
    static const char *geom_type = "CLOSED_PLANAR";
    std::vector<tomo::roi::curve> curves;
    seqptr_t res = { }, imgseq;
    itemptr_t item;

    curves = roi.load_file(dir);
    res.reset(new DcmSequenceOfItems(DCM_ContourSequence));
    for (const auto &curve: curves) {
        if (!curve.data().size()) {
            continue;
        }
        item.reset(new DcmItem);
        imgseq = make_contour_image_sequence(ctuid, curve.instance_num());
        tomo::insert_wrap(item.get(), imgseq.get());
        imgseq.release();
        tomo::insert_wrap(item.get(), DCM_ContourGeometricType, geom_type);
        /* The Standard no longer requires these attributes under the
        RTStructureSet IOD, per PS3.3 2020e
        tomo::insert_wrap(item.get(), DCM_RETIRED_ContourSlabThickness, 3);
        tomo::insert_wrap(item.get(), DCM_RETIRED_ContourOffsetVector, 3, (const float[]){ 0, 0, 0 }); */
        tomo::insert_wrap(item.get(), DCM_NumberOfContourPoints, curve.data().size() / 3);
        tomo::insert_wrap(item.get(), DCM_ContourData, curve.data().size(), curve.data().data(), 3);
        tomo::insert_wrap(res.get(), item.get());
        item.release();
    }
    if (!res->card()) {
        /* If it's empty delete it */
        res.reset();
    }
    return res;
}


void tomo::rtstruct::write_roi_contour_seq()
{
    std::array<int, 3> color;
    seqptr_t rois, contour;
    itemptr_t item;

    rois.reset(new DcmSequenceOfItems(DCM_ROIContourSequence));
    for (const auto &roi: structure_set().roilist()) {
        contour = make_contour_sequence(roi, image().dbinfo().uid(), archive().dir());
        if (!contour) {
            /* This ROI is empty ("ct iso" in the test file is not included) */
            continue;
        }
        item.reset(new DcmItem);
        color[0] = roi.color().red;
        color[1] = roi.color().green;
        color[2] = roi.color().blue;
        insert_wrap(item.get(), DCM_ROIDisplayColor, color.size(), color.data());
        insert_wrap(item.get(), DCM_ReferencedROINumber, roi.number());
        insert_wrap(item.get(), contour.get());
        contour.release();
        insert_wrap(rois.get(), item.get());
        item.release();
    }
    insert(rois.get());
    rois.release();
}


void tomo::rtstruct::write_observations_seq()
{
    itemptr_t item;
    seqptr_t seq;

    seq.reset(new DcmSequenceOfItems(DCM_RTROIObservationsSequence));
    for (const auto &roi: structure_set().roilist()) {
        item.reset(new DcmItem);
        /* What even is the observation number? The ROI info on disk contains
        no other consistent integral types */
        insert_wrap(item.get(), DCM_ObservationNumber, roi.number());
        insert_wrap(item.get(), DCM_ReferencedROINumber, roi.number());
        insert_wrap(item.get(), DCM_RTROIInterpretedType, roi.interpreted_type());
        insert_wrap(item.get(), DCM_ROIInterpreter, (const char *)nullptr);
        insert_wrap(seq.get(), item.get());
        item.release();
    }
    insert(seq.get());
    seq.release();
}


void tomo::rtstruct::write_numeric_attributes()
{
    insert(DCM_InstanceNumber, 3);  /* ??????????????? */
}


void tomo::rtstruct::write_attributes()
{
    using pair_t = std::pair<DcmTag, const char *>;
    std::string series_inst = structure_set().dbinfo().uid() + ".1";
    std::string series_num = series_number(structure_set().dbinfo().date(), structure_set().dbinfo().time());
    const pair_t pairs[] = {
        { DCM_SOPClassUID, UID_RTStructureSetStorage },
        { DCM_SOPInstanceUID, structure_set().dbinfo().uid().c_str() },
        { DCM_Modality, "RTSTRUCT" },
        { DCM_SeriesInstanceUID, series_inst.c_str() },
        { DCM_SeriesNumber, series_num.c_str() },
        { DCM_StructureSetLabel, structure_set().label().c_str() },
        { DCM_StructureSetDate, structure_set().dbinfo().date().c_str() },
        { DCM_StructureSetTime, structure_set().dbinfo().time().c_str() }
    };

    for (auto &pair: pairs) {
        insert(pair.first, pair.second);
    }
    write_numeric_attributes();
    write_frame_of_ref_seq();
    write_structset_roi_seq();
    write_roi_contour_seq();
    write_observations_seq();
}


tomo::rtstruct::rtstruct(const tomo::archive   &arch,
                         const tomo::disease   &dis,
                         const tomo::structset &ss):
    tomo::dicom(arch, dis),
    m_structs(ss),
    m_image(nullptr)
{
    find_plan_image();
    write_attributes();
}


void tomo::rtstruct::flush(const std::filesystem::path &dir, bool dry_run)
{
    char fbuf[80];
    std::filesystem::path path(dir);

    snprintf(fbuf, sizeof fbuf, "RS%s.dcm", structure_set().dbinfo().uid().c_str());
    path.append(fbuf);
    if (!dry_run) {
        save_file(path);
    }
}
