#pragma once

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <filesystem>
#include <vector>
#include "constructible.h"
#include "dbinfo.h"


namespace tomo {


class roi;


class structset: public constructible {
    tomo::dbinfo m_dbinfo;

    std::string m_label;
    std::string m_assoc_img;
    std::string m_mod_assoc_img;

    std::vector<tomo::roi> m_roilist;


    tomo::dbinfo &dbinfo() noexcept { return m_dbinfo; }

    std::string &label() noexcept { return m_label; }
    std::string &associated_img() noexcept { return m_assoc_img; }
    std::string &mod_associated_img() noexcept { return m_mod_assoc_img; }

    tomo::roi &roi(size_t i) noexcept { return roilist()[i]; }

    std::vector<tomo::roi> &roilist() noexcept { return m_roilist; }

public:
    structset();

    /** From "fullStructureSetDataArray" (lower) (must contain structureSet and
     *  troiList subtrees) */
    virtual void construct(pugi::xml_node root) override;


    const tomo::dbinfo &dbinfo() const noexcept { return m_dbinfo; }

    const std::string &label() const noexcept { return m_label; }
    const std::string &associated_img() const noexcept { return m_assoc_img; }
    const std::string &mod_associated_img() const noexcept { return m_mod_assoc_img; }

    size_t nrois() const noexcept { return roilist().size(); }
    const tomo::roi &roi(size_t i) const noexcept { return roilist()[i]; }

    const std::vector<tomo::roi> &roilist() const noexcept { return m_roilist; }
};


class roi: public constructible {
public:
    class curve: public constructible {
        std::vector<float> m_trips;
        std::vector<int> m_attached;

        std::string m_orient;

        double m_sliceval;

        int m_curveindex;
        int m_sliceindex;


        /** I don't know the topology of this subtree, because I have not yet
         *  seen an example of it. If this node is not empty, this function will
         *  throw a std::runtime_error
         */
        void construct_attached_curves(pugi::xml_node root);

        /** wtf coordinate system are they using
         *  can i always just negate y and z
         *  I don't need to negate z for the CT/dose
         *  god wtf is happening with these files
         */
        void construct_point_data(pugi::xml_node root);

        std::string &orientation() noexcept { return m_orient; }

        double &slice_value() noexcept { return m_sliceval; }

        int &curve_index() noexcept { return m_curveindex; }
        int &slice_index() noexcept { return m_sliceindex; }

        std::vector<float> &data() noexcept { return m_trips; }
        std::vector<int> &attached_curves() noexcept { return m_attached; }

    public:
        curve();


        /** From the root node "ROICurve_*" */
        virtual void construct(pugi::xml_node root) override;


        const std::string &orientation() const noexcept { return m_orient; }

        double slice_value() const noexcept { return m_sliceval; }

        /** curveIndex. This one increases from 0 */
        int curve_index() const noexcept { return m_curveindex; }

        /** ~~slicePlaneIndex refers the the CT instance number~~
         *  slicePlaneIndex evidently refers to the plane index, which would be
         *  zero-start
         *  The instance number is this plus one
         *  Gonna make a new method for that
         */
        int slice_index() const noexcept { return m_sliceindex; }

        int instance_num() const noexcept { return slice_index() + 1; }

        const std::vector<float> &data() const noexcept { return m_trips; }
        const std::vector<int> &attached_curves() const noexcept { return m_attached; }
    };


    struct color: public constructible {
        int red, green, blue;

        virtual void construct(pugi::xml_node root) override;
    };

private:
    tomo::dbinfo m_dbinfo;

    int m_structnum;

    bool m_is_density_overridden;
    bool m_lies_on_interpolation;
    bool m_is_displayed;

    struct color m_color;

    std::string m_interpreted_type;
    std::string m_name;

    std::string m_filename;


    tomo::dbinfo &dbinfo() noexcept { return m_dbinfo; }

    int &number() noexcept { return m_structnum; }

    bool &is_density_overridden() noexcept { return m_is_density_overridden; }
    bool &lies_on_interpolation() noexcept { return m_lies_on_interpolation; }
    bool &is_displayed() noexcept { return m_is_displayed; }

    struct color &color() noexcept { return m_color; }

    std::string &interpreted_type() noexcept { return m_interpreted_type; }
    std::string &name() noexcept { return m_name; }

    std::string &filename() noexcept { return m_filename; }

public:
    roi();


    /** From the root node "troiList" (lower) */
    virtual void construct(pugi::xml_node root) override;


    std::vector<curve> load_file(const std::filesystem::path &dir) const;

    const tomo::dbinfo &dbinfo() const noexcept { return m_dbinfo; }

    int number() const noexcept { return m_structnum; }

    bool is_density_overridden() const noexcept { return m_is_density_overridden; }
    bool lies_on_interpolation() const noexcept { return m_lies_on_interpolation; }
    bool is_displayed() const noexcept { return m_is_displayed; }

    const struct color &color() const noexcept { return m_color; }

    /** Yeah, this one needs to be mapped to the correct DICOM code string
     *  This will throw a std::runtime_error if it sees a string it doesn't
     *  recognize
     */
    const char *interpreted_type() const;

    const std::string &name() const noexcept { return m_name; }

    const std::string &filename() const noexcept { return m_filename; }
};


};


#endif /* STRUCTURES_H */
