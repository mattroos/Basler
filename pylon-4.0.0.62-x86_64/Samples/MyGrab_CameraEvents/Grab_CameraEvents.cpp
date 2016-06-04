// Grab_CameraEvents.cpp
/*
Basler GigE Vision and IEEE 1394 cameras can send event messages. For example, when a sensor
exposure has finished, the camera can send an Exposure End event to the PC. The event
can be received by the PC before the image data for the finished exposure has been completely
transferred. This sample illustrates how to be notified when camera event message data
is received.

The event messages are automatically retrieved and processed by the InstantCamera classes.
The information carried by event messages is exposed as parameter nodes in the camera node map
and can be accessed like "normal" camera parameters. These nodes are updated
when a camera event is received. You can register camera event handler objects that are
triggered when event data has been received.

These mechanisms are demonstrated for the Exposure End and the Event Overrun events.
The  Exposure End event carries the following information:
* ExposureEndEventFrameID: Indicates the number of the image frame that has been exposed.
* ExposureEndEventTimestamp: Indicates the moment when the event has been generated.
* ExposureEndEventStreamChannelIndex: Indicates the number of the image data stream used to
transfer the exposed frame.
The Event Overrun event is sent by the camera as a warning that events are being dropped. The
notification contains no specific information about how many or which events have been dropped.
Events may be dropped if events are generated at a very high frequency and if there isn't enough
bandwidth available to send the events.

It is shown in this sample how to register event handlers indicating the arrival of events
sent by the camera. For demonstration purposes, several different handlers are registered
for the same event.
*/


// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>

// Include files used by samples.
#include "../include/ConfigurationEventPrinter.h"
#include "../include/CameraEventPrinter.h"

// Namespace for using pylon objects.
using namespace Pylon;

#if defined( USE_1394 )
// Settings to use  Basler IEEE 1394 cameras.
#include <pylon/1394/Basler1394InstantCamera.h>
typedef Pylon::CBasler1394InstantCamera Camera_t;
typedef CBasler1394CameraEventHandler CameraEventHandler_t; // Or use Camera_t::CameraEventHandler_t
using namespace Basler_IIDC1394CameraParams;
#elif defined ( USE_GIGE )
// Settings to use Basler GigE cameras.
#include <pylon/gige/BaslerGigEInstantCamera.h>
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
typedef CBaslerGigECameraEventHandler CameraEventHandler_t; // Or use Camera_t::CameraEventHandler_t
using namespace Basler_GigECameraParams;
#else
#error camera type is not specified. For example, define USE_GIGE for using GigE cameras
#endif

// Namespace for using cout.
using namespace std;

//Enumeration used for distinguishing different events.
enum MyEvents
{
    eMyExposureEndEvent  = 100,
    eMyEventOverrunEvent = 200
};

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 5;


// Example handler for camera events.
class CSampleCameraEventHandler : public CameraEventHandler_t
{
public:
    // Only very short processing tasks should be performed by this method. Otherwise, the event notification will block the
    // processing of images.
    virtual void OnCameraEvent( Camera_t& camera, intptr_t userProvidedId, GenApi::INode* /* pNode */)
    {
        std::cout << std::endl;
        switch ( userProvidedId )
        {
        case eMyExposureEndEvent: // Exposure End event
            cout << "Exposure End event. FrameID: " << camera.ExposureEndEventFrameID.GetValue() << " Timestamp: " << camera.ExposureEndEventTimestamp.GetValue() << std::endl << std::endl;
            break;
        case eMyEventOverrunEvent:  // Event Overrun event
            cout << "Event Overrun event. FrameID: " << camera.EventOverrunEventFrameID.GetValue() << " Timestamp: " << camera.EventOverrunEventTimestamp.GetValue() << std::endl << std::endl;
            break;
        }
    }
};

//Example of an image event handler.
class CSampleImageEventHandler : public CImageEventHandler
{
public:
    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
    {
        cout << "CSampleImageEventHandler::OnImageGrabbed called." << std::endl;
        cout << std::endl;
        cout << std::endl;
    }
};

int main(int argc, char* argv[])
{
    // The exit code of the sample application
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    // Create an example event handler. In the present case, we use one single camera handler for handling multiple camera events.
    // The handler prints a message for each received event.
    CSampleCameraEventHandler* pHandler1 = new CSampleCameraEventHandler;

    // Create another more generic event handler printing out information about the node for which an event callback
    // is fired.
    CCameraEventPrinter*  pHandler2 = new CCameraEventPrinter;

    try
    {
        // Only look for cameras supported by Camera_t
        CDeviceInfo info;
        info.SetDeviceClass( Camera_t::DeviceClass());

        // Create an instant camera object with the first found camera device matching the specified device class.
        Camera_t camera( CTlFactory::GetInstance().CreateFirstDevice( info));

        // Register the standard configuration event handler for enabling software triggering.
        // The software trigger configuration handler replaces the default configuration
        // as all currently registered configuration handlers are removed by setting the registration mode to RegistrationMode_ReplaceAll.
        camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);

        // For demonstration purposes only, add sample configuration event handlers to print out information
        // about camera use and image grabbing.
        camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete); // Camera use.

        // For demonstration purposes only, register another image event handler.
        camera.RegisterImageEventHandler( new CSampleImageEventHandler, RegistrationMode_Append, Cleanup_Delete);

        // Camera event processing must be activated first, the default is off.
        camera.GrabCameraEvents = true;


        // Register an event handler for the Exposure End event. For each event type, there is a "data" node
        // representing the event. The actual data that is carried by the event is held by child nodes of the
        // data node. In the case of the Exposure End event, the child nodes are ExposureEndEventFrameID, ExposureEndEventTimestamp,
        // and ExposureEndEventStreamChannelIndex. The CSampleCameraEventHandler demonstrates how to access the child nodes within
        // a callback that is fired for the parent data node.
        camera.RegisterCameraEventHandler( pHandler1, "ExposureEndEventData", eMyExposureEndEvent, RegistrationMode_ReplaceAll, Cleanup_None);

        // Register the same handler for a second event. The user-provided ID can be used
        // to distinguish between the events.
        camera.RegisterCameraEventHandler( pHandler1, "EventOverrunEventData", eMyEventOverrunEvent, RegistrationMode_Append, Cleanup_None);

        // The handler is registered for both, the ExposureEndEventFrameID and the ExposureEndEventTimestamp
        // node. These nodes represent the data carried by the Exposure End event.
        // For each Exposure End event received, the handler will be called twice, once for the frame ID, and
        // once for the time stamp.
        camera.RegisterCameraEventHandler( pHandler2, "ExposureEndEventFrameID", eMyExposureEndEvent, RegistrationMode_Append, Cleanup_None);
        camera.RegisterCameraEventHandler( pHandler2, "ExposureEndEventTimestamp", eMyExposureEndEvent, RegistrationMode_Append, Cleanup_None);


        // Open the camera for setting parameters.
        camera.Open();

        // Check if the device supports events.
        if ( !GenApi::IsAvailable( camera.EventSelector))
        {
            throw RUNTIME_EXCEPTION( "The device doesn't support events.");
        }

        // Enable sending of Exposure End events.
        // Select the event to receive.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);
        // Enable it.
        camera.EventNotification.SetValue(EventNotification_GenICamEvent);

        // Enable sending of Event Overrun events.
        camera.EventSelector.SetValue(EventSelector_EventOverrun);
        camera.EventNotification.SetValue(EventNotification_GenICamEvent);


        // Start the grabbing of c_countOfImagesToGrab images.
        camera.StartGrabbing( c_countOfImagesToGrab);

        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        while ( camera.IsGrabbing())
        {
            // Execute the software trigger. Wait up to 1000 ms for the camera to be ready for trigger.
            if ( camera.WaitForFrameTriggerReady( 1000, TimeoutHandling_ThrowException))
            {
                camera.ExecuteSoftwareTrigger();
            }

            // Retrieve grab results and notify the camera event and image event handlers.
            camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);
            // Nothing to do here with the grab result, the grab results are handled by the registered event handler.
        }

        // Disable sending Exposure End events.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);
        camera.EventNotification.SetValue(EventNotification_Off);

        // Disable sending Event Overrun events.
        camera.EventSelector.SetValue(EventSelector_EventOverrun);
        camera.EventNotification.SetValue(EventNotification_Off);
    }
    catch (GenICam::GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Delete the event handlers.
    delete pHandler1;
    delete pHandler2;

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    return exitCode;
}

