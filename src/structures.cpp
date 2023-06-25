#include <array>
#include <cstring>
#include <unordered_map>
#include "structures.h"
#include "auxiliary.h"
#include "error.h"

using namespace std::literals;


tomo::structset::structset()
{

}


void tomo::structset::construct(pugi::xml_node root)
{
    tomo::xtable xtable;
    pugi::xml_node node;

    node = xchild(root, "structureSet");
    xtable.insert("dbInfo", dbinfo());
    xtable.insert("structureSetLabel", label());
    xtable.insert("associatedImage", associated_img());
    xtable.insert("modifiedAssociatedImage", mod_associated_img());
    xtable.search(node);
    if (xtable.size()) {
        throw missing_keys(node, xtable);
    }

    xtable.insert("troiList", xref(roilist(), "troiList"));
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


void tomo::roi::color::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    xtable.insert("red", red);
    xtable.insert("green", green);
    xtable.insert("blue", blue);
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


tomo::roi::roi()
{

}


void tomo::roi::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    filename() = xchild(root, "curveDataFile").text().as_string();
    root = xchild(root, "briefROI");
    xtable.insert("dbInfo", dbinfo());
    xtable.insert("structureNumber", number());
    xtable.insert("interpretedType", interpreted_type());
    xtable.insert("name", name());
    xtable.insert("color", color());
    xtable.insert("isDensityOverridden", is_density_overridden());
    xtable.insert("liesOnInterpolatedSlices", lies_on_interpolation());
    xtable.insert("isDisplayed", is_displayed());
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


void tomo::roi::curve::construct_attached_curves(pugi::xml_node root)
{
    if (!root.first_child()) {
        return;
    }
    throw std::runtime_error("attachedCurves subtree is not empty!");
    for (auto &node: root.children()) {
        attached_curves().push_back(node.text().as_int());
    }
}


static float pop_float(const char *text, char **endptr, char delim)
{
    float res;

    /* Convert to millimeters here */
    res = 10.0f * (float)strtod(text, endptr);
    if (**endptr != delim) {
        throw std::runtime_error("Malformed ROI curve triplet");
    }
    return res;
}


/** Pops a triplet into @p trip and advances @p text to the next triplet. At the
 *  end, sets @p text to NULL. If a triplet is not formatted correctly, this
 *  function throws a std::runtime_error
 */
static void parse_triplet(const pugi::char_t  **text,
                          std::array<float, 3> &trip)
{
    pugi::char_t *endptr;

    /* They appear to use a coordinate system with y and z flipped about an
    origin common to DICOM's patient-specific system. I should fact-check this
    before deployment */
    trip[0] = pop_float(*text, &endptr, ',');
    trip[1] = -pop_float(endptr + 1, &endptr, ',');
    trip[2] = -pop_float(endptr + 1, &endptr, ';');
    while (isspace(*++endptr)) { }
    *text = (*endptr) ? endptr : nullptr;
}


void tomo::roi::curve::construct_point_data(pugi::xml_node root)
/** We're doing this the (sorta) C way */
{
    std::array<float, 3> triplet;
    const pugi::char_t *text;

    text = root.text().as_string(nullptr);
    while (text) {
        parse_triplet(&text, triplet);
        for (float trip: triplet) {
            data().push_back(trip);
        }
    }
}


tomo::roi::curve::curve()
{

}


void tomo::roi::curve::construct(pugi::xml_node root)
{
    tomo::xtable xtable;

    construct_attached_curves(root.child("attachedCurves"));
    construct_point_data(xchild(root, "pointData"));

    xtable.insert("curveIndex", curve_index());
    xtable.insert("sliceOrientation", orientation());
    xtable.insert("sliceValue", slice_value());
    xtable.insert("slicePlaneIndex", slice_index());
    xtable.search(root);
    if (xtable.size()) {
        throw missing_keys(root, xtable);
    }
}


std::vector<tomo::roi::curve>
tomo::roi::load_file(const std::filesystem::path &dir) const
/** Holy moly look at all those declarations
 *  If only there were a reasonable way to break up a function's
 *  responsibilities and reduce the size of its alphabet
 */
{
    const pugi::string_t prefix = "ROICurve_"s;
    std::filesystem::path path = dir;
    pugi::xml_parse_result res;
    std::vector<curve> curves;
    pugi::xml_document doc;
    pugi::xml_node root;
    pugi::string_t name;
    tomo::xtable xtable;

    path.append(filename());
    res = doc.load_file(path.string().c_str());
    if (!res) {
        throw parse_error(res, path);
    }
    root = xchild(doc.root(), "ROICurves");
    for (pugi::xml_node node: root.children()) {
        name = node.name();
        if (prefixed(name, prefix)) {
            curves.push_back({ });
            curves.back().construct(node);
        }
    }
    return curves;
}


const char *tomo::roi::interpreted_type() const
{
    /* I have no idea what these mappings actually are, so I'm throwing an
    exception if a string is unrecognized */
    using map_t = std::unordered_map<std::string, const char *>;
    static const map_t map = {
        { "ROI_Null_Valued",  nullptr },    /* Special key used by Tomo */
        { "External",        "EXTERNAL" },
        { "PTV",             "PTV" },
        { "CTV",             "CTV" },
        { "GTV",             "GTV" },
        { "TreatedVolume",   "TREATED_VOLUME" },
        { "IrradVolume",     "IRRAD_VOLUME" },
        { "Bolus",           "BOLUS" },
        { "Avoidance",       "AVOIDANCE" },
        { "Organ",           "ORGAN" },
        { "Marker",          "MARKER" },
        { "Registration",    "REGISTRATION" },
        { "Isocenter",       "ISOCENTER" },
        { "ContrastAgent",   "CONTRAST_AGENT" },
        { "Cavity",          "CAVITY" },
        { "BrachyChannel",   "BRACHY_CHANNEL" },
        { "BrachyAccessory", "BRACHY_ACCESSORY" },
        /* Really unsure about these. They fit the pattern, but it is unlike
        Java programmers to be so terse in their abbreviations */
        { "BrachySrcApp",    "BRACHY_SRC_APP" },
        { "BrachyChnlShld",  "BRACHY_CHNL_SHLD" },
        /* I would be utterly unsurprised to learn that it is actually
        BrachySourceApplication and BrachyChannelShield */
        { "Support",         "SUPPORT" },
        { "Fixation",        "FIXATION" },
        { "DoseRegion",      "DOSE_REGION" },
        { "Control",         "CONTROL" },
        { "DoseMeasurement", "DOSE_MEASUREMENT" }
    };
    map_t::const_iterator it;
    std::stringstream ss;

    it = map.find(m_interpreted_type.c_str());
    if (it != map.end()) {
        return it->second;
    }
    ss << "Unrecognized ROI interpreted type string: " << m_interpreted_type;
    throw std::runtime_error(ss.str());
    return nullptr;
}
