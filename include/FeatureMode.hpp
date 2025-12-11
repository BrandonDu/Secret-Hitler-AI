#pragma once

namespace secret_hitler {

enum class FeatureMode {
    ManualOnly,      // extractVotingFeatures / extractEnactFeatures
    TransformerOnly, // use only transformer input
    Both             // concatenate manual + transformer (optional)
};

void setFeatureMode(FeatureMode mode);
FeatureMode getFeatureMode();

}
