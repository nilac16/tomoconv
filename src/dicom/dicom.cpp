#include <dcmtk/dcmdata/dctk.h>
#include "dicom.h"


tomo::dicom::insert_error::insert_error(const DcmTag &tag, OFCondition stat, const std::string &val):
    std::runtime_error("Failed to insert DICOM element"),
    m_key(tag),
    m_stat(stat),
    m_value(val)
{

}


std::string tomo::dicom::series_number(const std::string &date, const std::string &time)
{
    std::stringstream ss;

    if (date.length() > 4) {
        ss << date.c_str() + 4;
    }
    ss << time;
    return ss.str();
}


void tomo::dicom::insert(const DcmTag &key, const char *value)
{
    OFCondition stat;

    stat = dset()->putAndInsertString(key, value);
    if (stat.bad()) {
        throw insert_error(key, stat, value);
    }
}


template <class AppendT>
static std::string dicom_string(size_t n, const AppendT x[])
{
    std::stringstream ss;

    if (n) {
        ss << *x++;
        while (--n) {
            ss << '\\' << *x++;
        }
    }
    return ss.str();
}


void tomo::dicom::insert(const DcmTag &key, size_t n, const std::string str[])
{
    OFCondition stat;
    std::string val;

    val = dicom_string(n, str);
    stat = dset()->putAndInsertString(key, val.c_str());
    if (stat.bad()) {
        throw insert_error(key, stat, val);
    }
}


void tomo::dicom::insert(const DcmTag &key, size_t n, const long x[])
{
    OFCondition stat;
    std::string val;

    val = dicom_string(n, x);
    stat = dset()->putAndInsertString(key, val.c_str());
    if (stat.bad()) {
        throw insert_error(key, stat, val);
    }
}


void tomo::dicom::insert(const DcmTag &key, long x)
{
    insert(key, 1, &x);
}


void tomo::dicom::insert(const DcmTag &key, size_t n, const float x[])
{
    OFCondition stat;
    std::string val;

    val = dicom_string(n, x);
    stat = dset()->putAndInsertString(key, val.c_str());
    if (stat.bad()) {
        throw insert_error(key, stat, val);
    }
}


void tomo::dicom::insert(const DcmTag &key, float x)
{
    insert(key, 1, &x);
}


void tomo::dicom::insert(const DcmTag &key, size_t n, const double x[])
{
    OFCondition stat;
    std::string val;

    val = dicom_string(n, x);
    stat = dset()->putAndInsertString(key, val.c_str());
    if (stat.bad()) {
        throw insert_error(key, stat, val);
    }
}


void tomo::dicom::insert(const DcmTag &key, double x)
{
    insert(key, 1, &x);
}


void tomo::dicom::insert(const DcmTag &key, const DcmTag &val)
{
    OFCondition stat;

    stat = dset()->putAndInsertTagKey(key, val);
    if (stat.bad()) {
        throw insert_error(key, stat, val.toString().c_str());
    }
}


void tomo::dicom::insert(const DcmTag &key, DcmItem *item)
{
    OFCondition stat;

    stat = dset()->insertSequenceItem(key, item);
    if (stat.bad()) {
        throw insert_error(key, stat, "Sequence item");
    }
}


void tomo::dicom::insert(DcmElement *elem)
{
    OFCondition stat;
    std::string val;
    DcmTag tag;

    stat = dset()->insert(elem);
    if (stat.bad()) {
        tag = elem->getTag();
        elem->getOFString(val, 0);
        throw insert_error(tag, stat, val);
    }
}


void tomo::dicom::insert_pixels(const std::vector<uint16_t> &data)
{
    const Uint8 *pixels = reinterpret_cast<const Uint8 *>(data.data());
    size_t len = data.size() * sizeof data[0];
    OFCondition stat;

    stat = dset()->putAndInsertUint8Array(DCM_PixelData, pixels, len);
    if (stat.bad()) {
        std::stringstream ss;
        ss << len << " pixels";
        throw insert_error(DCM_PixelData, stat, ss.str());
    }
}


void tomo::dicom::save_file(const std::filesystem::path &path)
{
    OFCondition stat;

    stat = dcm().saveFile(path.string(), EXS_LittleEndianImplicit);
    if (stat.bad()) {
        throw std::runtime_error(stat.text());
    }
}


void tomo::dicom::write_patient_attributes()
{
    using pair_t = std::pair<DcmTag, const char *>;
    const pair_t pairs[] = {
        { DCM_SpecificCharacterSet, "ISO_IR 100" },
        { DCM_StudyDate, disease().dcm_studies()[0].date().c_str() },
        { DCM_StudyTime, disease().dcm_studies()[0].time().c_str() },
        { DCM_AccessionNumber, disease().dcm_studies()[0].accession_num().c_str() },
        { DCM_Manufacturer, "TomoTherapy Incorporated" },
        { DCM_ReferringPhysicianName, nullptr },
        { DCM_StudyDescription, disease().dcm_studies()[0].description().c_str() },
        { DCM_ManufacturerModelName, "Hi-Art" },    /* Not in ANY XML */
        { DCM_PatientName, archive().patient().name().c_str() },
        { DCM_PatientID, archive().patient().mrn().c_str() },
        { DCM_PatientBirthDate, archive().patient().bday().c_str() },
        { DCM_PatientSex, archive().patient().dcmgender() },
        { DCM_PatientAge, disease().pt_age().c_str() },
        { DCM_SoftwareVersions, "JH SIB tomoconv 0.2" },    /* I'm gonna hijack this key later */
        { DCM_StudyInstanceUID, disease().dcm_studies()[0].uid().c_str() },
        { DCM_StudyID, disease().name().c_str() }
    };

    for (const auto &pair: pairs) {
        insert(pair.first, pair.second);
    }
}


void tomo::dicom::write_current_datetime()
{
    OFString date, time;
    OFDateTime dt;

    if (!dt.setCurrentDateTime()
     || !dt.getDate().getISOFormattedDate(date, false)
     || !dt.getTime().getISOFormattedTime(time, true, false, false, false)) {
        throw std::runtime_error("Cannot get current date and time");
    }
    insert(DCM_InstanceCreationDate, date.c_str());
    insert(DCM_InstanceCreationTime, time.c_str());
}


tomo::dicom::dicom(const tomo::archive &arch, const tomo::disease &dis):
    m_dcm({ }),
    m_dset(m_dcm.getDataset()),
    m_arch(arch),
    m_disease(dis)
{
    write_patient_attributes();
    write_current_datetime();
}
