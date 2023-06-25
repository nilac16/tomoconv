#pragma once

#ifndef DICOM_H
#define DICOM_H

#include <filesystem>
#include <dcmtk/dcmdata/dcfilefo.h>
#include "archive.h"


namespace tomo {


class dicom {
public:
    class insert_error: public std::runtime_error {
        DcmTag m_key;
        OFCondition m_stat;
        std::string m_value;

    public:
        insert_error(const DcmTag &key, OFCondition stat, const std::string &v);

        const DcmTag &key() const noexcept { return m_key; }
        const OFCondition &stat() const noexcept { return m_stat; }
        const std::string &value() const noexcept { return m_value; }
    };

protected:
    DcmFileFormat m_dcm;
    DcmDataset *m_dset;

    const tomo::archive &m_arch;
    const tomo::disease &m_disease;


    /** Produce a series number using an SOP-specific date and time string */
    static std::string series_number(const std::string &date, const std::string &time);

    void insert(const DcmTag &key, const char *value);
    void insert(const DcmTag &key, size_t n, const std::string str[]);

    void insert(const DcmTag &key, int x) { insert(key, (long)x); };
    void insert(const DcmTag &key, long x);
    void insert(const DcmTag &key, size_t n, const long x[]);

    void insert(const DcmTag &key, float x);
    void insert(const DcmTag &key, size_t n, const float x[]);

    void insert(const DcmTag &key, double x);
    void insert(const DcmTag &key, size_t n, const double x[]);

    /** Insert an element of VR AT */
    void insert(const DcmTag &key, const DcmTag &atval);

    /** Insert a sequence item */
    void insert(const DcmTag &key, DcmItem *item);
    void insert(DcmElement *elem);

    void insert_pixels(const std::vector<uint16_t> &data);
    void save_file(const std::filesystem::path &path);

    void write_patient_attributes();
    void write_current_datetime();

    DcmFileFormat &dcm() noexcept { return m_dcm; }
    DcmDataset *&dset() noexcept { return m_dset; }

    const tomo::archive &archive() const noexcept { return m_arch; }
    const tomo::disease &disease() const noexcept { return m_disease; }

public:
    dicom(const tomo::archive &arch, const tomo::disease &dis);

    virtual void flush(const std::filesystem::path &dir, bool dry_run) = 0;
};


/** @brief Insert @p key @p val pair into the DICOM item @p item
 *  @throws tomo::dicom::insert_error on failure
 */
static inline void insert_wrap(DcmItem *item, const DcmTag &key, const char *val)
{
    OFCondition stat;

    stat = item->putAndInsertString(key, val);
    if (stat.bad()) {
        throw tomo::dicom::insert_error(key, stat, val);
    }
}


template <class ValT>
static inline void insert_wrap(DcmItem *item, const DcmTag &key, ValT x)
{
    std::stringstream ss;
    OFCondition stat;

    ss << x;
    stat = item->putAndInsertString(key, ss.str().c_str());
    if (stat.bad()) {
        throw tomo::dicom::insert_error(key, stat, ss.str().c_str());
    }
}


template <class ValT>
static inline void insert_wrap(DcmItem *item, const DcmTag &key, size_t n, ValT x)
{
    std::stringstream ss;
    OFCondition stat;

    if (n) {
        ss << *x++;
        while (--n) {
            ss << '\\' << *x++;
        }
    }
    stat = item->putAndInsertString(key, ss.str().c_str());
    if (stat.bad()) {
        throw tomo::dicom::insert_error(key, stat, ss.str().c_str());
    }
}


/** If you're writing a lot of floating-point values with arbitrary precision
 *  you should use this and set the precision yourself
 */
template <class FloatT>
static inline void insert_wrap(DcmItem      *item,
                               const DcmTag &key,
                               size_t        n,
                               FloatT        x[],
                               int           precision)
{
    std::stringstream ss;
    OFCondition stat;

    static_assert(std::is_floating_point<FloatT>::value);

    if (n) {
        ss << std::setprecision(precision) << std::fixed;
        ss << *x++;
        while (--n) {
            ss << '\\' << *x++;
        }
    }
    stat = item->putAndInsertString(key, ss.str().c_str());
    if (stat.bad()) {
        throw tomo::dicom::insert_error(key, stat, ss.str().c_str());
    }
}


template <class SeqT, class InsT>
static void insert_wrap(SeqT *seq, InsT *insert)
{
    DcmTag seq_tag, insert_tag;
    OFCondition stat;

    stat = seq->insert(insert);
    if (stat.bad()) {
        seq_tag = seq->getTag();
        insert_tag = insert->getTag();
        throw tomo::dicom::insert_error(seq_tag, stat, insert_tag.getTagName());
    }
}


};


#endif /* DICOM_H */
