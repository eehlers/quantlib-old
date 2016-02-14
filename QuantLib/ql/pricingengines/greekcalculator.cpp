//
// Created by Etuka Onono on 07/02/2016.
//

#include <ql/pricingengines/greekcalculator.hpp>
#include <ql/discretizedasset.hpp>
#include <iostream>


namespace QuantLib {

    GreekCalculator::GreekCalculator(
        const LabelledQuotes& underliers,
        const boost::shared_ptr<Instrument>& instrument
    ): underliers_(underliers), instrument_(instrument){
    }

    void GreekCalculator::calculateAdditionalResults() {

        calculating_ = true;
        double pv = instrument_->NPV();
        ShiftedQuote shiftedQuote = underliers_.at("price");
        //delta

        double rate = shiftedQuote.quote_->value();
        double rateUp = rate + shiftedQuote.shift_;

        shiftedQuote.quote_->setValue(rateUp);
        double pvUp = instrument_->NPV();

        double rateDown = rate - shiftedQuote.shift_;
        shiftedQuote.quote_->setValue(rateDown);
        double pvDown = instrument_->NPV();

        double delta = (pvUp - pvDown) / (2.*shiftedQuote.shift_);
        additionalResults_["delta"] = delta;

        //gamma
        rateUp = rateUp + shiftedQuote.shift_;
        shiftedQuote.quote_->setValue(rateUp);
        pvUp = instrument_->NPV();

        rateDown = rateDown - shiftedQuote.shift_;
        shiftedQuote.quote_->setValue(rateDown);
        pvDown = instrument_->NPV();

        double gamma = (pvUp + 2.* pv- pvDown) / (4. * shiftedQuote.shift_ * shiftedQuote.shift_);

        additionalResults_["gamma"] = gamma;
        //tidy up by setting things as they were.
        shiftedQuote.quote_->setValue(rate);
        calculating_ = false;
    }

    HullWhiteVegaCalculator::HullWhiteVegaCalculator(
        const ShiftedModel& model,
        const boost::shared_ptr<Instrument>& instrument
    ): model_(model), instrument_(instrument){
    }

    void HullWhiteVegaCalculator::calculateAdditionalResults() {
        calculating_ = true;

        double pv = instrument_->NPV();
        //vega
        double rate = model_.model_->params()[0];
        double vol = model_.model_->params()[1];
        double volUp = vol + model_.shift_;
        Array params(2);

        params[0] = rate; params[1] = volUp;
        model_.model_->setParams(params);
        instrument_->recalculate();
        double pvUp = instrument_->NPV();

        double volDown = vol - model_.shift_;
        params[1] = volDown;
        instrument_->recalculate();
        double pvDown = instrument_->NPV();

        double delta = (pvUp - pvDown) / (2.*model_.shift_);
        additionalResults_["vega"] = delta;
        //tidy up by setting things as they were.
        params[1] = vol;
        model_.model_->setParams(params);
        calculating_ = false;
    }

}