#include "dvh.h"


tomo::dvh::dvh()
{

}


void tomo::dvh::compute(const tomo::image            &dose,
                        const std::vector<tomo::roi> &rois)
{
    m_hists.resize(rois.size());
}
