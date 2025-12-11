#include "FeatureMode.hpp"

namespace secret_hitler {

static FeatureMode g_mode = FeatureMode::ManualOnly;

void setFeatureMode(FeatureMode mode) {
    g_mode = mode;
}

FeatureMode getFeatureMode() {
    return g_mode;
}

}
