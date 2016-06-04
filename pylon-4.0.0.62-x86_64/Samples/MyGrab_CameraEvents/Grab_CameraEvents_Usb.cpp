// Grab_CameraEvents_Usb.cpp
/* 
Basler USB3 Vision cameras can send event messages. For example, when a sensor 
exposure has finished, the camera can send an Exposure End event to the PC. The event 
can be received by the PC before the image data for the finished exposure has been completely 
transferred. This sample illustrates how to be notified when camera event message data
is received.

The event messages are automatically retrieved and processed by the InstantCamera classes.
The information carried by event messages is exposed as parameter nodes in the camera node map 
and can be accessed like "normal" camera parameters. These nodes are updated
when a camera event is received. You can register camera event handler objects that are
triggered when event data has been received.

These mechanisms are demonstrated for the Exposure End event.
The  Exposure End event carries the following information: 
* EventExposureEndFrameID: Indicates the number of the image frame that has been exposed. 
* EventExposureEndTimestamp: Indicates the moment when the event has been generated. 
transfer the exposed frame.

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

#if defined( USE_USB )
// Settings to use Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
typedef CBaslerUsbCameraEventHandler CameraEventHandler_t; // Or use Camera_t::CameraEventHandler_t
using namespace Basler_UsbCameraParams;
#else
#error camera type is not specified. For example, define USE_USB for using USB cameras
#endif

// Namespace for using cout.
using namespace std;

using namespace GenApi; // needed for Pylon image read/write


//Enumeration used for distinguishing different events.
enum MyEvents
{
    eMyExposureEndEvent  = 100,
    eMyFrameStartEvent = 200
    // More events can be added here.
};

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 4;


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
            cout << "Exposure End event. FrameID: " << camera.EventExposureEndFrameID.GetValue() << " Timestamp: " << camera.EventExposureEndTimestamp.GetValue() << std::endl << std::endl;
            break;
        case eMyFrameStartEvent: // Frame Start event
            cout << "Frame Start event. Timestamp: " << camera.EventFrameStartTimestamp.GetValue() << std::endl << std::endl;
            break;
        // More events can be added here.
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
        char frameFilename[100];
        uint16_t frameNumber = (uint16_t)ptrGrabResult->GetBlockID();
        sprintf(frameFilename,"GrabbedImage_%.2d.png",frameNumber);                
        CImagePersistence::Save( ImageFileFormat_Png, frameFilename, ptrGrabResult);
        cout << "Saved image to file: " << frameFilename << endl << endl;
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

        camera.RegisterConfiguration( new CAcquireContinuousConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
        camera.RegisterImageEventHandler( new CSampleImageEventHandler, RegistrationMode_Append, Cleanup_Delete);
        camera.GrabCameraEvents = true;

        // Register an event handler for the Exposure End and Frame Start events
        camera.RegisterCameraEventHandler( pHandler1, "EventExposureEndData", eMyExposureEndEvent, RegistrationMode_ReplaceAll, Cleanup_None);
        camera.RegisterCameraEventHandler( pHandler1, "EventFrameStartData", eMyFrameStartEvent, RegistrationMode_ReplaceAll, Cleanup_None);

        camera.RegisterCameraEventHandler( pHandler2, "EventExposureEndFrameID", eMyExposureEndEvent, RegistrationMode_Append, Cleanup_None);
        camera.RegisterCameraEventHandler( pHandler2, "EventExposureEndTimestamp", eMyExposureEndEvent, RegistrationMode_Append, Cleanup_None);
        camera.RegisterCameraEventHandler( pHandler2, "EventFrameStartTimestamp", eMyFrameStartEvent, RegistrationMode_Append, Cleanup_None);

        // Open the camera for setting parameters.
        camera.Open();


        // Set camera for hardware Frame Burst Start Trigger (code from Ace USB Users Manual)
        camera.AcquisitionMode.SetValue( AcquisitionMode_Continuous );
        camera.TriggerSelector.SetValue( TriggerSelector_FrameStart );
        camera.TriggerMode.SetValue( TriggerMode_Off );
        camera.TriggerSelector.SetValue( TriggerSelector_FrameBurstStart );
        camera.TriggerMode.SetValue( TriggerMode_On );
        camera.TriggerSource.SetValue ( TriggerSource_Line3 );
        camera.TriggerActivation.SetValue( TriggerActivation_FallingEdge );
        camera.AcquisitionBurstFrameCount.SetValue( c_countOfImagesToGrab );    // max value is 255
        camera.ExposureMode.SetValue( ExposureMode_Timed );
        camera.ExposureTime.SetValue( 20000.0 );
        // camera.AcquisitionStart.Execute( );  // MJR: Don't think this is necessary. Called by camera.StartGrabbing()?
        // while ( ! finished )
        // {
        //     // Apply a rising edge of the externally generated electrical signal
        //     // (ExFBTrig signal) to input line Line 1 on the camera

        //     // Perform the required functions to parameterize the frame start
        //     // trigger, to trigger 5 frame starts, and to retrieve 5 frames here
        // }
        // camera.AcquisitionStop.Execute( );


        // Check if the device supports events.
        if ( !GenApi::IsAvailable( camera.EventSelector))
        {
            throw RUNTIME_EXCEPTION( "The device doesn't support events.");
        }

        // Enable sending of Exposure End events.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);   // Select the event to receive.
        camera.EventNotification.SetValue(EventNotification_On);    // Enable it.

        // Enable sending of Frame Start events.        
        camera.EventSelector.SetValue(EventSelector_FrameStart);    // Select the event to receive.        
        camera.EventNotification.SetValue(EventNotification_On);    // Enable it.


        // Start the grabbing of c_countOfImagesToGrab images.
        camera.LineSelector.SetValue(LineSelector_Line3);
        camera.StartGrabbing( c_countOfImagesToGrab, GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
        int cnt = 0;
        // while ( camera.IsGrabbing() && cnt < 10000)
        while ( camera.IsGrabbing() )
        {
            bool bLineStatus = camera.LineStatus.GetValue();
            // Get info about single Line
            cout << cnt++ << ": " << bLineStatus << endl;
        }

        
        camera.StopGrabbing();              // MJR: Don't think this is necessary
        camera.AcquisitionStop.Execute( );  // MJR: Don't think this is necessary


        // Disable sending Exposure End events.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);
        camera.EventNotification.SetValue(EventNotification_Off);

        // Disable sending Frame Start events.
        camera.EventSelector.SetValue(EventSelector_FrameStart);
        camera.EventNotification.SetValue(EventNotification_Off);

        camera.Close();
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

    // // Comment the following two lines to disable waiting on exit.
    // cerr << endl << "Press Enter to exit." << endl;
    // while( cin.get() != '\n');

    return exitCode;
}

