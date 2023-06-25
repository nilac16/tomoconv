#pragma once

#ifndef TOMO_DVH_H
#define TOMO_DVH_H

#include "image.h"
#include "structures.h"


namespace tomo {


class dvh {
public:
    /* Each pair is <dose, volume>; the whole vector represents a single ROI */
    using dvh_t = std::vector<std::pair<double, double>>;

private:
    /* The order matches the order of ROIs */
    std::vector<dvh_t> m_hists;

    std::vector<dvh_t> &histograms() noexcept { return m_hists; }

public:
    dvh();

    void compute(const tomo::image &dose, const std::vector<tomo::roi> &rois);


    const std::vector<dvh_t> &histograms() const noexcept { return m_hists; }
};


}


#endif /* TOMO_DVH_H */
