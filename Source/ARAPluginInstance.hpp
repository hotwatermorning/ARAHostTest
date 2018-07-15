#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <memory>

// ARA API includes
#include <ARAInterface.h>

struct ARAPluginInstance
{
    //! コンストラクタ
    /*! ARAプラグインを実装しているプラグインを渡すと、
     *  そのプラグインからARAプラグイン用のファクトリなどを初期化し、ARAとして使用できるようにする。
     */
    ARAPluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance);
    ~ARAPluginInstance();
    
    //! ARAプラグインを実装しているベースのプラグイン（コンストラクタに渡したもの）をjuce::AudioPluginInstanceの形式で取得
    juce::AudioPluginInstance * GetBase();
    
public:
    struct Impl;
private:
    std::unique_ptr<Impl> pimpl_;
    
public:
    ARA::ARAFactory const * GetARAFactory();
    ARA::ARAPlugInExtensionInstance const * GetARAPlugInExtension(ARA::ARADocumentControllerRef controllerRef);
};
