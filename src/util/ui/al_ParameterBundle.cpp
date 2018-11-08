
#include <cstring>


#include "al/util/ui/al_ParameterBundle.hpp"
#include "al/util/ui/al_ParameterServer.hpp"

using namespace al;

int ParameterBundle::mBundleCounter = 0;

ParameterBundle::ParameterBundle(std::string name) {
    if (name.size() == 0) {
        mBundleName = "bundle";
    } else {
        mBundleName = name;
    }
    mBundleIndex = mBundleCounter;
    ParameterBundle::mBundleCounter++;
}

std::string ParameterBundle::name() const
{
    return mBundleName;
}

std::string ParameterBundle::bundlePrefix(bool appendCounter) const
{
    std::string prefix = "/" + mBundleName;
    if (appendCounter) {
        prefix += "/" + std::to_string(mBundleIndex);
    }
    return prefix;
}

int ParameterBundle::bundleIndex() const
{
    return mBundleIndex;
}

void ParameterBundle::addParameter(ParameterMeta *parameter) {
    mParameters.push_back(parameter);
    if (strcmp(typeid(*parameter).name(), typeid(ParameterBool).name() ) == 0) { // ParameterBool
        ParameterBool *p = dynamic_cast<ParameterBool *>(parameter);
        p->registerChangeCallback([this, p](float value){
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(Parameter).name()) == 0) {// Parameter
        std::cout << "Register parameter " << parameter->getName() << std::endl;
        Parameter *p = dynamic_cast<Parameter *>(parameter);
        p->registerChangeCallback([this, p](float value){
//            std::cout << "Changed  " << p->getName() << std::endl;
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(ParameterPose).name()) == 0) {// ParameterPose
        ParameterPose *p = dynamic_cast<ParameterPose *>(parameter);
        p->registerChangeCallback([this, p](al::Pose value){
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(ParameterMenu).name()) == 0) {// ParameterMenu
        ParameterMenu *p = dynamic_cast<ParameterMenu *>(parameter);
        p->registerChangeCallback([this, p](int value){
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(ParameterChoice).name()) == 0) {// ParameterChoice
        ParameterChoice *p = dynamic_cast<ParameterChoice *>(parameter);
        p->registerChangeCallback([this, p](uint16_t value){
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(ParameterVec3).name()) == 0) {// ParameterVec3
        ParameterVec3 *p = dynamic_cast<ParameterVec3 *>(parameter);

        p->registerChangeCallback([this, p](al::Vec3f value){
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(ParameterVec4).name()) == 0) {// ParameterVec4
        ParameterVec4 *p = dynamic_cast<ParameterVec4 *>(parameter);
        p->registerChangeCallback([this, p](al::Vec4f value){
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), value);
            }
        });
    } else if (strcmp(typeid(*parameter).name(), typeid(ParameterColor).name()) == 0) {// ParameterColor
        ParameterColor *p = dynamic_cast<ParameterColor *>(parameter);

        p->registerChangeCallback([this, p](Color value){
            Vec4f valueVec(value.r, value.g, value.b, value.a);
            for (OSCNotifier *n: mNotifiers) {
                n->notifyListeners(bundlePrefix() + p->getFullAddress(), valueVec);
            }
        });
    } else {
        // TODO this check should be performed on registration
        std::cout << "Unsupported Parameter type for bundle OSC dsitribution" << std::endl;
    }
}

ParameterBundle &ParameterBundle::operator <<(ParameterMeta *parameter) {
    addParameter(parameter);
    return *this;
}

ParameterBundle &ParameterBundle::operator <<(ParameterMeta &parameter)
{
    addParameter(&parameter);
    return *this;
}
