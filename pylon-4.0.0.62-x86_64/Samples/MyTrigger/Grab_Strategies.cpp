// Grab_Strategies.cpp
/*
   This sample shows the use of the Instant Camera grab strategies.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>

// Include files used by samples.
#include "../include/ConfigurationEventPrinter.h"
#include "../include/ImageEventPrinter.h"

// Namespace for using pylon objects.
using namespace Pylon;

// Settings to use Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
typedef Pylon::CBaslerUsbCameraEventHandler CameraEventHandler_t; // Or use Camera_t::CameraEventHandler_t
typedef Pylon::CBaslerUsbImageEventHandler ImageEventHandler_t; // Or use Camera_t::ImageEventHandler_t
typedef Pylon::CBaslerUsbGrabResultPtr GrabResultPtr_t; // Or use Camera_t::GrabResultPtr_t
using namespace Basler_UsbCameraParams;


using namespace GenApi; // needed for Pylong image read/write? -MJR


// Namespace for using cout.
using namespace std;


// Enumeration used for distinguishing different events.
enum MyEvents
{
    eMyExposureEndEvent,      // Triggered by a camera event.
    eMyFrameStartOvertrigger, // Triggered by a camera event.
    eMyImageReceivedEvent,    // Triggered by the receipt of an image.
    eMyMoveEvent,             // Triggered when the imaged item or the sensor head can be moved.
    eMyNoEvent                // Used as default setting.
};
// Names of possible events for a printed output.
const char* MyEventNames[] =
{
    "ExposureEndEvent     ",
    "FrameStartOvertrigger",
    "ImageReceived        ",
    "Move                 ",
    "NoEvent              "
};



//==============================================
// WHAT PARTS OF CODE BELOW ARE REALLY NEEDED?
//==============================================

#include <sys/time.h>
#include <iomanip>      // std::setw

// Used for logging received events without outputting the information on the screen
// because outputting will change the timing.
// This class is used for demonstration purposes only.
struct LogItem
{
    LogItem()
        : eventType( eMyNoEvent)
        , frameNumber(0)
    {
    }
    LogItem( MyEvents event, uint16_t frameNr)
        : eventType(event)
        , frameNumber(frameNr)
    {
        //Warning, measured values can be wrong on older PC hardware.
#if defined(PYLON_WIN_BUILD)
        QueryPerformanceCounter(&time);
#elif defined(PYLON_LINUX_BUILD)
        struct timeval tv;
        gettimeofday(&tv, NULL);
        time = static_cast<unsigned long long>(tv.tv_sec) * 1000L + static_cast<unsigned long long>(tv.tv_usec) / 1000LL;
#endif
    }
#if defined(PYLON_WIN_BUILD)
    LARGE_INTEGER time; // Recorded time stamps.
#elif defined(PYLON_LINUX_BUILD)
    unsigned long long time; // Recorded time stamps.
#endif
    MyEvents eventType; // Type of the received event.
    uint16_t frameNumber; // Frame number of the received event.
};
// Helper function for printing a log.
// This function is used for demonstration purposes only.
void PrintLog( const std::vector<LogItem>& aLog)
{
#if defined(PYLON_WIN_BUILD)
    // Get the PC timer frequency.
    LARGE_INTEGER timerFrequency;
    QueryPerformanceFrequency(&timerFrequency);
#elif defined(PYLON_LINUX_BUILD)
#endif
    cout << std::endl << "Warning, the printed time values can be wrong on older PC hardware." << std::endl << std::endl;
    // Print the event information header.
    cout << "Time [ms]    " << "Event                 " << "FrameNumber" << std::endl;
    cout << "------------ " << "--------------------- " << "-----------" << std::endl;
    // Print the logged information.
    size_t logSize = aLog.size();
    for ( size_t i = 0; i < logSize; ++i)
    {
        // Calculate the elapsed time between the events.
        double time_ms = 0;
        if ( i)
        {
#if defined(PYLON_WIN_BUILD)
            __int64 oldTicks = ((__int64)aLog[i-1].time.HighPart << 32) + (__int64)aLog[i-1].time.LowPart;
            __int64 newTicks = ((__int64)aLog[i].time.HighPart << 32) + (__int64)aLog[i].time.LowPart;
            long double timeDifference = (long double) (newTicks - oldTicks);
            long double ticksPerSecond = (long double) (((__int64)timerFrequency.HighPart << 32) + (__int64)timerFrequency.LowPart);
            time_ms = (timeDifference / ticksPerSecond) * 1000;
#elif defined(PYLON_LINUX_BUILD)
            time_ms = aLog[i].time - aLog[i-1].time;
#endif
        }
        // Print the event information.
        cout << setw(12) << fixed << setprecision(4) << time_ms <<" "<< MyEventNames[ aLog[i].eventType ] <<" "<< aLog[i].frameNumber << std::endl;
    }
}

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 50;

// Example handler for GigE camera events.
// Additional handling is required for GigE camera events because the event network packets can be lost, doubled or delayed on the network.
class CEventHandler : public CameraEventHandler_t, public ImageEventHandler_t
{
public:
    CEventHandler()
        : m_nextExpectedFrameNumberImage(0)
        , m_nextExpectedFrameNumberExposureEnd(0)
        , m_nextFrameNumberForMove(0)
        , m_frameIDsInitialized(false)
    {
        // Reserve space to log camera, image and move events.
        m_log.reserve( c_countOfImagesToGrab * 3);
    }

    // This method is called when a camera event has been received.
    virtual void OnCameraEvent( Camera_t& camera, intptr_t userProvidedId, GenApi::INode* /* pNode */)
    {
        cout << "MJR: Start OnCameraEvent()" << endl;
        cout << "MJR: userProvidedId = " << userProvidedId << endl;
        if ( userProvidedId == eMyExposureEndEvent)
        {
            cout << "\tMJR: Start OnCameraEvent:eMyExposureEndEvent" << endl;

            // An Exposure End event has been received.
            uint16_t frameNumber = (uint16_t)camera.EventExposureEndFrameID.GetValue();
            cout << "frameNumber = " << frameNumber << endl; 
            m_log.push_back( LogItem( eMyExposureEndEvent, frameNumber));
            // If Exposure End event is not doubled.
            if ( GetIncrementedFrameNumber( frameNumber) != m_nextExpectedFrameNumberExposureEnd)
            {
                // Check whether the imaged item or the sensor head can be moved.
                if ( frameNumber == m_nextFrameNumberForMove)
                {
                    MoveImagedItemOrSensorHead();
                }
                // Check for missing Exposure End events.
                if ( frameNumber != m_nextExpectedFrameNumberExposureEnd)
                {
                    throw RUNTIME_EXCEPTION( "An Exposure End event has been lost. Expected frame number is %d but got frame number %d.", m_nextExpectedFrameNumberExposureEnd, frameNumber);
                }
                else
                {
                    cout << "Expected frame number is " << m_nextExpectedFrameNumberExposureEnd << " and got frame number " << frameNumber << endl;
                }
                IncrementFrameNumber( m_nextExpectedFrameNumberExposureEnd);
            }
            cout << "\tMJR: End OnCameraEvent:eMyExposureEndEvent" << endl;
        }
        else if ( userProvidedId == eMyFrameStartOvertrigger)
        {
            cout << "\tMJR: Start OnCameraEvent:eMyFrameStartOvertrigger" << endl;
            // The camera has been overtriggered.
            m_log.push_back( LogItem( eMyFrameStartOvertrigger, 0));
            // Handle this error...
            cout << "\tMJR: End OnCameraEvent:eMyFrameStartOvertrigger" << endl;
        }
        else if ( userProvidedId == eMyImageReceivedEvent)
        {
            cout << "\tMJR: Start OnCameraEvent:eMyImageReceivedEvent" << endl;
            cout << "\tMJR: End OnCameraEvent:eMyImageReceivedEvent" << endl;
        }
        else
        {
            PYLON_ASSERT2(false, "The sample has been modified and a new event has been registered. Add handler code above.");
            cout << "\tMJR: Start OnCameraEvent:userProvidedId Unknown" << endl;
            cout << "\tMJR: End OnCameraEvent:userProvidedId Unknown" << endl;
        }
        cout << "MJR: End OnCameraEvent()" << endl;
    }

    // This method is called when an image has been grabbed.
    virtual void OnImageGrabbed( Camera_t& camera, const GrabResultPtr_t& ptrGrabResult)
    {   
        cout << "MJR: Start OnImageGrabbed()" << endl;

        // An image has been received.
        uint16_t frameNumber = (uint16_t)ptrGrabResult->GetBlockID();
        m_log.push_back( LogItem( eMyImageReceivedEvent, frameNumber));
        // Check whether the imaged item or the sensor head can be moved.
        // This will be the case if the Exposure End has been lost or if the Exposure End is received later than the image.
        if ( frameNumber == m_nextFrameNumberForMove)
        {
            MoveImagedItemOrSensorHead();
        }
        // Check for missing images.
        if ( frameNumber != m_nextExpectedFrameNumberImage)
        {
            //throw RUNTIME_EXCEPTION( "An image has been lost. Expected frame number is %d but got frame number %d.", m_nextExpectedFrameNumberImage, frameNumber);
            cout << "Expected frame number is " << m_nextExpectedFrameNumberExposureEnd << " and got frame number " << frameNumber << endl;
        }
        cout << m_nextExpectedFrameNumberExposureEnd << " --> ";
        IncrementFrameNumber( m_nextExpectedFrameNumberImage);
        cout << m_nextExpectedFrameNumberExposureEnd << endl;
        cout << "MJR: End OnImageGrabbed()" << endl;
    }

    void MoveImagedItemOrSensorHead()
    {
        cout << "MJR: Start MoveImagedItemOrSensorHead()" << endl;
        // The imaged item or the sensor head can be moved now...
        // The camera may not be ready for a trigger at this point yet because the sensor is still being read out.
        // See the documentation of the CInstantCamera::WaitForFrameTriggerReady() method for more information.
        m_log.push_back( LogItem( eMyMoveEvent, m_nextFrameNumberForMove));
        IncrementFrameNumber( m_nextFrameNumberForMove);
        cout << "MJR: End MoveImagedItemOrSensorHead()" << endl;
    }

    void PrintLog()
    {
        ::PrintLog( m_log);
    }

private:
    void IncrementFrameNumber( uint16_t& frameNumber)
    {
        frameNumber = GetIncrementedFrameNumber( frameNumber);
    }
    uint16_t GetIncrementedFrameNumber( uint16_t frameNumber)
    {
        ++frameNumber;
        return frameNumber;
    }
    uint16_t m_nextExpectedFrameNumberImage;
    uint16_t m_nextExpectedFrameNumberExposureEnd;
    uint16_t m_nextFrameNumberForMove;
    bool m_frameIDsInitialized;
    std::vector<LogItem> m_log;
};





int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    try
    {

        // Create the event handler.
        CEventHandler eventHandler;
        // Only look for cameras supported by Camera_t. 
        CDeviceInfo info;
        info.SetDeviceClass( Camera_t::DeviceClass());
        // Create an instant camera object with the first found camera device matching the specified device class.
        Camera_t camera( CTlFactory::GetInstance().CreateFirstDevice( info));


        // // For demonstration purposes only, add sample configuration event handlers to print out information
        // // about camera use and image grabbing.
        // camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete); // Camera use.


        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // // Create an instant camera object for the camera device found first.
        // //CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());
        // CBaslerUsbInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());


        // Register the event handler.
        camera.RegisterImageEventHandler( &eventHandler, RegistrationMode_Append, Cleanup_None);
        camera.RegisterCameraEventHandler( &eventHandler, "EventExposureEndData", eMyExposureEndEvent, RegistrationMode_ReplaceAll, Cleanup_None);
        camera.RegisterCameraEventHandler( &eventHandler, "EventFrameStartOvertriggerData", eMyFrameStartOvertrigger, RegistrationMode_Append, Cleanup_None);
        // Camera event processing must be activated first, the default is off.
        camera.GrabCameraEvents = true;


        // Open the camera.
        camera.Open();  // DO THIS BEFORE SETTING CAMERA PARAMETERS ABOVE???


        camera.AcquisitionMode.SetValue( AcquisitionMode_SingleFrame );
        camera.TriggerSelector.SetValue(TriggerSelector_FrameBurstStart);
        camera.TriggerMode.SetValue( TriggerMode_Off );
        camera.TriggerSelector.SetValue( TriggerSelector_FrameStart );
        camera.TriggerMode.SetValue( TriggerMode_On );
        camera.TriggerSource.SetValue ( TriggerSource_Line3 );
        camera.TriggerActivation.SetValue( TriggerActivation_FallingEdge );
        camera.ExposureMode.SetValue( ExposureMode_Timed );
        camera.ExposureTime.SetValue( 20000.0 );
        camera.AcquisitionStart.Execute( );


        // Check if the device supports events.
        if ( !IsAvailable( camera.EventSelector))
        {
            throw RUNTIME_EXCEPTION( "The device doesn't support events.");
        }
        // Enable the sending of Exposure End events.
        // Select the event to be received.
        camera.EventSelector.SetValue(EventSelector_ExposureEnd);
        // Enable it.
        camera.EventNotification.SetValue(EventNotification_On);
        // Enable the sending of Frame Start Overtrigger events.
        if ( IsAvailable( camera.EventSelector.GetEntry(EventSelector_FrameStartOvertrigger)))
        {
            camera.EventSelector.SetValue(EventSelector_FrameStartOvertrigger);
            camera.EventNotification.SetValue(EventNotification_On);
        }


        // // Register the standard configuration event handler for enabling software triggering.
        // // The software trigger configuration handler replaces the default configuration
        // // as all currently registered configuration handlers are removed by setting the registration mode to RegistrationMode_ReplaceAll.
        // camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);

        // // For demonstration purposes only, add sample configuration event handlers to print out information
        // // about camera use and image grabbing.
        // camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete);
        // camera.RegisterImageEventHandler( new CImageEventPrinter, RegistrationMode_Append, Cleanup_Delete);

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        camera.MaxNumBuffer = 15;


        // TRY USING HARDWARE TRIGGER

        // Select output line Line 3 and read the status
        camera.LineSelector.SetValue(LineSelector_Line3);
        int cnt = 0;
        while(false)
        {
            // Getting informed about the line status of all I/O lines
            //int64_t i = camera.LineStatusAll.GetValue();
            //cout << cnt << ": " << i << endl;
            bool b = camera.LineStatus.GetValue();
            cout << cnt << ": " << b << endl;
            cnt++;
        }


        int frameCount = 0;
        char frameFilename[100];
        while (frameCount < 2)
        {
            // // Set height of image (cuts off bottom portion)
            // camera.Height.SetValue( camera.Height.GetInc() * (increments / (frameCount+1)));

            // Set exposure time in microseconds
            //camera.ExposureTime.SetValue( pow(10.0,frameCount+2) );

            bool b = camera.LineStatus.GetValue();
            cout << frameCount << ": " << b << endl;


            // Grab a single image
            if (camera.GrabOne(5000,ptrGrabResult,TimeoutHandling_ThrowException))
            {
                cout << "MJR: Start GrabOne()" << endl;

                frameCount = frameCount + 1;

                // // Access the image data.
                // cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                // cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                // const uint8_t *pImageBuffer = (uint8_t *) ptrGrabResult->GetBuffer();
                // cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;

                // Save the image using Pylon API
                sprintf(frameFilename,"GrabbedImage_%.2d.png",frameCount);                
                CImagePersistence::Save( ImageFileFormat_Png, frameFilename, ptrGrabResult);
                //CImagePersistence::Save( ImageFileFormat_Png, "GrabbedImage.png", ptrGrabResult);
                cout << "MJR: End GrabOne()" << endl;
            }
        }



        // cout << endl << endl << "Grab using the GrabStrategy_OneByOne default strategy:" << endl;
        // cout <<                 "======================================================" << endl << endl;

        // // The GrabStrategy_OneByOne strategy is used. The images are processed
        // // in the order of their arrival.
        // camera.StartGrabbing( GrabStrategy_OneByOne);

        // // In the background, the grab engine thread retrieves the
        // // image data and queues the buffers into the internal output queue.

        // // Issue software triggers. For each call, wait up to 1000 ms until the camera is ready for triggering the next image.
        // cerr << endl << "Press Enter to begin." << endl;
        // while( cin.get() != '\n');
        // for ( int i = 0; i < 3; ++i)
        // {
        //     if ( camera.WaitForFrameTriggerReady( 1000, TimeoutHandling_ThrowException))
        //     {
        //         camera.ExecuteSoftwareTrigger();
        //     }
        // }

        // // For demonstration purposes, wait for the last image to appear in the output queue.
        // WaitObject::Sleep( 3*1000);

        // // Check that grab results are waiting.
        // if ( camera.GetGrabResultWaitObject().Wait( 0))
        // {
        //     cout << endl << "Grab results are waiting in the output queue." << endl << endl;
        // }

        // // All triggered images are still waiting in the output queue and are retrieved below.
        // // The grabbing continues in the background, e.g. when using hardware trigger mode,
        // // as long as the grab engine does not run out of buffers.
        // int nBuffersInQueue = 0;
        // while( camera.RetrieveResult( 0, ptrGrabResult, TimeoutHandling_Return))
        // {
        //     nBuffersInQueue++;
        // }
        // cout << "Retrieved " << nBuffersInQueue << " grab results from output queue." << endl << endl;

        // //Stop the grabbing.
        // camera.StopGrabbing();



        // cout << endl << endl << "Grab using the GrabStrategy_LatestImageOnly strategy:" << endl;
        // cout <<                 "=====================================================" << endl << endl;

        // // The GrabStrategy_LatestImageOnly strategy is used. The images are processed
        // // in the order of their arrival but only the last received image
        // // is kept in the output queue.
        // // This strategy can be useful when the acquired images are only displayed on the screen.
        // // If the processor has been busy for a while and images could not be displayed automatically
        // // the latest image is displayed when processing time is available again.
        // camera.StartGrabbing( GrabStrategy_LatestImageOnly);

        // // Execute the software trigger, wait actively until the camera accepts the next frame trigger or until the timeout occurs.
        // cerr << endl << "Press Enter to begin." << endl;
        // while( cin.get() != '\n');
        // for ( int i = 0; i < 3; ++i)
        // {
        //     if ( camera.WaitForFrameTriggerReady( 1000, TimeoutHandling_ThrowException))
        //     {
        //         camera.ExecuteSoftwareTrigger();
        //     }
        // }

        // // Wait for all images.
        // WaitObject::Sleep( 3*1000);

        // // Check whether the grab result is waiting.
        // if ( camera.GetGrabResultWaitObject().Wait( 0))
        // {
        //     cout << endl << "A grab result is waiting in the output queue." << endl << endl;
        // }

        // // Only the last received image is waiting in the internal output queue
        // // and is retrieved below.
        // // The grabbing continues in the background, e.g. when using hardware trigger mode.
        // nBuffersInQueue = 0;
        // while( camera.RetrieveResult( 0, ptrGrabResult, TimeoutHandling_Return))
        // {
        //     cout << "Skipped " << ptrGrabResult->GetNumberOfSkippedImages() << " images." << endl;
        //     nBuffersInQueue++;
        // }

        // cout << "Retrieved " << nBuffersInQueue << " grab result from output queue." << endl << endl;

        // //Stop the grabbing.
        // camera.StopGrabbing();



        // cout << endl << endl << "Grab using the GrabStrategy_LatestImages strategy:" << endl;
        // cout <<                 "==================================================" << endl << endl;

        // // The GrabStrategy_LatestImages strategy is used. The images are processed
        // // in the order of their arrival, but only a number of the images received last
        // // are kept in the output queue.

        // // The size of the output queue can be adjusted.
        // // When using this strategy the OutputQueueSize parameter can be changed during grabbing.
        // camera.OutputQueueSize = 2;

        // camera.StartGrabbing( GrabStrategy_LatestImages);

        // // Execute the software trigger, wait actively until the camera accepts the next frame trigger or until the timeout occurs.
        // cerr << endl << "Press Enter to begin." << endl;
        // while( cin.get() != '\n');
        // for ( int i = 0; i < 3; ++i)
        // {
        //     if ( camera.WaitForFrameTriggerReady( 1000, TimeoutHandling_ThrowException))
        //     {
        //         camera.ExecuteSoftwareTrigger();
        //     }
        // }

        // // Wait for all images.
        // WaitObject::Sleep( 3*1000);

        // // Check whether the grab results are waiting.
        // if ( camera.GetGrabResultWaitObject().Wait( 0))
        // {
        //     cout << endl << "Grab results are waiting in the output queue." << endl << endl;
        // }

        // // Only the images received last are waiting in the internal output queue
        // // and are retrieved below.
        // // The grabbing continues in the background, e.g. when using hardware trigger mode.
        // nBuffersInQueue = 0;
        // while( camera.RetrieveResult( 0, ptrGrabResult, TimeoutHandling_Return))
        // {
        //     if ( ptrGrabResult->GetNumberOfSkippedImages())
        //     {
        //         cout << "Skipped " << ptrGrabResult->GetNumberOfSkippedImages() << " image." << endl;
        //     }
        //     nBuffersInQueue++;
        // }

        // cout << "Retrieved " << nBuffersInQueue << " grab results from output queue." << endl << endl;

        // // When setting the output queue size to 1 this strategy is equivalent to grab strategy GrabStrategy_LatestImageOnly.
        // camera.OutputQueueSize = 1;

        // // When setting the output queue size to CInstantCamera::MaxNumBuffer this strategy is equivalent to GrabStrategy_OneByOne.
        // camera.OutputQueueSize = camera.MaxNumBuffer;

        // //Stop the grabbing.
        // camera.StopGrabbing();



        // The Upcoming Image grab strategy cannot be used together with USB camera devices.
        // For more information, see the advanced topics section of the pylon Programmer's Guide.
        if ( !camera.IsUsb())
        {
            cout << endl << endl << "Grab using the GrabStrategy_UpcomingImage strategy:" << endl;
            cout <<                 "==================================================" << endl << endl;

            // Reconfigure the camera to use continuous acquisition.
            CAcquireContinuousConfiguration().OnOpened( camera);

            // The GrabStrategy_UpcomingImage strategy is used. A buffer for grabbing
            // is queued each time when RetrieveResult()
            // is called. The image data is grabbed into the buffer and returned.
            // This ensures that the image grabbed is the next image
            // received from the camera.
            // All images are still transported to the PC.
            camera.StartGrabbing( GrabStrategy_UpcomingImage);

            // Queues a buffer for grabbing and waits for the grab to finish.
            camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);

        // Sleep.
        WaitObject::Sleep( 1000);

            // Check no grab result is waiting, because no buffers are queued for grabbing.
            if ( !camera.GetGrabResultWaitObject().Wait( 0))
            {
                cout << "No grab result waits in the output queue." << endl << endl;
            }

            //Stop the grabbing
            camera.StopGrabbing();        
        }
    }
    catch (GenICam::GenericException &e)
    {
        // Error handling.
        cerr << "MJR..." << endl;
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    return exitCode;
}
