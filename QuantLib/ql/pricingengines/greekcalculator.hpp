#ifndef QUANTLIB_GREEKCALCULATOR_H
#define QUANTLIB_GREEKCALCULATOR_H

#include <ql/pricingengines/additionalresultcalculators.hpp>
#include <ql/quote.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/instruments/swaption.hpp>
#include <ql/quotes/simplequote.hpp>

namespace QuantLib {

    struct ShiftedQuote{

        ShiftedQuote(double shift, const boost::shared_ptr<SimpleQuote>& quote): shift_(shift), quote_(quote) {}

        double shift_;
        boost::shared_ptr<SimpleQuote> quote_;
    };

    struct ShiftedModel{

        ShiftedModel(double shift, const boost::shared_ptr<HullWhite>& model): shift_(shift), model_(model) {}

        double shift_;
        boost::shared_ptr<HullWhite> model_;
    };

    typedef std::map<std::string, ShiftedQuote> LabelledQuotes;

    /**
     * This collects greek calculations.  It depends on SimpleQuote,
     * so is a basic implementation.
     * */
    struct GreekCalculator : virtual public AdditionalResultCalculator {
        GreekCalculator(const LabelledQuotes& underliers, const boost::shared_ptr<Instrument>& instrument);
        void calculateAdditionalResults() ;
    private:
        const LabelledQuotes underliers_;
        boost::shared_ptr<Instrument> instrument_;
    };

    void calculateDeltaAndGamma(
        std::map<std::string, boost::any>& additionalResults,
        const ShiftedQuote& shiftedQuote,
        const boost::shared_ptr<Instrument>& instrument
    );

    /**
     * This vol calculator is a bit of a hack.  This caters for instances where
     * we do not have a vol surface to calibrate to, and so
     * */
    struct HullWhiteVegaCalculator: virtual public AdditionalResultCalculator {
         HullWhiteVegaCalculator(const ShiftedModel& model, const boost::shared_ptr<Instrument>& instrument);

        void calculateAdditionalResults() ;
    private:
        ShiftedModel model_;
        boost::shared_ptr<Instrument> instrument_;
    };
    void calculateHullWhiteVega(
        std::map<std::string, boost::any>& additionalResults,
        const ShiftedModel& model,
        const boost::shared_ptr<Instrument>& instrument
    );
}


#endif //QUANTLIB_GREEKCALCULATOR_H
