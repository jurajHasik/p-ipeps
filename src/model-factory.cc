#include "pi-peps/config.h"
#include "pi-peps/model-factory.h"
#include "pi-peps/models/aklt-2x2-ABCD.h"
#include "pi-peps/models/hb-2x2-ABCD.h"
#include "pi-peps/models/ising-2x2-ABCD.h"
#include "pi-peps/models/j1j2-2x2-ABCD.h"
#include "pi-peps/models/ladders-2x2-ABCD.h"

ModelFactory::ModelFactory() {
  registerModel("ISING_2X2_ABCD", &itensor::IsingModel_2x2_ABCD::create);
  registerModel("ISING_2X2_AB", &itensor::IsingModel_2x2_AB::create);
  registerModel("HB_2X2_ABCD", &itensor::HeisenbergModel_2x2_ABCD::create);
  registerModel("HB_2X2_AB", &itensor::HeisenbergModel_2x2_AB::create);
  registerModel("HB_1X1_A", &itensor::HeisenbergModel_1x1_A::create);
  registerModel("AKLT_2X2_ABCD", &itensor::AKLTModel_2x2_ABCD::create);
  registerModel("AKLT_2X2_AB", &itensor::AKLTModel_2x2_AB::create);
  registerModel("J1J2_2X2_ABCD", &itensor::J1J2Model_2x2_ABCD::create);
  registerModel("J1J2_1X1_A", &itensor::J1J2Model_1x1_A::create);
  registerModel("LADDERS_2X2_ABCD", &itensor::LaddersModel_2x2_ABCD::create);
  registerModel("LADDERS_4X2_ABCD", &itensor::LaddersModel_2x2_ABCD::create);
}

bool ModelFactory::registerModel(std::string const& name,
                                 TCreateMethod funcCreate) {
  auto it = s_methods.find(name);
  if (it == s_methods.end()) {
    s_methods[name] = funcCreate;
    return true;
  }
  return false;
}

std::unique_ptr<Model> ModelFactory::create(nlohmann::json& json_model) {
  std::string model_type = json_model.value("type", "NOT_FOUND");

  return create(model_type, json_model);

  return nullptr;
}

std::unique_ptr<Model> ModelFactory::create(std::string const& name,
                                            nlohmann::json& json_model) {
  auto it = s_methods.find(name);
  if (it != s_methods.end())
    return it->second(json_model);  // call the "create" function

  std::string message = "[ModelFactory] Invalid model: " + name;
  throw std::runtime_error(message);

  return nullptr;
}
