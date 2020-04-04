#include "plugin.hpp"
#include "BidooComponents.hpp"
#include "dsp/digital.hpp"
#include <sstream>
#include <iomanip>

using namespace std;

struct HUITRE : Module {
	enum ParamIds {
		TRIG_PARAM,
		PATTERN_PARAM = TRIG_PARAM + 8,
		CV1_PARAM = PATTERN_PARAM + 8,
		CV2_PARAM = CV1_PARAM + 8,
		NUM_PARAMS = CV2_PARAM + 8
	};
	enum InputIds {
		MEASURE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PATTERN_OUTPUT,
		CV1_OUTPUT,
		CV2_OUTPUT,
		PATTERNTRIG_OUTPUT,
		NUM_OUTPUTS = PATTERNTRIG_OUTPUT + 8
	};
	enum LightIds {
		PATTERN_LIGHT,
		NUM_LIGHTS = PATTERN_LIGHT+16*3
	};

	dsp::SchmittTrigger patTriggers[8];
	dsp::SchmittTrigger syncTrigger;
	int currentPattern = 0;
	int nextPattern = 0;
	dsp::PulseGenerator gatePulse;
	bool pulse=false;

	HUITRE() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 8; i++) {
			configParam(TRIG_PARAM+i, 0.0f, 10.0f, 0.0f);
			configParam(PATTERN_PARAM+i, 0.0f, 10.0f, i*1.25f);
			configParam(CV1_PARAM+i, 0.0f, 10.0f, 0.0f);
			configParam(CV2_PARAM+i, 0.0f, 10.0f, 0.0f);
		}
  }

  void process(const ProcessArgs &args) override;


	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {

	}
};

void HUITRE::process(const ProcessArgs &args) {
	for (int i = 0; i < 8; i++) {
		if (patTriggers[i].process(params[TRIG_PARAM+i].getValue())) {
			nextPattern = i;
		}

		if (i==currentPattern) {
			lights[PATTERN_LIGHT+(i*3)].setBrightness(0.0f);
			lights[PATTERN_LIGHT+(i*3)+1].setBrightness(1.0f);
			lights[PATTERN_LIGHT+(i*3)+2].setBrightness(0.0f);
		}
		else if (i==nextPattern) {
			lights[PATTERN_LIGHT+(i*3)].setBrightness(1.0f);
			lights[PATTERN_LIGHT+(i*3)+1].setBrightness(0.0f);
			lights[PATTERN_LIGHT+(i*3)+2].setBrightness(0.0f);
		}
		else {
			lights[PATTERN_LIGHT+(i*3)].setBrightness(0.0f);
			lights[PATTERN_LIGHT+(i*3)+1].setBrightness(0.0f);
			lights[PATTERN_LIGHT+(i*3)+2].setBrightness(0.0f);
		}

		outputs[PATTERNTRIG_OUTPUT+i].setVoltage(0.0f);
	}

	if ((syncTrigger.process(inputs[MEASURE_INPUT].getVoltage())) && (currentPattern != nextPattern)) {
		currentPattern = nextPattern;
		gatePulse.trigger(1e-3f);
	}

	pulse = gatePulse.process(args.sampleTime);

	if (pulse) {
		outputs[PATTERNTRIG_OUTPUT+currentPattern].setVoltage(10.0f);
	}

	outputs[PATTERN_OUTPUT].setVoltage(1.25f*params[PATTERN_PARAM+currentPattern].getValue());
	outputs[CV1_OUTPUT].setVoltage(1.25f*params[CV1_PARAM+currentPattern].getValue());
	outputs[CV2_OUTPUT].setVoltage(1.25f*params[CV2_PARAM+currentPattern].getValue());
}

struct HUITREWidget : ModuleWidget {
  HUITREWidget(HUITRE *module);
};

template <typename BASE>
struct HUITRELight : BASE {
	HUITRELight() {
		this->box.size = mm2px(Vec(6.0f, 6.0f));
	}
};

HUITREWidget::HUITREWidget(HUITRE *module) {
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HUITRE.svg")));

	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(createInput<PJ301MPort>(Vec(7, 330), module, HUITRE::MEASURE_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(44.0f, 330), module, HUITRE::PATTERN_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(81.5f, 330), module, HUITRE::CV1_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(118.5f, 330), module, HUITRE::CV2_OUTPUT));

	for (int i = 0; i < 8; i++) {
		addParam(createParam<LEDBezel>(Vec(11.0f, 50 + i*33), module, HUITRE::TRIG_PARAM + i));
		addChild(createLight<HUITRELight<RedGreenBlueLight>>(Vec(13.0f, 52 + i*33), module, HUITRE::PATTERN_LIGHT + i*3));
		addParam(createParam<BidooBlueTrimpot>(Vec(45,52 + i*33), module, HUITRE::PATTERN_PARAM+i));
		addParam(createParam<BidooBlueTrimpot>(Vec(72,52 + i*33), module, HUITRE::CV1_PARAM+i));
		addParam(createParam<BidooBlueTrimpot>(Vec(99,52 + i*33), module, HUITRE::CV2_PARAM+i));
		addOutput(createOutput<TinyPJ301MPort>(Vec(130, 55 + i*33), module, HUITRE::PATTERNTRIG_OUTPUT+i));
	}
}

Model *modelHUITRE = createModel<HUITRE, HUITREWidget>("HUITre");
