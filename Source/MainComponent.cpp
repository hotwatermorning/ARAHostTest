#include "../JuceLibraryCode/JuceHeader.h"

#include <mutex>
#include <iomanip>
#include <sstream>
#include <memory>

#include <ARAInterface.h>
#include <TestCommon/AraDebug.h>

#include "ARAPluginInstance.hpp"

using namespace ARA;

// defines enable/disable various test code
#define TEST_CONTENTREAD 1
#define TEST_TIMESTRETCH 1
#define TEST_UNARCHIVING 1
#define TEST_RENDERING 1
#define TEST_MODIFICATIONCLONING 1


// we are not using actual objects in this test implementation, so here's a few constants that are used where actual host code would use object pointers or array indices
int kTestAudioSourceSampleRate = 44100;	/* Hertz */
int kTestAudioSourceDuration = 5;			/* seconds */
#define kHostAudioSourceHostRef ((ARAAudioSourceHostRef) 1)
#define kHostAudioModificationHostRef ((ARAAudioModificationHostRef) 2)
#define kHostMusicalContextHostRef ((ARAMusicalContextHostRef) 3)
#define kAudioAccessControllerHostRef ((ARAContentAccessControllerHostRef) 10)
#define kContentAccessControllerHostRef ((ARAContentAccessControllerHostRef) 11)
#define kModelUpdateControllerHostRef ((ARAModelUpdateControllerHostRef) 12)
#define kPlaybackControllerHostRef ((ARAPlaybackControllerHostRef) 13)
#define kArchivingControllerHostRef ((ARAArchivingControllerHostRef) 14)
#define kAudioAccessor32BitHostRef ((ARAAudioReaderHostRef) 20)
#define kAudioAccessor64BitHostRef ((ARAAudioReaderHostRef) 21)
#define kHostTempoContentReaderHostRef ((ARAContentReaderHostRef) 30)
#define kHostSignaturesContentReaderHostRef ((ARAContentReaderHostRef) 31)
#define kARAInvalidHostRef ((ARAHostRef) 0)


// ARAAudioAccessControllerInterface (required)
ARAAudioReaderHostRef ARA_CALL ARACreateAudioReaderForSource(ARAAudioAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARABool use64BitSamples)
{
    ARAAudioReaderHostRef accessorRef = (use64BitSamples) ? kAudioAccessor64BitHostRef : kAudioAccessor32BitHostRef;
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kAudioAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef);
    ARA_LOG("TestHost: createAudioReaderForSource() returns fake ref %p", accessorRef);
    return accessorRef;
}

juce::AudioFormatReader *g_reader;
ARABool ARA_CALL ARAReadAudioSamples(ARAAudioAccessControllerHostRef controllerHostRef, ARAAudioReaderHostRef readerHostRef,
                                     ARASamplePosition samplePosition, ARASampleCount samplesPerChannel, void * const * buffers)
{
    void * bufferPtr;
    
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kAudioAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(readerHostRef, (readerHostRef == kAudioAccessor32BitHostRef) || (readerHostRef == kAudioAccessor64BitHostRef));
    ARA_VALIDATE_ARGUMENT(NULL, 0 <= samplePosition);
    //ARA_VALIDATE_ARGUMENT(NULL, samplePosition + samplesPerChannel <= kTestAudioSourceSampleRate * kTestAudioSourceDuration);
    ARA_VALIDATE_ARGUMENT(buffers, buffers != NULL);
    ARA_VALIDATE_ARGUMENT(buffers, buffers[0] != NULL);

    assert(g_reader);
    juce::AudioSampleBuffer buffer((float * const *)buffers, (int)g_reader->numChannels, (int)samplesPerChannel);;
    g_reader->read(&buffer, 0, (int)samplesPerChannel, samplePosition, true, true);
    
    return kARATrue;
}
void ARA_CALL ARADestroyAudioReader(ARAAudioAccessControllerHostRef controllerHostRef, ARAAudioReaderHostRef readerHostRef)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kAudioAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(readerHostRef, (readerHostRef == kAudioAccessor32BitHostRef) || (readerHostRef == kAudioAccessor64BitHostRef));
    ARA_LOG("TestHost: destroyAudioReader() called for fake ref %p", readerHostRef);
}
static const ARAAudioAccessControllerInterface hostAudioAccessControllerInterface = { sizeof(hostAudioAccessControllerInterface),
    &ARACreateAudioReaderForSource, &ARAReadAudioSamples, &ARADestroyAudioReader };


// ARAContentAccessControllerInterface
ARABool ARA_CALL ARAIsMusicalContextContentAvailable(ARAContentAccessControllerHostRef controllerHostRef, ARAMusicalContextHostRef musicalContextHostRef, ARAContentType type)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(musicalContextHostRef, musicalContextHostRef == kHostMusicalContextHostRef);
    
    if (type == kARAContentTypeTempoEntries)
        return kARATrue;
    
    if (type == kARAContentTypeSignatures)
        return kARATrue;
    
    return kARAFalse;
}
ARAContentGrade ARA_CALL ARAGetMusicalContextContentGrade(ARAContentAccessControllerHostRef controllerHostRef, ARAMusicalContextHostRef musicalContextHostRef, ARAContentType type)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(musicalContextHostRef, musicalContextHostRef == kHostMusicalContextHostRef);
    
    if (type == kARAContentTypeTempoEntries)
        return kARAContentGradeAdjusted;
    
    if (type == kARAContentTypeSignatures)
        return kARAContentGradeAdjusted;
    
    return kARAContentGradeInitial;
}
ARAContentReaderHostRef ARA_CALL ARACreateMusicalContextContentReader(ARAContentAccessControllerHostRef controllerHostRef, ARAMusicalContextHostRef musicalContextHostRef, ARAContentType type, const ARAContentTimeRange * range)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(musicalContextHostRef, musicalContextHostRef == kHostMusicalContextHostRef);
    ARA_VALIDATE_ARGUMENT(NULL, (type == kARAContentTypeTempoEntries) || (type == kARAContentTypeSignatures));	// only supported types may be requested
    
    if (type == kARAContentTypeTempoEntries)
    {
        ARA_LOG("TestHost: createMusicalContextContentReader() called for fake tempo reader ref %p", kHostTempoContentReaderHostRef);
        return kHostTempoContentReaderHostRef;
    }
    
    if (type == kARAContentTypeSignatures)
    {
        ARA_LOG("TestHost: createMusicalContextContentReader() called for fake signatures reader ref %p", kHostSignaturesContentReaderHostRef);
        return kHostSignaturesContentReaderHostRef;
    }
    
    return kARAInvalidHostRef;
}
ARABool ARA_CALL ARAIsAudioSourceContentAvailable(ARAContentAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAContentType type)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef);
    
    return kARAFalse;
}
ARAContentGrade ARA_CALL ARAGetAudioSourceContentGrade(ARAContentAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAContentType type)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef);
    
    return kARAContentGradeInitial;
}
ARAContentReaderHostRef ARA_CALL ARACreateAudioSourceContentReader(ARAContentAccessControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAContentType type, const ARAContentTimeRange * range)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef);
    
    ARA_VALIDATE_CONDITION(0 && "should never be called since we do not provide content information at audio source level");
    
    return kARAInvalidHostRef;
}


static const ARAContentSignature signatureDefinition = { 4, 4, 0.0 };
static const ARAContentTempoEntry tempoSyncPoints[2] = { { 0.0, 0.0 }, { 60 / 127.0, 1.0 } };
// these are some more valid timelines you can use for testing your implementation:
// static const ARAContentTempoEntry tempoSyncPoints[2] = { { -0.5, -1.0 }, { 0.0, 0.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[3] = { { -1.0, -2.0 }, { -0.5, -1.0 }, { 0.0, 0.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[3] = { { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[3] = { { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.0, 2.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[4] = { { -1.0, -2.0 }, { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[4] = { { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.0, 2.0 } };
// static const ARAContentTempoEntry tempoSyncPoints[5] = { { -1.0, -2.0 }, { -0.5, -1.0 }, { 0.0, 0.0 }, { 0.5, 1.0 }, { 1.0, 2.0 } };

ARAInt32 ARA_CALL ARAGetContentReaderEventCount(ARAContentAccessControllerHostRef controllerHostRef, ARAContentReaderHostRef readerHostRef)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(readerHostRef, (readerHostRef == kHostTempoContentReaderHostRef) || (readerHostRef == kHostSignaturesContentReaderHostRef));
    
    if (readerHostRef == kHostTempoContentReaderHostRef)
        return sizeof(tempoSyncPoints) / sizeof(tempoSyncPoints[0]);
    
    if (readerHostRef == kHostSignaturesContentReaderHostRef)
        return 1;
    
    return 0;
}
const void * ARA_CALL ARAGetContentReaderDataForEvent(ARAContentAccessControllerHostRef controllerHostRef, ARAContentReaderHostRef readerHostRef, ARAInt32 eventIndex)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(readerHostRef, (readerHostRef == kHostTempoContentReaderHostRef) || (readerHostRef == kHostSignaturesContentReaderHostRef));
    ARA_VALIDATE_ARGUMENT(NULL, 0 <= eventIndex);
    ARA_VALIDATE_ARGUMENT(NULL, eventIndex < ARAGetContentReaderEventCount(controllerHostRef, readerHostRef));
    
    if (readerHostRef == kHostTempoContentReaderHostRef)
    {
        return &tempoSyncPoints[eventIndex];
    }
    
    if (readerHostRef == kHostSignaturesContentReaderHostRef)
    {
        return &signatureDefinition;
    }
    
    return NULL;
}
void ARA_CALL ARADestroyContentReader(ARAContentAccessControllerHostRef controllerHostRef, ARAContentReaderHostRef readerHostRef)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kContentAccessControllerHostRef);
    ARA_VALIDATE_ARGUMENT(readerHostRef, (readerHostRef == kHostTempoContentReaderHostRef) || (readerHostRef == kHostSignaturesContentReaderHostRef));
    ARA_LOG("TestHost: plug-in destroyed content reader ref %p", readerHostRef);
}
static const ARAContentAccessControllerInterface hostContentAccessControllerInterface = { sizeof(hostContentAccessControllerInterface),
    &ARAIsMusicalContextContentAvailable, &ARAGetMusicalContextContentGrade, &ARACreateMusicalContextContentReader,
    &ARAIsAudioSourceContentAvailable, &ARAGetAudioSourceContentGrade, &ARACreateAudioSourceContentReader,
    &ARAGetContentReaderEventCount, &ARAGetContentReaderDataForEvent, &ARADestroyContentReader };


// ARAModelUpdateControllerInterface
#define kInvalidProgressValue -1.0f
static float lastProgressValue = kInvalidProgressValue;
void ARA_CALL ARANotifyAudioSourceAnalysisProgress(ARAModelUpdateControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, ARAAnalysisProgressState state, float value)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kModelUpdateControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef);
    ARA_VALIDATE_ARGUMENT(NULL, 0.0f <= value);
    ARA_VALIDATE_ARGUMENT(NULL, value <= 1.0f);
    ARA_VALIDATE_ARGUMENT(NULL, lastProgressValue <= value);
    
    switch (state)
    {
        case kARAAnalysisProgressStarted:
        {
            ARA_VALIDATE_STATE(lastProgressValue == kInvalidProgressValue);
            ARA_LOG("TestHost: audio source analysis started with progress %.f%%.", 100.0f * value);
            lastProgressValue = value;
            break;
        }
        case kARAAnalysisProgressUpdated:
        {
            ARA_VALIDATE_STATE(lastProgressValue != kInvalidProgressValue);
            ARA_LOG("TestHost: audio source analysis progresses %.f%%.", 100.0f * value);
            lastProgressValue = value;
            break;
        }
        case kARAAnalysisProgressCompleted:
        {
            ARA_VALIDATE_STATE(lastProgressValue != kInvalidProgressValue);
            ARA_LOG("TestHost: audio source analysis finished with progress %.f%%.", 100.0f * value);
            lastProgressValue = kInvalidProgressValue;
            break;
        }
        default:
        {
            ARA_VALIDATE_ARGUMENT(NULL, 0 && "invalid progress state");
        }
    }
}
void ARA_CALL ARANotifyAudioSourceContentChanged(ARAModelUpdateControllerHostRef controllerHostRef, ARAAudioSourceHostRef audioSourceHostRef, const ARAContentTimeRange * range, ARAContentUpdateFlags contentFlags)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kModelUpdateControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioSourceHostRef, audioSourceHostRef == kHostAudioSourceHostRef);
    if (range)
    {
        ARA_VALIDATE_ARGUMENT(range, 0.0 <= range->start);
        ARA_VALIDATE_ARGUMENT(range, 0.0 <= range->duration);
        ARA_VALIDATE_ARGUMENT(range, range->start + range->duration < kTestAudioSourceDuration);
    }
    ARA_LOG("TestHost: audio source content was updated in range %.3f-%.3f, flags %X", (range) ? range->start : 0.0, (range) ? range->start + range->duration : kTestAudioSourceDuration, contentFlags);
}
void ARA_CALL ARANotifyAudioModificationContentChanged(ARAModelUpdateControllerHostRef controllerHostRef, ARAAudioModificationHostRef audioModificationHostRef, const ARAContentTimeRange * range, ARAContentUpdateFlags contentFlags)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kModelUpdateControllerHostRef);
    ARA_VALIDATE_ARGUMENT(audioModificationHostRef, audioModificationHostRef == kHostAudioModificationHostRef);
    if (range)
        ARA_VALIDATE_ARGUMENT(range, 0.0 <= range->duration);
    
    ARA_LOG("TestHost: audio modification content was updated in range %.3f-%.3f, flags %X", (range) ? range->start : 0.0, (range) ? range->start + range->duration : kTestAudioSourceDuration, contentFlags);
}
static const ARAModelUpdateControllerInterface hostModelUpdateControllerInterface = { sizeof(hostModelUpdateControllerInterface),
    &ARANotifyAudioSourceAnalysisProgress, &ARANotifyAudioSourceContentChanged, &ARANotifyAudioModificationContentChanged };

class ARATransporter
{
protected:
    ARATransporter() {}
    
public:
    virtual ~ARATransporter() {}

    //! time in seconds
    using Time = double;
    
    virtual void RequestStartPlayback() = 0;
    virtual void RequestStopPlayback() = 0;
    virtual void RequestSetPlaybackPosition(Time pos) = 0;
    virtual void RequestSetCycleRange(Time begin, Time duration) = 0;
    virtual void RequestEnableCycle(bool enabled) = 0;
};
ARATransporter *g_transporter;

// ARAPlaybackControllerInterface
void ARA_CALL ARARequestStartPlayback(ARAPlaybackControllerHostRef controllerHostRef)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kPlaybackControllerHostRef);
    g_transporter->RequestStartPlayback();
}
void ARA_CALL ARARequestStopPlayback(ARAPlaybackControllerHostRef controllerHostRef)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kPlaybackControllerHostRef);
    g_transporter->RequestStopPlayback();
}
void ARA_CALL ARARequestSetPlaybackPosition(ARAPlaybackControllerHostRef controllerHostRef, ARATimePosition timePosition)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kPlaybackControllerHostRef);
    g_transporter->RequestSetPlaybackPosition(timePosition);
}
void ARA_CALL ARARequestSetCycleRange(ARAPlaybackControllerHostRef controllerHostRef, ARATimePosition startTime, ARATimeDuration duration)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kPlaybackControllerHostRef);
    g_transporter->RequestSetCycleRange(startTime, duration);
}
void ARA_CALL ARARequestEnableCycle(ARAPlaybackControllerHostRef controllerHostRef, ARABool enable)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kPlaybackControllerHostRef);
    g_transporter->RequestEnableCycle(enable);
}
static const ARAPlaybackControllerInterface hostPlaybackControllerInterface = { kARAPlaybackControllerInterfaceMinSize /* we're not supporting everything here: sizeof(hostPlaybackControllerInterface) */,
    &ARARequestStartPlayback, &ARARequestStopPlayback, &ARARequestSetPlaybackPosition, &ARARequestSetCycleRange, &ARARequestEnableCycle };


// ARAArchivingControllerInterface
typedef struct TestArchive
{
    char * path;
    FILE * file;
} TestArchive;

ARASize ARA_CALL ARAGetArchiveSize(ARAArchivingControllerHostRef controllerHostRef, ARAArchiveReaderHostRef archiveReaderHostRef)
{
    TestArchive * archive = (TestArchive *)archiveReaderHostRef;
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kArchivingControllerHostRef);
    ARA_VALIDATE_ARGUMENT(archiveReaderHostRef, archive != NULL);
    
    fseek(archive->file, 0L, SEEK_END);
    return (ARASize)ftell(archive->file);
}
ARABool ARA_CALL ARAReadBytesFromArchive(ARAArchivingControllerHostRef controllerHostRef, ARAArchiveReaderHostRef archiveReaderHostRef,
                                         ARASize position, ARASize length, void * const buffer)
{
    size_t value;
    TestArchive * archive = (TestArchive *)archiveReaderHostRef;
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kArchivingControllerHostRef);
    ARA_VALIDATE_ARGUMENT(archiveReaderHostRef, archive != NULL);
    ARA_VALIDATE_ARGUMENT(NULL, 0 < length);
    ARA_VALIDATE_ARGUMENT(NULL, position + length <= ARAGetArchiveSize(controllerHostRef, archiveReaderHostRef));
    
    value = fseek(archive->file, (long)position, SEEK_SET);
    ARA_INTERNAL_ASSERT(value == position);
    
    value = fread(buffer, 1, length, archive->file);
    ARA_INTERNAL_ASSERT(value == length);
    
    return kARATrue;
}
ARABool ARA_CALL ARAWriteBytesToArchive(ARAArchivingControllerHostRef controllerHostRef, ARAArchiveWriterHostRef archiveWriterHostRef,
                                        ARASize position, ARASize length, const void * const buffer)
{
    size_t value;
    TestArchive * archive = (TestArchive *)archiveWriterHostRef;
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kArchivingControllerHostRef);
    ARA_VALIDATE_ARGUMENT(archiveWriterHostRef, archive != NULL);
    ARA_VALIDATE_ARGUMENT(NULL, 0 < length);
    
    value = fseek(archive->file, (long)position, SEEK_SET);
    ARA_INTERNAL_ASSERT(value == position);
    
    value = fwrite(buffer, 1, length, archive->file);
    ARA_INTERNAL_ASSERT(value == length);
    
    return kARATrue;
}
void ARA_CALL ARANotifyDocumentArchivingProgress(ARAArchivingControllerHostRef controllerHostRef, float value)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kArchivingControllerHostRef);
    ARA_LOG("TestHost: document archiving progresses %.f%%.", 100.0f * value);
}
void ARA_CALL ARANotifyDocumentUnarchivingProgress(ARAArchivingControllerHostRef controllerHostRef, float value)
{
    ARA_VALIDATE_ARGUMENT(controllerHostRef, controllerHostRef == kArchivingControllerHostRef);
    ARA_LOG("TestHost: document unarchiving progresses %.f%%.", 100.0f * value);
}
static const ARAArchivingControllerInterface hostArchivingInterface = { kARAArchivingControllerInterfaceMinSize,
    &ARAGetArchiveSize, &ARAReadBytesFromArchive, &ARAWriteBytesToArchive,
    &ARANotifyDocumentArchivingProgress, &ARANotifyDocumentUnarchivingProgress };


// helper
#if TEST_CONTENTREAD
void testAudioSourceContentReaders(const ARADocumentControllerInterface * documentControllerInterface, ARADocumentControllerRef documentControllerRef, ARAAudioSourceRef audioSourceRef)
{
    if (documentControllerInterface->isAudioSourceContentAvailable(documentControllerRef, audioSourceRef, kARAContentTypeTempoEntries))
    {
        ARAContentReaderRef contentReaderRef = documentControllerInterface->createAudioSourceContentReader(documentControllerRef, audioSourceRef, kARAContentTypeTempoEntries, NULL);
        ARAInt32 eventCount = documentControllerInterface->getContentReaderEventCount(documentControllerRef, contentReaderRef);
        ARAInt32 i;
        ARA_VALIDATE_CONDITION(eventCount > 0);
        ARA_LOG("TestHost: %i tempo entries available for audio source %p", eventCount, audioSourceRef);
        for (i = 0; i < eventCount; ++i)
        {
            const ARAContentTempoEntry * tempoData =
            (const ARAContentTempoEntry *)documentControllerInterface->getContentReaderDataForEvent(documentControllerRef, contentReaderRef, i);
            ARA_VALIDATE_CONDITION(tempoData != NULL);
            ARA_LOG("TestHost: tempo event %i is (sec = %.3f, quarters = %.3f)", i, tempoData->timePosition, tempoData->quarterPosition);
        }
        documentControllerInterface->destroyContentReader(documentControllerRef, contentReaderRef);
    }
    else
    {
        ARA_LOG("TestHost: no tempo entries available for audio source %p", audioSourceRef);
    }
    
    if (documentControllerInterface->isAudioSourceContentAvailable(documentControllerRef, audioSourceRef, kARAContentTypeSignatures))
    {
        ARAContentReaderRef contentReaderRef = documentControllerInterface->createAudioSourceContentReader(documentControllerRef, audioSourceRef, kARAContentTypeSignatures, NULL);
        ARAInt32 eventCount = documentControllerInterface->getContentReaderEventCount(documentControllerRef, contentReaderRef);
        ARAInt32 i;
        ARA_VALIDATE_CONDITION(eventCount > 0);
        ARA_LOG("TestHost: %i signatures available for audio source %p", eventCount, audioSourceRef);
        for (i = 0; i < eventCount; ++i)
        {
            const ARAContentSignature * signatureData =
            (const ARAContentSignature *)documentControllerInterface->getContentReaderDataForEvent(documentControllerRef, contentReaderRef, i);
            ARA_VALIDATE_CONDITION(signatureData != NULL);
            ARA_LOG("TestHost: signature event %i is (%i/%i, quarters = %.3f)", i, signatureData->numerator, signatureData->denominator, signatureData->position);
        }
        documentControllerInterface->destroyContentReader(documentControllerRef, contentReaderRef);
    }
    else
    {
        ARA_LOG("TestHost: no signatures available for audio source %p", audioSourceRef);
    }
    
    if (documentControllerInterface->isAudioSourceContentAvailable(documentControllerRef, audioSourceRef, kARAContentTypeNotes))
    {
        ARAContentReaderRef contentReaderRef = documentControllerInterface->createAudioSourceContentReader(documentControllerRef, audioSourceRef, kARAContentTypeNotes, NULL);
        ARAInt32 eventCount = documentControllerInterface->getContentReaderEventCount(documentControllerRef, contentReaderRef);
        ARAInt32 i;
        ARA_VALIDATE_CONDITION(eventCount > 0);
        ARA_LOG("TestHost: %i notes available for audio source %p", eventCount, audioSourceRef);
        for (i = 0; i < eventCount; ++i)
        {
            const ARAContentNote * noteData = (const ARAContentNote *)documentControllerInterface->getContentReaderDataForEvent(documentControllerRef, contentReaderRef, i);
            ARA_VALIDATE_CONDITION(noteData != NULL);
            ARA_LOG("TestHost: note event %i is (%.3f @ %4.f Hz (%i), %.3f-%.3f-%.3f-%.3f)", i,
                    noteData->volume, noteData->frequency, noteData->pitchNumber,
                    noteData->startPosition, noteData->startPosition + noteData->attackDuration,
                    noteData->startPosition + noteData->noteDuration, noteData->startPosition + noteData->signalDuration);
        }
        documentControllerInterface->destroyContentReader(documentControllerRef, contentReaderRef);
    }
    else
    {
        ARA_LOG("TestHost: no notes available for audio source %p", audioSourceRef);
    }
}

void testPlaybackRegionNoteContentReader(const ARADocumentControllerInterface * documentControllerInterface, ARADocumentControllerRef documentControllerRef, ARAPlaybackRegionRef playbackRegionRef)
{
    if (documentControllerInterface->isPlaybackRegionContentAvailable(documentControllerRef, playbackRegionRef, kARAContentTypeNotes) != kARAFalse)
    {
        ARAContentReaderRef contentReaderRef = documentControllerInterface->createPlaybackRegionContentReader(documentControllerRef, playbackRegionRef, kARAContentTypeNotes, NULL);
        ARAInt32 eventCount = documentControllerInterface->getContentReaderEventCount(documentControllerRef, contentReaderRef);
        ARAInt32 i;
        ARA_VALIDATE_CONDITION(eventCount > 0);
        ARA_LOG("TestHost: %i notes available for playback region %p", eventCount, playbackRegionRef);
        for (i = 0; i < eventCount; ++i)
        {
            const ARAContentNote * noteData = (const ARAContentNote *)documentControllerInterface->getContentReaderDataForEvent(documentControllerRef, contentReaderRef, i);
            ARA_VALIDATE_CONDITION(noteData != NULL);
            ARA_LOG("TestHost: note event %i is (%.3f @ %4.f Hz (%i), %.3f-%.3f-%.3f-%.3f)", i,
                    noteData->volume, noteData->frequency, noteData->pitchNumber,
                    noteData->startPosition, noteData->startPosition + noteData->attackDuration,
                    noteData->startPosition + noteData->noteDuration, noteData->startPosition + noteData->signalDuration);
        }
        documentControllerInterface->destroyContentReader(documentControllerRef, contentReaderRef);
    }
    else
    {
        ARA_LOG("TestHost: no notes available for playback region %p", playbackRegionRef);
    }
}
#endif	// TEST_CONTENTREAD

#if TEST_UNARCHIVING
void setupTestArchive(TestArchive * archive)
{
#if defined(_MSC_VER)
    char prefix[] = "AraTestHost\0";
    char path[MAX_PATH];
    char result[MAX_PATH];
    char * tmpPath;
    size_t length;
    UINT success;
    
    if (GetTempPathA(MAX_PATH, path))
        tmpPath = (char *)&path;
    else if (GetEnvironmentVariableA("TEMP", path, MAX_PATH) > 0)
        tmpPath = (char *)&path;
    else if (GetEnvironmentVariableA("TMP", path, MAX_PATH) > 0)
        tmpPath = (char *)&path;
    else
        tmpPath = ".";
    
    success = GetTempFileNameA(tmpPath, prefix, 0, result);
    ARA_INTERNAL_ASSERT(success);
    length = strlen(result);
    archive->path = malloc(length + 1);
    ARA_INTERNAL_ASSERT(archive->path != NULL);
    strncpy(archive->path, result, length);
    archive->path[length] = '\0';
#else
    char secureTmp[] = "/tmp/AraTestHostArchiveXXXXXXXXXXXX\0";
    int fd = mkstemp(secureTmp);
    ARA_INTERNAL_ASSERT(fd);
    close(fd);
    size_t length = strlen(secureTmp);
    archive->path = (char *)malloc(length + 1);
    strncpy(archive->path, secureTmp, length);
    archive->path[length] = '\0';
#endif
    
    archive->file = fopen(archive->path, "w+");
    ARA_INTERNAL_ASSERT(archive->file != NULL);
}

void teardownTestArchive(TestArchive * archive)
{
#if defined(_MSC_VER)
    BOOL success;
#endif
    int result = fclose(archive->file);
    ARA_INTERNAL_ASSERT(result == 0);
    archive->file = NULL;
    
#if defined(_MSC_VER)
    success = DeleteFileA(archive->path);
    ARA_INTERNAL_ASSERT(success);
#else
    result = remove(archive->path);
    ARA_INTERNAL_ASSERT(result == 0);
#endif
    free(archive->path);
    archive->path = NULL;
}
#endif	// TEST_UNARCHIVING

// asserts
ARAAssertFunction assertFunction = &AraInterfaceAssert;
ARAAssertFunction * assertFunctionReference = &assertFunction;

//============================================================

using TransportInfo = juce::AudioPlayHead::CurrentPositionInfo;

class ITransportInfoProvider
{
protected:
    ITransportInfoProvider() {}

public:
    virtual
    ~ITransportInfoProvider() {}

    //! この関数は、非リアルタイムスレッドから呼び出し可能
    virtual
    TransportInfo GetTransportInfo() const = 0;
    
    virtual
    int PPQToSample(double ppq_pos) const = 0;
    
    virtual
    double SampleToPPQ(int sample_pos) const = 0;
    
    virtual
    int SecondToSample(double sec) const = 0;
    
    virtual
    double SampleToSecond(int sample) const = 0;
};

class IndicatorComponent
:   public Component
,   Timer
{
public:
    IndicatorComponent(ITransportInfoProvider const *prov)
    :   prov_(prov)
    {
        assert(prov);
        startTimer(16);
        
        font_ = Font(Font::getDefaultMonospacedFontName(),
                     12,
                     Font::FontStyleFlags::plain);
    }
    
private:
    void paint(Graphics &g) override
    {
        g.fillAll(juce::Colours::white);
        auto ti = prov_->GetTransportInfo();
        std::stringstream ss;
        ss
        << "Smp: " << std::setw(8) << ti.timeInSamples << " | "
        << "Ms : " << std::setw(8) << ti.timeInSeconds << std::endl;
        g.setFont(font_);
        g.drawText(ss.str(), 0, 0, getWidth(), getHeight(), Justification::centredLeft);
    }
    
    void timerCallback() override
    {
        repaint();
    }
    
private:
    ITransportInfoProvider const *prov_;
    Font font_;
};

class DeviceSettingWindow
:   juce::DocumentWindow
{
public:
    DeviceSettingWindow(AudioDeviceManager &mgr,
                        std::function<void()> on_close)
    :   DocumentWindow("Audio Device Setting",
                       juce::Colours::black,
                       DocumentWindow::TitleBarButtons::allButtons)
    ,   c_(mgr, 0, 0, 1, 2, false, false, true, true)
    ,   on_close_(on_close)
    {
        c_.setBounds(0, 0, 500, 500);
        setUsingNativeTitleBar(true);
        setContentOwned(&c_, true);
        setResizable(true, false);
        setVisible(true);
    }
    
    void userTriedToCloseWindow() override
    {
        on_close_();
    }
    
private:
    juce::AudioDeviceSelectorComponent c_;
    std::function<void()> on_close_;
};

template<class T>
struct mini_optional
{
    mini_optional(T value)
    {
        new(value_) T(std::move(value));
        is_initialized_ = true;
    }
    
    mini_optional & operator=(T const &value)
    {
        reset();
        new(value_) T(value);
        is_initialized_ = true;
        
        return *this;
    }
    
    mini_optional & operator=(T &&value)
    {
        reset();
        new(value_) T(std::move(value));
        is_initialized_ = true;
        
        return *this;
    }
    
    mini_optional() = default;
    ~mini_optional()
    {
        reset();
    }
    
    mini_optional(mini_optional const &rhs)
    {
        if(rhs) {
            *this = rhs.get();
        }
    }
    
    mini_optional & operator=(mini_optional const &rhs)
    {
        if(rhs) {
            *this = rhs.get();
        } else {
            reset();
        }
        
        return *this;
    }
    
    mini_optional(mini_optional &&rhs)
    {
        if(rhs) {
            *this = rhs.get();
            rhs.reset();
        }
    }
    
    mini_optional & operator=(mini_optional &&rhs)
    {
        if(rhs) {
            *this = rhs.get();
            rhs.reset();
        } else {
            reset();
        }
        
        return *this;
    }
    
    void swap(mini_optional &rhs)
    {
        if(*this && rhs) {
            swap(get(), rhs.get());
        } else if(*this) {
            rhs = std::move(*this);
        } else if(rhs) {
            *this = std::move(rhs);
        }
    }
    
    explicit operator bool() const noexcept { return is_initialized_; }
    bool operator!() const noexcept { return !is_initialized_; }
    
    T & get() noexcept
    {
        assert(is_initialized_);
        return *get_ptr();
    }
                 
     T const & get() const noexcept
     {
         assert(is_initialized_);
         return *get_ptr();
     }
    
    T * get_ptr() noexcept
    {
        return reinterpret_cast<T*>(value_);
    }
    
    T const * get_ptr() const noexcept
    {
        return reinterpret_cast<T const *>(value_);
    }
    
    void reset()
    {
        if(!is_initialized_) { return; }
        get_ptr()->~T();
        is_initialized_ = false;
    }
    
private:
    alignas(T) char value_[sizeof(T)];
    bool is_initialized_ = false;
};

class ARAPluginEditorComponent
:   public juce::Component
{
public:
    //! editor will be owned by this class.
    ARAPluginEditorComponent(juce::AudioProcessorEditor *editor)
    :   editor_(editor)
    {
        assert(editor_);
        addAndMakeVisible(editor_.get());
    }
    
    void constrainBounds(Rectangle<int> &r)
    {
        if(!already_checked_) {
            auto orig = editor_->getLocalBounds();
            editor_->setBounds(0, 0, 1, 1);
            auto rmin = editor_->getLocalBounds();
            editor_->setBounds(0, 0, 65535, 65535);
            auto rmax = editor_->getLocalBounds();
            
            editor_->setResizeLimits(rmin.getWidth(),
                                     rmin.getHeight(),
                                     rmax.getWidth(),
                                     rmax.getHeight());
            already_checked_ = true;
        }
        
        auto c = editor_->getConstrainer();
        r.setTop(0);
        r.setLeft(0);
        r.setWidth(juce::jlimit(c->getMinimumWidth(),
                                INT_MAX,
                                r.getWidth()));
        r.setHeight(juce::jlimit(c->getMinimumHeight(),
                                 INT_MAX,
                                 r.getHeight()));
    }
    
private:
    void resized() override
    {
        editor_->setBounds(getLocalBounds());
    }
    
private:
    std::unique_ptr<juce::AudioProcessorEditor> editor_;
    bool already_checked_ = false;
};

//==============================================================================
class MainContentComponent
:   public AudioAppComponent
,   public Button::Listener
,   public ITransportInfoProvider
,   public ARATransporter
,   Timer
{
    enum {
        kNoRequest = -1,
    };
    
    struct TransportRequest
    {
        struct TimeRange
        {
            double begin_;
            double duration_;
            
            double end() { return begin_ + duration_; }
        };

        std::mutex mutable mtx_;
        std::unique_lock<std::mutex> get_lock() const
        {
            return std::unique_lock<std::mutex>(mtx_);
        }
        
        mini_optional<double> position_;
        mini_optional<bool> start_;
        mini_optional<TimeRange> cycle_range_;
        mini_optional<bool> enable_cycle_;
    };
    
    struct PlayHead : juce::AudioPlayHead
    {
        PlayHead(std::atomic<TransportInfo> *ti)
        :   ti_(ti)
        {}
        
        //! called only on the audio thread.
        bool getCurrentPosition(CurrentPositionInfo &result) override
        {
            result = ti_->load();
            return true;
        }
        
    private:
        std::atomic<TransportInfo> *ti_;
    };
    
    TransportRequest tp_request_;
    PlayHead play_head_;
    
public:
    //==============================================================================
    MainContentComponent()
    :   sampleRate (0.0)
    ,   expectedSamplesPerBlock (0)
    ,   lastMousePosition(0, 0)
    ,   play_head_(&transport_info_)
    ,   indicator_(this)
    {
        setSize (800, 600);

        // Specify the number of input and output channels that we want to open.
        setAudioChannels (0, 2);
        
        addAndMakeVisible(btn_play_);
        btn_play_.addListener(this);
        
        addAndMakeVisible(btn_stop_);
        btn_stop_.addListener(this);
        
        addAndMakeVisible(indicator_);
        
        addAndMakeVisible(btn_load_ara_);
        btn_load_ara_.addListener(this);
        
        addAndMakeVisible(btn_device_);
        btn_device_.addListener(this);
        
        btn_play_.setButtonText("Play");
        btn_stop_.setButtonText("Stop");
        btn_load_ara_.setButtonText("Load ARA");
        btn_device_.setButtonText("Device");
        btn_play_.setClickingTogglesState(true);
        btn_load_ara_.setClickingTogglesState(true);
        
        afm_.registerBasicFormats();
        
        WildcardFileFilter wildcardFilter ("*.wav;*.mp3;*.m4a", String(), "Audio files");
        FileBrowserComponent browser (FileBrowserComponent::canSelectFiles|FileBrowserComponent::openMode,
                                      File(),
                                      &wildcardFilter,
                                      nullptr);
        FileChooserDialogBox dialogBox ("Open an audio file",
                                        "Please choose an audio file to analyze...",
                                        browser,
                                        false,
                                        Colours::lightgrey);
        
        if(!dialogBox.show()) { exit(0); }
        
        File const file = browser.getSelectedFile (0);

        reader_.reset(afm_.createReaderFor(file));
        assert(reader_);
        
        g_reader = reader_.get();
        g_transporter = this;
        
        kTestAudioSourceSampleRate = reader_->sampleRate;
        kTestAudioSourceDuration = reader_->lengthInSamples / (double)reader_->sampleRate;
        {
            auto lock = tp_request_.get_lock();
            tp_request_.enable_cycle_ = true;
            tp_request_.cycle_range_ = TransportRequest::TimeRange{0.0, reader_->lengthInSamples / (double)reader_->sampleRate};
        }
            
        thumbnail_cache_ = std::make_unique<AudioThumbnailCache>(10);
        thumbnail_ = std::make_unique<AudioThumbnail>(512, afm_, *thumbnail_cache_);
        thumbnail_->setSource(new FileInputSource(file));

        startTimer(100);
    }

    ~MainContentComponent()
    {
        CloseDeviceSettingWindow();
        shutdownAudio();
    }
    
    void RequestStartPlayback() override
    {
        auto lock = tp_request_.get_lock();
        tp_request_.start_ = true;
    }
    
    void RequestStopPlayback() override
    {
        auto lock = tp_request_.get_lock();
        tp_request_.start_ = false;
    }
    
    void RequestSetPlaybackPosition(Time pos) override
    {
        auto lock = tp_request_.get_lock();
        tp_request_.position_ = pos;
    }
    
    void RequestSetCycleRange(Time begin, Time duration) override
    {
        auto lock = tp_request_.get_lock();
        tp_request_.cycle_range_ = TransportRequest::TimeRange{begin, duration};
    }
    
    void RequestEnableCycle(bool enabled) override
    {
        auto lock = tp_request_.get_lock();
        tp_request_.enable_cycle_ = enabled;
    }

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        std::cout << "prepareToPlay(" << samplesPerBlockExpected << ", " << newSampleRate << ")" << std::endl;
        sampleRate = newSampleRate;
        expectedSamplesPerBlock = samplesPerBlockExpected;
        
        TransportInfo ti;
        ti.resetToDefault();
        
        UpdateTransportInfo(ti);
        loop_start_sample_ = 0;
        loop_end_sample_ = 0;
        
        output_buffer_.setSize(2, samplesPerBlockExpected);
        setupRendering();
    }

    /*  This method generates the actual audio samples.
        In this example the buffer is filled with a sine wave whose frequency and
        amplitude are controlled by the mouse position.
     */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
        
        auto ti = GetTransportInfo();
        
        {
            auto lock = tp_request_.get_lock();
            if(tp_request_.start_) {
                ti.isPlaying = tp_request_.start_.get(); // todo (playing statusに変更）
                tp_request_.start_.reset();
            }
            
            if(tp_request_.position_) {
                ti.timeInSamples = SecondToSample(tp_request_.position_.get());
                ti.timeInSeconds = SampleToSecond(ti.timeInSamples);
                tp_request_.position_.reset();
            }
            
            if(tp_request_.cycle_range_) {
                loop_start_sample_ = SecondToSample(tp_request_.cycle_range_.get().begin_);
                ti.ppqLoopStart = SampleToPPQ(loop_start_sample_);
                loop_end_sample_ = SecondToSample(tp_request_.cycle_range_.get().end());
                ti.ppqLoopEnd = SampleToPPQ(loop_end_sample_);
            }
            
            if(tp_request_.enable_cycle_) {
                ti.isLooping = tp_request_.enable_cycle_.get();
                tp_request_.enable_cycle_.reset();
            }
        }

        if(ti.isPlaying) {
            int length = bufferToFill.numSamples;
            int start = bufferToFill.startSample;
            
            for( ; length != 0; ) {
                int num_to_process = 0;
                if(ti.isLooping && ti.timeInSamples < loop_end_sample_) {
                    num_to_process = std::min<juce::int64>(ti.timeInSamples + length, loop_end_sample_) - ti.timeInSamples;
                } else {
                    num_to_process = length;
                }
                
                if(plugInInstance) {
                    renderARAPlugin(ti.timeInSamples, num_to_process);
                    for(int ch = 0; ch < 2; ++ch) {
                        bufferToFill.buffer->copyFrom(ch, start, output_buffer_, ch, 0, num_to_process);
                    }
                } else if(reader_) {
                    reader_->read(bufferToFill.buffer, start, num_to_process, ti.timeInSamples, true, true);
                }
                
                if(ti.isLooping && ti.timeInSamples + num_to_process == loop_end_sample_) {
                    ti.timeInSamples = loop_start_sample_;
                } else {
                    ti.timeInSamples += num_to_process;
                }
                
                length -= num_to_process;
                start += num_to_process;
            }
        }
        UpdateTransportInfo(ti);
    }

    void releaseResources() override
    {
        // This gets automatically called when audio device parameters change
        // or device is restarted.
    }


    //==============================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

        // Draw an ellipse based on the mouse position and audio volume
        g.setColour (Colours::lightgreen);

        if(thumbnail_) {
            auto b = getBounds();
            b.removeFromTop(20);
            if(ara_instance_) {
                b = b.withHeight(b.getHeight()/2);
            }
            b = b.reduced(2);
            thumbnail_->drawChannels(g, b, 0, reader_->lengthInSamples / (double)reader_->sampleRate, 1.0);
        }
        
        auto ti = GetTransportInfo();
        if(ti.isLooping) {
            auto x1 = sampleToX(PPQToSample(ti.ppqLoopStart));
            auto x2 = sampleToX(PPQToSample(ti.ppqLoopEnd));
            
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            juce::Rectangle<float> r(0, 0, x1, getHeight());
            g.fillRect(r);
            
            r.setX(x2);
            r.setRight(getWidth());
            g.fillRect(r);
        }
        
        g.setColour (Colours::lightblue);

        auto x = sampleToX(ti.timeInSamples);
        g.drawLine(x, 0.0, x, getHeight());
    }
    
    float sampleToX(int sample_count) const
    {
        if(!reader_) { return 0; }
        return sample_count / (double)reader_->lengthInSamples * getWidth();
    }
    
    int xToSample(float x) const
    {
        if(!reader_) { return 0; }
        return x / getWidth() * reader_->lengthInSamples;
    }

    // Mouse handling..
    void mouseDown (const MouseEvent& e) override
    {
        lastMousePosition = e.position;
        mouseDrag (e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        auto lock = tp_request_.get_lock();
        tp_request_.position_ = xToSample(e.position.x) / sampleRate;
        repaint();
    }

    void mouseUp (const MouseEvent& e) override
    {
        repaint();
    }

    void resized() override
    {
        // This is called when the MainContentComponent is resized.
        // If you add any child components, this is where you should
        // update their positions.
        auto const btn_height = 20;
        auto const btn_width = 60;
        auto const indicator_height = 20;
        auto const indicator_width = 300;
        
        btn_stop_.setBounds(0, 0, btn_width, btn_height);
        btn_play_.setBounds(60, 0, btn_width, btn_height);
        indicator_.setBounds(120, 0, indicator_width, indicator_height);
        btn_load_ara_.setBounds(getWidth() - 120, 0, btn_width, indicator_height);
        btn_device_.setBounds(getWidth() - 60, 0, btn_width, indicator_height);
        
        if(plugin_editor_window_) {
            auto const editor_height_limit = getHeight() - btn_height;
            auto b
            = getLocalBounds()
            .withTop(btn_height)
            .withHeight(editor_height_limit / 2)
            .withY(0);
            plugin_editor_window_->constrainBounds(b);
            auto top = btn_height + std::max<int>(0, editor_height_limit - b.getHeight());
            plugin_editor_window_->setBounds(b.withY(top));
 
            auto test = getLocalBounds();
            test.setHeight(std::max<int>(test.getHeight(), b.getHeight() + btn_height));
            test.setWidth(std::max<int>(test.getWidth(), b.getWidth()));
            
            if(test != getLocalBounds()) {
                setBounds(test);
            }
        }
    }

    void buttonClicked(Button *btn) override
    {
        if(btn == &btn_play_) {
            auto ti = GetTransportInfo();
            auto lock = tp_request_.get_lock();
            tp_request_.start_ = !ti.isPlaying;
        } else if(btn == &btn_stop_) {
            auto ti = GetTransportInfo();
            auto lock = tp_request_.get_lock();
            tp_request_.start_ = false;
            
            if(ti.isPlaying) {
                tp_request_.position_ = xToSample(lastMousePosition.x) / (double)sampleRate;
            } else {
                tp_request_.position_ = 0;
            }
            btn_play_.setState(Button::ButtonState::buttonNormal);
        } else if(btn == &btn_load_ara_) {
            if(btn_load_ara_.getToggleState()) {
                loadARAPlugin();
                btn_load_ara_.setButtonText("Unload ARA");
            } else {
                unloadARAPlugin();
                btn_load_ara_.setButtonText("Load ARA");
            }
        } else if(btn == &btn_device_) {
            ShowDeviceSettingWindow();
        }
        
        repaint();
    }
    
    void ShowDeviceSettingWindow()
    {
        device_setting = std::make_unique<DeviceSettingWindow>(deviceManager,
                                                               [this] { CloseDeviceSettingWindow(); }
                                                               );
        btn_device_.setEnabled(false);
    }
    
    void CloseDeviceSettingWindow()
    {
        device_setting.reset();
        btn_device_.setEnabled(true);
    }
    
    void timerCallback() override
    {
        repaint();
        
        if(GetTransportInfo().isPlaying) {
            startTimer(16);
        } else {
            startTimer(100);
        }
    }
    
private:
    //==========================================================
    
    int loadARAPlugin()
    {
        juce::AudioPluginFormatManager mgr;
        mgr.addDefaultFormats();

        KnownPluginList result;
        VST3PluginFormat target_format;
        FileSearchPath directories_to_search;
        directories_to_search.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
        bool search_recursively = false;
        File dead_mans_pedal_file;
        juce::PluginDirectoryScanner scanner(result,
                                             target_format,
                                             directories_to_search,
                                             search_recursively,
                                             dead_mans_pedal_file);
        
        juce::String name = scanner.getNextPluginFileThatWillBeScanned();
        for( ; ; ) {
            // scan only melodyne
            if(name.contains("Melodyne")) {
                if(!scanner.scanNextFile(true, name)) { break; }
            } else {
                if(!scanner.skipNextFile()) { break; }
                name = scanner.getNextPluginFileThatWillBeScanned();
            }
        }
        
        auto found = std::find_if(result.begin(),
                                  result.end(),
                                  [](auto const &entry) {
                                      return entry->name == "Melodyne";
                                  });
        
        assert(found != result.end());
        
        juce::String error_msg;
        auto plugin = mgr.createPluginInstance(**found, sampleRate, expectedSamplesPerBlock, error_msg);
        ARA_INTERNAL_ASSERT(plugin != nullptr);

        ara_instance_ = std::make_unique<ARAPluginInstance>(std::unique_ptr<juce::AudioPluginInstance>(plugin));
        factory = ara_instance_->GetARAFactory();
        
        if (factory == NULL)
        {
            ARA_WARN("TestHost: this plug-in doesn't support ARA.");
            return -1;				// this plug-in doesn't support ara.
        }
        ARA_VALIDATE_CONDITION(factory->structSize >= kARAFactoryMinSize);
        
        if (factory->lowestSupportedApiGeneration > kARACurrentAPIGeneration)
        {
            ARA_WARN("TestHost: this plug-in only supports newer generations of ARA.");
            return -1;				// this plug-in doesn't support our generation ara.
        }
        if (factory->highestSupportedApiGeneration < kARACurrentAPIGeneration)
        {
            ARA_WARN("TestHost: this plug-in only supports older generations of ARA.");
            return -1;				// this plug-in doesn't support our generation ara.
        }
        
        AraSetExternalAssertReference(assertFunctionReference);
        
        ARA_VALIDATE_CONDITION(factory->factoryID != NULL);
        ARA_VALIDATE_CONDITION(strlen(factory->factoryID) > 5);	// at least "xx.y." needed to form a valid url-based unique id
        ARA_VALIDATE_CONDITION(factory->initializeARAWithConfiguration != NULL);
        ARA_VALIDATE_CONDITION(factory->uninitializeARA != NULL);
        ARA_VALIDATE_CONDITION(factory->createDocumentControllerWithDocument != NULL);
        
        factory->initializeARAWithConfiguration(&interfaceConfig);
        
        
        // create a document
        ARA_LOG("TestHost: creating a document and hooking up plug-in instance");
        
        documentControllerInstance = factory->createDocumentControllerWithDocument(&documentEntry, &documentProperties);
        ARA_VALIDATE_CONDITION(documentControllerInstance != NULL);
        documentControllerRef = documentControllerInstance->documentControllerRef;
        documentControllerInterface = documentControllerInstance->documentControllerInterface;
        ARA_VALIDATE_INTERFACE(documentControllerInterface, ARADocumentControllerInterface);
        
        // bind companion plug-in to document
        plugInInstance = ara_instance_->GetARAPlugInExtension(documentControllerRef);
        
        plugInExtensionRef = plugInInstance->plugInExtensionRef;
        plugInExtensionInterface = plugInInstance->plugInExtensionInterface;
        ARA_VALIDATE_INTERFACE(plugInExtensionInterface, ARAPlugInExtensionInterface);
        
        
        // start editing the document
        documentControllerInterface->beginEditing(documentControllerRef);
        
        // add a musical context to describe out timeline
        musicalContextRef = documentControllerInterface->createMusicalContext(documentControllerRef, kHostMusicalContextHostRef, &musicalContextProperties);
        
        audioSourceProperties.sampleRate = reader_->sampleRate;
        audioSourceProperties.sampleCount = reader_->lengthInSamples;
        audioSourceProperties.channelCount = reader_->numChannels;
        // add an audio source to it and an audio modification to contain the edits for this source
        audioSourceRef = documentControllerInterface->createAudioSource(documentControllerRef, kHostAudioSourceHostRef, &audioSourceProperties);
        
        audioModificationRef = documentControllerInterface->createAudioModification(documentControllerRef, audioSourceRef, kHostAudioModificationHostRef, &audioModificationProperties);
        
        // add a playback region to render modification in our musical context
        playbackRegionProperties.musicalContextRef = musicalContextRef;
        playbackRegionProperties.startInModificationTime
        = playbackRegionProperties.startInPlaybackTime
        = 0;
        playbackRegionProperties.durationInModificationTime
        = playbackRegionProperties.durationInPlaybackTime
        = reader_->lengthInSamples / (double)reader_->sampleRate;
        playbackRegionRef = documentControllerInterface->createPlaybackRegion(documentControllerRef, audioModificationRef, kHostAudioModificationHostRef, &playbackRegionProperties);
        
        // done with editing the document, allow plug-in to access the audio
        documentControllerInterface->endEditing(documentControllerRef);
        documentControllerInterface->enableAudioSourceSamplesAccess(documentControllerRef, audioSourceRef, kARATrue);
        
        setupRendering();
        
        auto editor = ara_instance_->GetBase()->createEditor();
        if(editor) {
            plugin_editor_window_ = std::make_unique<ARAPluginEditorComponent>(editor);
            addAndMakeVisible(plugin_editor_window_.get());
            resized();
        }
        
        // --- from here on, the model is set up and analysis can be used - actual rendering however requiers the following render setup too. ---
        return 0;
    }
    
    void setupRendering()
    {
        if(!plugInInstance) { return; }
        
        // prepare rendering
        ARA_LOG("TestHost: setting up rendering.");
        
        playback_region_was_set_ = true;
        plugInExtensionInterface->setPlaybackRegion(plugInExtensionRef, playbackRegionRef);
        
        ara_instance_->GetBase()->setPlayHead(&play_head_);
        ara_instance_->GetBase()->prepareToPlay(sampleRate, expectedSamplesPerBlock);
    }
    
    void renderARAPlugin(int sample_pos, int length)
    {
#if TEST_RENDERING
        ARA_LOG("TestHost: performing rendering.");
        
        output_buffer_.clear();
        
        juce::AudioSampleBuffer buffer(output_buffer_.getArrayOfWritePointers(),
                                       output_buffer_.getNumChannels(),
                                       std::min<int>(length, output_buffer_.getNumSamples()));
                                       
        juce::MidiBuffer midi;
        ara_instance_->GetBase()->processBlock(buffer, midi);
        
#endif	// TEST_RENDERING
        
        
        // --- the world is set up, every thing is good to go - real code would do something useful with the plug-in now. ---
    }
    
    void readContentFromARAPlugin()
    {
#if TEST_CONTENTREAD
        ARA_LOG("TestHost: testing analysis and content reading");
        documentControllerInterface->requestAudioSourceContentAnalysis(documentControllerRef, audioSourceRef,
                                                                       factory->analyzeableContentTypesCount, factory->analyzeableContentTypes);
        
        while (1)
        {
            ARABool allDone = kARATrue;
            ARASize i;
            
            // this is a crude test implementation - real code wouldn't implement such a simple infinite loop.
            // instead, it would periodically request notifications and evaluate incoming calls to notifyAudioSourceContentChanged().
            // further, it would evaluate notifyAudioSourceAnalysisProgress() to provide proper progress indication,
            // and offer the user a way to cancel the operation if desired.
            documentControllerInterface->notifyModelUpdates(documentControllerRef);
            
            for (i = 0; i < factory->analyzeableContentTypesCount; ++i)
            {
                if (documentControllerInterface->isAudioSourceContentAnalysisIncomplete(documentControllerRef, audioSourceRef, factory->analyzeableContentTypes[i]))
                {
                    allDone = kARAFalse;
                    break;
                }
            }
            if (allDone)
                break;
            
#if defined(_MSC_VER)
            Sleep(100);
#elif defined(__APPLE__)
            usleep(100000);
#endif
        }
        
        testAudioSourceContentReaders(documentControllerInterface, documentControllerRef, audioSourceRef);
#endif	// TEST_CONTENTREAD
    }
    
    void timeStretchARAPlugin()
    {
#if TEST_TIMESTRETCH
        if (factory->supportedPlaybackTransformationFlags & kARAPlaybackTransformationTimestretch)
        {
            ARA_LOG("TestHost: testing time stretching");
            documentControllerInterface->beginEditing(documentControllerRef);
            playbackRegionProperties.transformationFlags |= kARAPlaybackTransformationTimestretch;
            playbackRegionProperties.durationInPlaybackTime *= 0.5;
            documentControllerInterface->updatePlaybackRegionProperties(documentControllerRef, playbackRegionRef, &playbackRegionProperties);
            documentControllerInterface->endEditing(documentControllerRef);
            
#if TEST_CONTENTREAD
            testPlaybackRegionNoteContentReader(documentControllerInterface, documentControllerRef, playbackRegionRef);
#endif
        }
#endif	// TEST_TIMESTRETCH

    }
    
    void modificationCloningARAPlugin()
    {
#if TEST_MODIFICATIONCLONING
        ARA_LOG("TestHost: testing cloning modifications");
        documentControllerInterface->beginEditing(documentControllerRef);
        audioModificationRef2 = documentControllerInterface->cloneAudioModification(documentControllerRef, audioModificationRef, 0/*hostRef*/, &audioModificationProperties2);
        playbackRegionProperties2.musicalContextRef = musicalContextRef;
        playbackRegionRef2 = documentControllerInterface->createPlaybackRegion(documentControllerRef, audioModificationRef2, 0/*hostRef*/, &playbackRegionProperties2);
        documentControllerInterface->endEditing(documentControllerRef);
        
#if TEST_CONTENTREAD
        testPlaybackRegionNoteContentReader(documentControllerInterface, documentControllerRef, playbackRegionRef2);
#endif
        
        documentControllerInterface->beginEditing(documentControllerRef);
        documentControllerInterface->destroyPlaybackRegion(documentControllerRef, playbackRegionRef2);
        documentControllerInterface->destroyAudioModification(documentControllerRef, audioModificationRef2);
        documentControllerInterface->endEditing(documentControllerRef);
#endif	// TEST_MODIFICATIONCLONING
    }
    
    void unarchiveARAPlugin()
    {
#if TEST_UNARCHIVING
        ARA_LOG("TestHost: archiving the document");
        
        setupTestArchive(&archive);
        archiveSuccess = documentControllerInterface->storeDocumentToArchive(documentControllerRef, &archive);
        ARA_VALIDATE_CONDITION(archiveSuccess);
#endif
    }
    
    void unloadARAPlugin()
    {
        // shut everything down again
        ARA_LOG("TestHost: destroying the document again");
        
#if TEST_RENDERING

        ara_instance_->GetBase()->releaseResources();
        plugin_editor_window_.reset();
        
        if(playback_region_was_set_) {
            plugInExtensionInterface->removePlaybackRegion(plugInExtensionRef, playbackRegionRef);
        }
        
#endif	// TEST_RENDERING
        
        documentControllerInterface->enableAudioSourceSamplesAccess(documentControllerRef, audioSourceRef, kARAFalse);
        
        documentControllerInterface->beginEditing(documentControllerRef);
        documentControllerInterface->destroyPlaybackRegion(documentControllerRef, playbackRegionRef);
        documentControllerInterface->destroyAudioModification(documentControllerRef, audioModificationRef);
        documentControllerInterface->destroyAudioSource(documentControllerRef, audioSourceRef);
        documentControllerInterface->destroyMusicalContext(documentControllerRef, musicalContextRef);
        documentControllerInterface->endEditing(documentControllerRef);
        documentControllerInterface->destroyDocumentController(documentControllerRef);
        
        factory->uninitializeARA();

        ara_instance_.reset();
        ARA_LOG("TestHost: teardown completed");
    }

private:
    //==============================================================================
    double sampleRate = 0;
    int expectedSamplesPerBlock = 0;
    Point<float> lastMousePosition;
    int loop_start_sample_;
    int loop_end_sample_;

    std::atomic<TransportInfo> transport_info_;
    
    TransportInfo GetTransportInfo() const override {
        return transport_info_.load();
    }
    
    int PPQToSample(double ppq_pos) const override
    {
        auto const tempo = 120.0; // todo: implement tempo automation and use it here.
        auto const sec_per_beat = 60.0 / tempo;
        return SecondToSample(ppq_pos * sec_per_beat);
    }
    
    double SampleToPPQ(int sample_pos) const override
    {
        auto const tempo = 120.0; // todo: implement tempo automation and use it here.

        auto const sec = SampleToSecond(sample_pos);
        auto beat_per_sec = tempo / 60.0;
        return sec * beat_per_sec;
    }
    
    int SecondToSample(double sec) const override
    {
        return (int)std::floor(sec * sampleRate);
    }
    
    double SampleToSecond(int sample) const override
    {
        assert(sampleRate != 0);
        return sample / sampleRate;
    }
    
    void UpdateTransportInfo(TransportInfo const &ti) {
        transport_info_.store(ti);
    }
    
    TextButton btn_play_;
    TextButton btn_stop_;
    TextButton btn_load_ara_;
    IndicatorComponent indicator_;
    AudioFormatManager afm_;
    std::unique_ptr<AudioFormatReader> reader_;
    std::unique_ptr<AudioThumbnail> thumbnail_;
    std::unique_ptr<AudioThumbnailCache> thumbnail_cache_;
    std::unique_ptr<ARAPluginEditorComponent> plugin_editor_window_;
    std::unique_ptr<DeviceSettingWindow> device_setting;
    TextButton btn_device_;
    
    //=================================================
    
    const ARAInterfaceConfiguration interfaceConfig = { sizeof(interfaceConfig), kARACurrentAPIGeneration, assertFunctionReference };
    
    const ARAPlugInExtensionInstance * plugInInstance = NULL;
    ARAPlugInExtensionRef plugInExtensionRef = nullptr;
    const ARAPlugInExtensionInterface * plugInExtensionInterface = NULL;
    
    const ARAFactory * factory = NULL;
    
    const ARADocumentControllerInstance * documentControllerInstance = NULL;
    ARADocumentControllerRef documentControllerRef = nullptr;
    const ARADocumentControllerInterface * documentControllerInterface = NULL;
    
    ARADocumentControllerHostInstance documentEntry = { sizeof(documentEntry),
        kAudioAccessControllerHostRef, &hostAudioAccessControllerInterface,
        kArchivingControllerHostRef, &hostArchivingInterface,
        kContentAccessControllerHostRef, &hostContentAccessControllerInterface,
        kModelUpdateControllerHostRef, &hostModelUpdateControllerInterface,
        kPlaybackControllerHostRef, &hostPlaybackControllerInterface };
    
    ARADocumentProperties documentProperties = { sizeof(documentProperties), "Test document" };
    
    ARAMusicalContextProperties musicalContextProperties = { sizeof(musicalContextProperties) };
    ARAMusicalContextRef musicalContextRef = nullptr;
    
    ARAAudioSourceProperties audioSourceProperties = { sizeof(audioSourceProperties), "Test audio source", "audioSourceTestPersistentID",
        kTestAudioSourceSampleRate * kTestAudioSourceDuration, (double)kTestAudioSourceSampleRate, 1, kARAFalse };
    ARAAudioSourceRef audioSourceRef = nullptr;
    
    ARAAudioModificationProperties audioModificationProperties = { sizeof(audioModificationProperties),
        "Test audio modification", "audioModificationTestPersistentID" };
    ARAAudioModificationRef audioModificationRef = nullptr;
    
    ARAPlaybackRegionProperties playbackRegionProperties = { sizeof(playbackRegionProperties), kARAPlaybackTransformationTimestretch,
        0, 0, 0, 0, 0 /*ref must be inserted here before using the struct!*/ };
    ARAPlaybackRegionRef playbackRegionRef = nullptr;
    
#if TEST_MODIFICATIONCLONING
    ARAAudioModificationProperties audioModificationProperties2 = { sizeof(audioModificationProperties),
        "PulsedSineExample" /*name*/, "ID_AudioModification2" /*persistentID*/ };
    ARAAudioModificationRef audioModificationRef2 = nullptr;
    ARAPlaybackRegionProperties playbackRegionProperties2 = { sizeof(playbackRegionProperties), kARAPlaybackTransformationNoChanges,
        0, 0, 0, 0, 0 /*ref must be inserted here before using the struct!*/ };
    ARAPlaybackRegionRef playbackRegionRef2 = nullptr;
#endif
    
#if TEST_UNARCHIVING
    TestArchive archive;
    ARABool archiveSuccess = kARAFalse;
#endif

    std::unique_ptr<ARAPluginInstance> ara_instance_;
    
    bool playback_region_was_set_ = false;
    AudioSampleBuffer output_buffer_;
    
    //=================================================
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


Component* createMainContentComponent() { return new MainContentComponent(); };
