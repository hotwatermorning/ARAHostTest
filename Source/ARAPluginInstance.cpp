#include "../JuceLibraryCode/JuceHeader.h"
#include "ARAPluginInstance.hpp"

#include <base/source/fstring.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>

#include <ARAVST3.h>

DEF_CLASS_IID(ARA::IMainFactory)
DEF_CLASS_IID(ARA::IPlugInEntryPoint)

struct ARAPluginInstance::Impl
{
protected:
    Impl(std::unique_ptr<juce::AudioPluginInstance> instance)
    :   instance_(std::move(instance))
    {}
    
public:
    virtual
    ~Impl() {}

    virtual
    ARA::ARAFactory const * GetARAFactory() = 0;
    
    virtual
    ARA::ARAPlugInExtensionInstance const * GetARAPlugInExtension(ARA::ARADocumentControllerRef controllerRef) = 0;

    std::unique_ptr<juce::AudioPluginInstance> instance_;
};

struct Vst3PluginImpl : ARAPluginInstance::Impl
{
    Vst3PluginImpl(std::unique_ptr<juce::AudioPluginInstance> instance,
                   Steinberg::Vst::IComponent *component)
    :   Impl(std::move(instance))
    ,   component_(component)
    ,   factory_(nullptr)
    {
        ARA::IPlugInEntryPoint * entry = NULL;
        if ((component_->queryInterface(ARA::IPlugInEntryPoint::iid, (void**) &entry) == Steinberg::kResultTrue) && entry)
        {
            factory_ = entry->getFactory();
            entry->release();
        }
    }
    
    ARA::ARAFactory const * GetARAFactory() override
    {
        return factory_;
    }
    
    ARA::ARAPlugInExtensionInstance const * GetARAPlugInExtension(ARA::ARADocumentControllerRef controllerRef) override
    {
        const ARA::ARAPlugInExtensionInstance * result = NULL;
        
        ARA::IPlugInEntryPoint * entry = NULL;
        if ((component_->queryInterface(ARA::IPlugInEntryPoint::iid, (void**) &entry) == Steinberg::kResultTrue) && entry)
        {
            result = entry->bindToDocumentController(controllerRef);
            entry->release();
        }
        
        return result;
    }


    Steinberg::Vst::IComponent *component_;
    ARA::ARAFactory const * factory_;
};

ARAPluginInstance::ARAPluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance)
:   pimpl_()
{
    assert(instance);

    auto const format_name = instance->getPluginDescription().pluginFormatName;
    auto data = instance->getPlatformSpecificData();
    
    if(format_name == "VST3") {
        pimpl_ = std::make_unique<Vst3PluginImpl>(std::move(instance),
                                                  reinterpret_cast<Steinberg::Vst::IComponent *>(data)
                                                  );
    } else {
        throw std::runtime_error("Unsupported plugin type.");
    }
}

ARAPluginInstance::~ARAPluginInstance()
{}

juce::AudioPluginInstance * ARAPluginInstance::GetBase()
{
    return pimpl_->instance_.get();
    
}

ARA::ARAFactory const * ARAPluginInstance::GetARAFactory()
{
    return pimpl_->GetARAFactory();
}

ARA::ARAPlugInExtensionInstance const * ARAPluginInstance::GetARAPlugInExtension(ARA::ARADocumentControllerRef controllerRef)
{
    return pimpl_->GetARAPlugInExtension(controllerRef);
}
